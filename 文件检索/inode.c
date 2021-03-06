#include "inode.h"
#include "fs.h"
#include "file.h"
#include "global.h"
#include "debug.h"
#include "memory.h"
#include "interrupt.h"
#include "list.h"
#include "stdio-kernel.h"
#include "string.h"
#include "super_block.h"

struct inode_position {
  bool two_sec;
  uint32_t sec_lba;
  uint32_t off_size;
};

static void inode_locate(struct partition* part, uint32_t inode_no, struct inode_position* inode_pos) {
  ASSERT(inode_no < 4096);
  uint32_t inode_table_lba = part->sb->inode_table_lba;

  uint32_t inode_size = sizeof(struct inode);
  uint32_t off_size = inode_no * inode_size;
  uint32_t off_sec = off_size / 512;
  uint32_t off_size_in_sec = off_size % 512;

  uint32_t left_in_sec = 512 - off_size_in_sec;
  if (left_in_sec < inode_size ) {	  // 若扇区内剩下的空间不足以容纳一个inode,必然是I结点跨越了2个扇区
     inode_pos->two_sec = true;
  } else {				  // 否则,所查找的inode未跨扇区
     inode_pos->two_sec = false;
  }
   inode_pos->sec_lba = inode_table_lba + off_sec;
   inode_pos->off_size = off_size_in_sec;
}


void inode_sync(struct partition* part, struct inode* inode, void* io_buf) {	 // io_buf是用于硬盘io的缓冲区
    uint8_t inode_no = inode->i_no;
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));

    struct inode pure_inode;
    memcpy(&pure_inode, inode, sizeof(struct inode));

    pure_inode.i_open_cnts = 0;
    pure_inode.write_deny = false;	 // 置为false,以保证在硬盘中读出时为可写
    pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;

    char* inode_buf = (char*)io_buf;
    if (inode_pos.two_sec) {

        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);	//

        memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    } else {
      ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
      memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
      ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }
}

struct inode* inode_open(struct partition* part, uint32_t inode_no) {
    struct list_elem* elem = part->open_inodes.head.next;
    struct inode* inode_found;
    while (elem != &part->open_inodes.tail) {
        inode_found = elem2entry(struct inode, inode_tag, elem);
        if (inode_found->i_no == inode_no) {
  	 inode_found->i_open_cnts++;
  	 return inode_found;
        }
        elem = elem->next;
     }

     struct inode_position inode_pos;
     inode_locate(part, inode_no, &inode_pos);
     struct task_struct* cur = running_thread();
     uint32_t* cur_pagedir_bak = cur->pgdir;
     cur->pgdir = NULL;

     inode_found = (struct inode*)sys_malloc(sizeof(struct inode));
     cur->pgdir = cur_pagedir_bak;

     char* inode_buf;
     if (inode_pos.two_sec) {
       inode_buf = (char*)sys_malloc(1024);
       ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    } else {	// 否则,所查找的inode未跨扇区,一个扇区大小的缓冲区足够
       inode_buf = (char*)sys_malloc(512);
       ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
    }
    memcpy(inode_found, inode_buf + inode_pos.off_size, sizeof(struct inode));

    list_push(&part->open_inodes, &inode_found->inode_tag);
    inode_found->i_open_cnts = 1;

    sys_free(inode_buf);
    return inode_found;
}

void inode_close(struct inode* inode) {

   enum intr_status old_status = intr_disable();
   if (--inode->i_open_cnts == 0) {
     list_remove(&inode->inode_tag);
     struct task_struct* cur = running_thread();
     uint32_t* cur_pagedir_bak = cur->pgdir;
     cur->pgdir = NULL;
     sys_free(inode);
     cur->pgdir = cur_pagedir_bak;
   }
   intr_set_status(old_status);
}

void inode_init(uint32_t inode_no, struct inode* new_inode) {
   new_inode->i_no = inode_no;
   new_inode->i_size = 0;
   new_inode->i_open_cnts = 0;
   new_inode->write_deny = false;

   /* 初始化块索引数组i_sector */
   uint8_t sec_idx = 0;
   while (sec_idx < 13) {
   /* i_sectors[12]为一级间接块地址 */
      new_inode->i_sectors[sec_idx] = 0;
      sec_idx++;
   }
}
