#ifndef __FS_FS_H
#define __FS_FS_H
#include "stdint.h"
#include "ide.h"

#define MAX_FILES_PER_PART 4096	    // 每个分区所支持最大创建的文件数
#define BITS_PER_SECTOR 4096	    // 每扇区的位数
#define SECTOR_SIZE 512		    // 扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE	    // 块字节大小

enum file_types{
  FT_UNKNOWN,
  FT_REGULAR,
  FT_DIRECTORY
};
void filesys_init(void);

struct path_search_record{
  char searched_path[MAX_PATH_LEN];
  struct dir* parent_dir;
  enum file_types file_type;
}

extern struct partition* cur_part;
void filesys_init(void);
int32_t path_depth_cnt(char* pathname);
int32_t sys_open(const char* pathname, uint8_t flags);
#endif
