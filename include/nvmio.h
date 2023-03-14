#ifndef _ATFS_NVMIO_H
#define _ATFS_NVMIO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "allocator.h"
#include "inode.h"
#include "types.h"

struct log_page_tail {};

struct file_log_entry {
  u64 data;
  u64 file_size;

  u64 start_write_off;
  u64 end_write_off;

  u32 num_pages;
  u32 invalid_pages;
  u64 bitmap;
};

void init_atfs(void);
struct file_log_entry *get_log_entry(u64 *log_tail);
void init_log_entry(struct file_log_entry *entry, u64 pages, u64 file_size,
                    u64 alloc_num, u64 start_wrtie_off, u64 end_write_off);
void update_inode_tail(struct atfs_inode *inode, u64 log_tail);
void update_inode_offset(struct atfs_inode *inode, u64 cnt);
void update_inode_size(struct atfs_inode *inode, u64 cnt);
void update_inode_info(struct atfs_inode *inode, u64 cnt);

ssize_t nvmio_write(struct atfs_inode *inode, const void *buf, size_t cnt,
                    u64 file_off);
ssize_t nvmio_read(struct atfs_inode *inode, void *buf, size_t cnt,
                   u64 file_off);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ATFS_NVMIO_H */
