#include "ide.h"
#include "sync.h"
#include "stdio.h"
#include "stdio-kernel.h"
#include "interrupt.h"
#include "memory.h"
#include "debug.h"
#include "string.h"

/* 定义硬盘各寄存器的端口号 */
#define reg_data(channel)	 (channel->port_base + 0)
#define reg_error(channel)	 (channel->port_base + 1)
#define reg_sect_cnt(channel)	 (channel->port_base + 2)
#define reg_lba_l(channel)	 (channel->port_base + 3)
#define reg_lba_m(channel)	 (channel->port_base + 4)
#define reg_lba_h(channel)	 (channel->port_base + 5)
#define reg_dev(channel)	 (channel->port_base + 6)
#define reg_status(channel)	 (channel->port_base + 7)
#define reg_cmd(channel)	 (reg_status(channel))
#define reg_alt_status(channel)  (channel->port_base + 0x206)
#define reg_ctl(channel)	 reg_alt_status(channel)

#define BIT_STAT_BSY	 0x80	      // 硬盘忙
#define BIT_STAT_DRDY	 0x40	      // 驱动器准备好
#define BIT_STAT_DRQ	 0x8	      // 数据传输准备好了

/* device寄存器的一些关键位 */
#define BIT_DEV_MBS	0xa0	    // 第7位和第5位固定为1
#define BIT_DEV_LBA	0x40
#define BIT_DEV_DEV	0x10

/* 一些硬盘操作的指令 */
#define CMD_IDENTIFY	   0xec	    // identify指令
#define CMD_READ_SECTOR	   0x20     // 读扇区指令
#define CMD_WRITE_SECTOR   0x30	    // 写扇区指令

#define max_lba ((80*1024*1024/512) - 1)

uint8_t channel_cnt;	   // 按硬盘数计算的通道数
struct ide_channel channels[2];	 // 有两个ide通道

int32_t ext_lba_base = 0;

uint8_t p_no = 0, l_no = 0;

struct list partition_list;

struct partition_table_entry{
  uint8_t bootable;
  uint8_t start_head;
  uint8_t start_sec;
  uint8_t start_chs;
  uint8_t fs_type;
  uint8_t end_head;
  uint8_t end_sec;
  uint8_t end_chs;

  uint32_t start_lba;
  uint32_t sec_cnt;
}__attribute__((packed));

struct boot_sector{
  uint8_t other[446];
  struct partition_table_entry partition_table[4];
  uint16_t signature;
}__attribute__((packed));





static void select_disk(struct disk* hd) {
  uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
  if (hd->dev_no == 1) {	// 若是从盘就置DEV位为1
     reg_device |= BIT_DEV_DEV;
  }
   outb(reg_dev(hd->my_channel), reg_device);
  /* code */
}

static void select_sector(struct disk* hd, uint32_t lba, uint8_t sec_cnt){
  ASSERT(lba <= max_lba);
  struct ide_channel* channel = hd->my_channel;

  outb(reg_sect_cnt(channel), sec_cnt);	 // 如果sec_cnt为0,则表示写入256个扇区

  /* 写入lba地址(即扇区号) */
  outb(reg_lba_l(channel), lba);		 // lba地址的低8位,不用单独取出低8位.outb函数中的汇编指令outb %b0, %w1会只用al。
  outb(reg_lba_m(channel), lba >> 8);		 // lba地址的8~15位
  outb(reg_lba_h(channel), lba >> 16);		 // lba地址的16~23位

  /* 因为lba地址的24~27位要存储在device寄存器的0～3位,
   * 无法单独写入这4位,所以在此处把device寄存器再重新写入一次*/
  outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_LBA | (hd->dev_no == 1 ? BIT_DEV_DEV : 0) | lba >> 24);


}

static void cmd_out(struct ide_channel* channel, uint8_t cmd) {
/* 只要向硬盘发出了命令便将此标记置为true,硬盘中断处理程序需要根据它来判断 */
   channel->expecting_intr = true;
   outb(reg_cmd(channel), cmd);
}

static void read_from_sector(struct disk* hd, void* buf, uint8_t sec_cnt){
  uint32_t size_in_byte;
  if (sec_cnt == 0) {
  /* 因为sec_cnt是8位变量,由主调函数将其赋值时,若为256则会将最高位的1丢掉变为0 */
     size_in_byte = 256 * 512;
  } else {
     size_in_byte = sec_cnt * 512;
  }
  insw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

static void write2sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
   uint32_t size_in_byte;
   if (sec_cnt == 0) {
   /* 因为sec_cnt是8位变量,由主调函数将其赋值时,若为256则会将最高位的1丢掉变为0 */
      size_in_byte = 256 * 512;
   } else {
      size_in_byte = sec_cnt * 512;
   }
   outsw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

static bool busy_wait(struct disk* hd){
  struct ide_channel* channel = hd->my_channel;
  uint16_t time_limit = 30 * 1000;
  while (time_limit -= 10 >= 0) {
     if (!(inb(reg_status(channel)) & BIT_STAT_BSY)) {
  return (inb(reg_status(channel)) & BIT_STAT_DRQ);
     } else {
  mtime_sleep(10);		     // 睡眠10毫秒
     }
  }
  return false;
}

void ide_read(struct disk* hd, uint32_t lba, void* buf,uint32_t sec_cnt){
  ASSERT(lba <= max_lba);
  ASSERT(sec_cnt > 0);
  lock_acquire (&hd->my_channel->lock);

  select_disk(hd);

  uint32_t secs_op;		 // 每次操作的扇区数
  uint32_t secs_done = 0;	 // 已完成的扇区数
  while(secs_done < sec_cnt) {
     if ((secs_done + 256) <= sec_cnt) {
  secs_op = 256;
     } else {
  secs_op = sec_cnt - secs_done;
     }

     select_sector(hd, lba + secs_done, secs_op);
    cmd_out(hd->my_channel, CMD_READ_SECTOR);
    sema_down(&hd->my_channel->disk_done);

    if (!busy_wait(hd)) {	 // 若失败
 char error[64];
 sprintf(error, "%s read sector %d failed!!!!!!\n", hd->name, lba);
 PANIC(error);
    }

    read_from_sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);
    secs_done += secs_op;

  }
  lock_release(&hd->my_channel->lock);
}

void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
  ASSERT(lba <= max_lba);
  ASSERT(sec_cnt > 0);
  lock_acquire (&hd->my_channel->lock);

  elect_disk(hd);

  uint32_t secs_op;		 // 每次操作的扇区数
  uint32_t secs_done = 0;	 // 已完成的扇区数
  while(secs_done < sec_cnt) {
     if ((secs_done + 256) <= sec_cnt) {
  secs_op = 256;
     } else {
  secs_op = sec_cnt - secs_done;
     }

     select_sector(hd, lba + secs_done, secs_op);

/* 3 执行的命令写入reg_cmd寄存器 */
     cmd_out(hd->my_channel, CMD_WRITE_SECTOR);
     if (!busy_wait(hd)) {			      // 若失败
  char error[64];
  sprintf(error, "%s write sector %d failed!!!!!!\n", hd->name, lba);
  PANIC(error);
     }
     write2sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);

     /* 在硬盘响应期间阻塞自己 */
     sema_down(&hd->my_channel->disk_done);
     secs_done += secs_op;
   }
   lock_release(&hd->my_channel->lock);

}

void intr_hd_handler(uint8_t irq_no){
  ASSERT(irq_no == 0x2e || irq_no == 0x2f);
  uint8_t ch_no = irq_no - 0x2e;
  struct ide_channel* channel = &channels[ch_no];
  ASSERT(channel->irq_no == irq_no);
  if (channel->expecting_intr) {
     channel->expecting_intr = false;
     sema_up(&channel->disk_done);

/* 读取状态寄存器使硬盘控制器认为此次的中断已被处理,从而硬盘可以继续执行新的读写 */
     inb(reg_status(channel));
  }
}

static void swap_pairs_bytes(const char* dst, char* buf, uint32_t len){
  uint8_t idx;
  for (idx = 0; idx < len; idx += 2){
    buf[idx + 1] = *dst++;
    buf[idx]     = *dst++;
  }
  buf[idx] = '\0';
}

static void identify_disk(struct disk* hd) {
  char id_info[512];
  select_disk(hd);
  cmd_out(hd->my_channel, CMD_IDENTIFY);
  sema_down(&hd->my_channel->disk_done);

  if (!busy_wait(hd)) {     //  若失败
     char error[64];
     sprintf(error, "%s identify failed!!!!!!\n", hd->name);
     PANIC(error);
  }
  read_from_sector(hd, id_info, 1);
  char buf[64];
  uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;
  swap_pairs_bytes(&id_info[sn_start], buf, sn_len);
  printk("   disk %s info:\n      SN: %s\n", hd->name, buf);
  memset(buf, 0, sizeof(buf));
  swap_pairs_bytes(&id_info[md_start], buf, md_len);
  printk("      MODULE: %s\n", buf);
  uint32_t sectors = *(uint32_t*)&id_info[60 * 2];
  printk("      SECTORS: %d\n", sectors);
  printk("      CAPACITY: %dMB\n", sectors * 512 / 1024 / 1024);
}

static void partition_scan(struct disk* hd, uint32_t ext_lba) {
  struct boot_sector* bs = sys_malloc(sizeof(struct boot_sector));
  ide_read(hd, ext_lba, bs, 1);
  uint8_t part_idx = 0;
  struct partition_table_entry* p = bs->partition_table;

  while (part_idx++ < 4) {
     if (p->fs_type == 0x5) {	 // 若为扩展分区
  if (ext_lba_base != 0) {
      partition_scan(hd, p->start_lba + ext_lba_base);
  } else {
    ext_lba_base = p->start_lba;
    partition_scan(hd, p->start_lba);
 }
  } else if (p->fs_type != 0) { // 若是有效的分区类型
      if (ext_lba == 0) {	 // 此时全是主分区
        hd->prim_parts[p_no].start_lba = ext_lba + p->start_lba;
        hd->prim_parts[p_no].sec_cnt = p->sec_cnt;
        hd->prim_parts[p_no].my_disk = hd;
        list_append(&partition_list, &hd->prim_parts[p_no].part_tag);
        sprintf(hd->prim_parts[p_no].name, "%s%d", hd->name, p_no + 1);
        p_no++;
        ASSERT(p_no < 4);	    // 0,1,2,3
    } else {
        hd->logic_parts[l_no].start_lba = ext_lba + p->start_lba;
        hd->logic_parts[l_no].sec_cnt = p->sec_cnt;
        hd->logic_parts[l_no].my_disk = hd;
        list_append(&partition_list, &hd->logic_parts[l_no].part_tag);
        sprintf(hd->logic_parts[l_no].name, "%s%d", hd->name, l_no + 5);	 // 逻辑分区数字是从5开始,主分区是1～4.
        l_no++;
      if (l_no >= 8)    // 只支持8个逻辑分区,避免数组越界
           return;
    }
  }
      p++;
}
  sys_free(bs);
}

static bool partition_info(struct list_elem* pelem, int arg UNUSED) {
   struct partition* part = elem2entry(struct partition, part_tag, pelem);
   printk("   %s start_lba:0x%x, sec_cnt:0x%x\n",part->name, part->start_lba, part->sec_cnt);

/* 在此处return false与函数本身功能无关,
 * 只是为了让主调函数list_traversal继续向下遍历元素 */
   return false;
}



void ide_init() {
  printk("ide_init start\n");
  uint8_t hd_cnt = *((uint8_t*)(0x475));	      // 获取硬盘的数量
  ASSERT(hd_cnt > 0);
  channel_cnt = DIV_ROUND_UP(hd_cnt, 2);
  struct ide_channel* channel;
  uint8_t channel_no = 0;

  while (channel_no < channel_cnt) {
    channel = &channel[channel_no];
    sprintf(channel->name, "ide%d", channel_no);

    switch (channel_no) {
      case 0:
   	    channel->port_base	 = 0x1f0;	   // ide0通道的起始端口号是0x1f0
   	    channel->irq_no	 = 0x20 + 14;	   // 从片8259a上倒数第二的中断引脚,温盘,也就是ide0通道的的中断向量号
   	    break;
   	 case 1:
   	    channel->port_base	 = 0x170;	   // ide1通道的起始端口号是0x170
   	    channel->irq_no	 = 0x20 + 15;	   // 从8259A上的最后一个中断引脚,我们用来响应ide1通道上的硬盘中断
   	    break;
    }

    channel->expecting_intr = false;		   // 未向硬盘写入指令时不期待硬盘的中断
    lock_init(&channel->lock);

    sema_init(&channel->disk_done, 0);
    register_handler(channel->irq_no, intr_hd_handler);
    channel_no++;				   // 下一个channel
  }
  printk("ide_init done\n");
}
