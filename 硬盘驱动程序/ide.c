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
