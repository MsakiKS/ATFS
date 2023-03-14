#include <pthread.h>

#include "./include/allocator.h"
#include "./include/bplustree.h"
#include "./include/inode.h"
#include "./include/internal.h"

void init_atfs_inode(struct atfs_inode *inode_node) {
  list_node_t *pages = NULL;

  ATFS_DEBUG("init_atfs_inode start");

  inode_node->valid = 0;
  inode_node->deleted = 0;
  inode_node->i_size = 0;
  inode_node->fd = -1;

  pages = alloc_free_pages(WRITE_LOG_PERALLOC);

  inode_node->log_head = pages->offset;
  inode_node->log_tail = pages->offset;
  inode_node->file_off = 0;

  // init_radixtree_root(&inode_node->radix_tree, RADIXTREE_PAGE);

  pthread_mutex_init(&inode_node->mutex, NULL);

  ATFS_DEBUG("init_atfs_inode success");
}