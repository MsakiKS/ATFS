#ifndef _ATFS_BPLUSLOG_H
#define _ATFS_BPLUSLOG_H

#include <pthread.h>

#include "types.h"

#define PER_NODE_SHIFT 8
#define PTRS_PER_NODE (1ULL << PER_NODE_SHIFT) /* 256 */

typedef enum bplustree_type_enum {
  BPLUSTREE_INODE,
  BPLUSTREE_PAGE,
} bplustree_type_t;

typedef struct bplustree_node_struct {
  u64 in_page;
  u64 next;
  int count;
  int index;
  u64 entries[PTRS_PER_NODE];
  pthread_mutex_t mutex;
} bplustree_node_t;

typedef struct bplustree_struct {
  u64 count;
  u64 root;
  bplustree_type_t type;
  pthread_mutex_t mutex;
} bplustree_t;

void init_bplustree_root(bplustree_t *root, bplustree_type_t type);
void init_bplustree_node(bplustree_node_t *node);
u64 get_bplustree_node(bplustree_t *root, u64 index, bplustree_type_t type);
void set_bplustree_node(bplustree_t *root, u64 value, u64 index,
                        bplustree_type_t type);

int atomic_increase(int *count);

#endif /* _ATFS_TREELOG_H */
