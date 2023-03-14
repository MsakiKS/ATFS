#ifndef _ATFS_ALLOCATOR_H
#define _ATFS_ALLOCATOR_H

#include "inode.h"
#include "types.h"

typedef enum allocator_type_enum {
  ALLOCATOR_INODE,
  ALLOCATOR_BPLUSTREE,
} allocator_type_t;

typedef struct list_node_struct {
  u64 next_offset;  // 下一页
  u64 offset;       // 当前页

  u32 obj_cnt;        // 页面切分成多个对象
  u32 invalid_cnt;    // 不可用对象
  u32 pages_cnt;      // 总计的页面数据
  u32 invalid_pages;  // 不可用的页面数据
  u64 log_entry;

} list_node_t;

typedef struct pagelist_struct {
  u64 head;
  u64 count;
  pthread_mutex_t mutex;
} pagelist_t;

typedef struct allocator_list_struct {
  u64 head;
  u64 count;
  allocator_type_t type;
  pthread_mutex_t mutex;
} allocator_list_t;

void init_allocator_list(allocator_list_t *alloc_list);
list_node_t *alloc_free_pages(u32 page_num);
void free_pages(list_node_t *node, u32 page_num);

void fill_atfs_inode_list(allocator_list_t *alloc_list);
u64 alloc_atfs_inode(void);
void free_atfs_inode(struct atfs_inode *node);

#endif /* _ATFS_ALLOCATOR_H */
