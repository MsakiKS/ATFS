#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "./include/allocator.h"
#include "./include/debug.h"
#include "./include/inode.h"
#include "./include/internal.h"
#include "./include/nvmio.h"
#include "./include/super.h"

extern void *g_nvm_base_addr;
extern int get_cpu_nums(void);
extern void kernel_gc_thread_init(void);

static char *g_atfs_path = NULL;
static char *g_atfs_file = "atfs_file";

bool process_initialized = false;
extern u32 g_cpu_nums;

void init_env(void) {
  int fd, s;
  char filename[256];
  unsigned long long *init_num = NULL;

  g_atfs_path = "/home/rita/atfs";
  if (__glibc_unlikely(g_atfs_path == NULL)) {
    handle_error("g_atfs_path is NULL.");
  }
  ATFS_DEBUG("get g_atfs_path success");

  sprintf(filename, "%s/%s", g_atfs_path, g_atfs_file);
  fd = open(filename, O_CREAT | O_RDWR, 0777);
  if (fd < 0) {
    handle_error("open g_atfs_path failed");
  }
  ATFS_DEBUG("open g_atfs_file success");
  s = posix_fallocate(fd, 0, NVM_MMAP_SIZE);
  if (__glibc_unlikely(s != 0)) {
    handle_error("fallocate");
  }
  ATFS_DEBUG("fallocate g_atfs_file success");

  g_nvm_base_addr = mmap(NULL, NVM_MMAP_SIZE, PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_POPULATE, fd, 0);
  if (__glibc_unlikely(g_nvm_base_addr == NULL)) {
    handle_error("g_nvm_base_addr is NULL");
  }
  ATFS_DEBUG("mmap g_atfs_file success");

  init_num = g_nvm_base_addr;
  if (__sync_bool_compare_and_swap(init_num, 0ULL, SB_INIT_NUMBER)) {
    kernel_page_list_init();

    kernel_inode_list_init();
    kernel_radixtree_list_init();

    kernel_superblock_init(g_nvm_base_addr, g_atfs_path);

    kernel_gc_thread_init();

    ATFS_DEBUG("init super block success");
  }
}

void init_atfs(void) {
  if (__sync_bool_compare_and_swap(&process_initialized, false, true)) {
    init_env();
  }
}

bool is_last_entry(u64 tail) {
  u64 offset = get_nvm_page_off2free_space(tail);
  u64 entry_end = (offset % ATFS_PAGE_SIZE) + sizeof(struct file_log_entry);

  return entry_end > PAGE_SIZE;
}

list_node_t *entry2log_page(struct file_log_entry *entry) {
  char *free_space = get_free_space_base_addr();
  char *page_addr =
      free_space + (nvm_addr2off(entry) - nvm_addr2off(free_space)) /
                       ATFS_PAGE_SIZE * ATFS_PAGE_SIZE;

  return get_nvm_page2node_addr(page_addr);
}

struct file_log_entry *get_log_entry(u64 *log_tail) {
  u64 entry_size = sizeof(struct file_log_entry);
  list_node_t *pages = NULL;
  list_node_t *last_page = NULL;
  u64 free_space;
  u64 last_page_off;
  struct file_log_entry *entry = NULL;

  ATFS_DEBUG("get_log_entry start");

  if (is_last_entry(*log_tail)) {
    free_space = nvm_addr2off(get_free_space_base_addr());
    last_page_off =
        free_space + (*log_tail - free_space) / ATFS_PAGE_SIZE * ATFS_PAGE_SIZE;
    last_page = get_nvm_page_node_addr(last_page_off);
    pages = alloc_free_pages(WRITE_LOG_PERALLOC);
    last_page->next_offset = pages->offset;
    *log_tail = pages->offset + entry_size;

    pages->invalid_cnt = 0;
    pages->obj_cnt = 1;
    pages->invalid_pages = 0;
    pages->pages_cnt = 0;

    ATFS_DEBUG("get_log_entry success");
    return nvm_off2addr(pages->offset);
  }

  entry = nvm_off2addr(*log_tail);

  *log_tail += entry_size;
  pages = entry2log_page(nvm_off2addr(*log_tail));
  ++pages->obj_cnt;

  ATFS_DEBUG("get_log_entry success");
  return entry;
}

void init_log_entry(struct file_log_entry *entry, u64 pages, u64 file_size,
                    u64 alloc_num, u64 start_wrtie_off, u64 end_write_off) {
  list_node_t *page_node;

  page_node = entry2log_page(entry);
  page_node->pages_cnt += alloc_num;

  memset(entry, 0, sizeof(struct file_log_entry));

  entry->data = pages;
  entry->file_size = file_size;

  entry->start_write_off = start_wrtie_off;
  entry->end_write_off = end_write_off;

  entry->num_pages = alloc_num;
}

void update_inode_tail(struct atfs_inode *inode, u64 log_tail) {
  inode->log_tail = log_tail;
}

void update_inode_offset(struct atfs_inode *inode, u64 cnt) {
  inode->file_off += cnt;
}

void update_inode_size(struct atfs_inode *inode, u64 cnt) {
  if (inode->file_off + cnt > inode->i_size) {
    inode->i_size = inode->file_off + cnt;
  }
}

void update_inode_info(struct atfs_inode *inode, u64 cnt) {
  update_inode_offset(inode, cnt);
  update_inode_size(inode, 0);
}

u64 next_log_page(u64 tail) {
  list_node_t *last_page = NULL;
  char *free_space = get_free_space_base_addr();
  char *last_page_addr = free_space + (tail - nvm_addr2off(free_space)) /
                                          ATFS_PAGE_SIZE * ATFS_PAGE_SIZE;

  last_page = get_nvm_page2node_addr(last_page_addr);

  return last_page->next_offset;
}

void invalidate_old_log_entry(struct file_log_entry *old_entry, u64 page_off) {
  u64 start_off = old_entry->start_write_off / PAGE_SIZE;
  u64 curr_off = page_off / PAGE_SIZE;
  u64 pos = curr_off - start_off;
  list_node_t *page_node = NULL;

  old_entry->bitmap |= (1ULL << pos);
  ++old_entry->invalid_pages;

  page_node = entry2log_page(old_entry);
  ++page_node->invalid_pages;
  if (old_entry->num_pages == old_entry->invalid_pages) {
    ++page_node->invalid_cnt;
  }
}

// void update_radixtree(struct unvmfs_inode *inode, u64 old_log_tail) {}

ssize_t nvmio_write(struct atfs_inode *inode, const void *buf, size_t cnt,
                    u64 file_off) {
  int i;
  // u64 file_off;
  u64 file_size;
  u64 offset;
  u64 page_nums;
  u64 start_page;
  int alloc_num;
  int mid_num;
  u64 wr_size;
  u64 total_size = cnt;
  u64 first_page_wr;
  u64 last_page_wr;
  const char *user_buf = buf;
  char *node2page;
  list_node_t *pages = NULL;
  list_node_t *part_page = NULL;
  list_node_t *page_addr_node = NULL;
  char *page_addr = NULL;
  u64 part_page_off;
  u64 start_write_off;
  u64 end_write_off;
  u64 log_tail;
  u64 old_log_tail;
  u64 entry_off;

  struct file_log_entry *entry = NULL;

  ATFS_DEBUG("nvmio_write start, inode=%p, fd=%d", inode, (int)inode->fd);

  log_tail = inode->log_tail;
  file_size = inode->i_size;
  offset = file_off & (~PAGE_MASK);
  page_nums = ((cnt + offset - 1) >> PAGE_SHIFT) + 1;
  start_page = file_off & PAGE_MASK;

  while (page_nums > 0) {
    if (page_nums > MAX_NUM_PAGE_PER_ENTRY) {
      alloc_num = MAX_NUM_PAGE_PER_ENTRY;
    } else {
      alloc_num = page_nums;
    }
    mid_num = alloc_num;
    pages = alloc_free_pages(alloc_num);
    if (pages == NULL) {
      // todo
    }
    node2page = get_nvm_node2page_addr(pages);

    wr_size = PAGE_SIZE * alloc_num - offset;
    if (total_size < wr_size) {
      wr_size = total_size;
    }

    if (total_size <= PAGE_SIZE - offset) {
      first_page_wr = total_size;
      last_page_wr = 0;
      --mid_num;
    } else {
      if (offset != 0) {
        first_page_wr = PAGE_SIZE - offset;
        --mid_num;
      } else {
        first_page_wr = 0;
      }
      last_page_wr = (wr_size + offset) & (~PAGE_MASK);
      if (last_page_wr != 0) {
        --mid_num;
      }
    }

    entry = get_log_entry(&log_tail);
    if (entry == NULL) {
      // todo
    }
    entry_off = nvm_addr2off(entry);

    page_addr = node2page;
    // first page
    if (first_page_wr != 0) {
      page_addr_node = get_nvm_page2node_addr(page_addr);
      page_addr_node->log_entry = entry_off;
      memcpy(page_addr + offset, user_buf, first_page_wr);
      user_buf += (PAGE_SIZE - offset);
      if (page_addr_node->next_offset != OFFSET_NULL)
        page_addr = nvm_off2addr(page_addr_node->next_offset);
    }

    // middle pages
    for (i = 0; i < mid_num; ++i) {
      page_addr_node = get_nvm_page2node_addr(page_addr);
      page_addr_node->log_entry = entry_off;
      memcpy(page_addr, user_buf, PAGE_SIZE);
      user_buf += PAGE_SIZE;
      if (page_addr_node->next_offset != OFFSET_NULL)
        page_addr = nvm_off2addr(page_addr_node->next_offset);
    }
    // last page
    if (last_page_wr != 0) {
      page_addr_node = get_nvm_page2node_addr(page_addr);
      page_addr_node->log_entry = entry_off;
      memcpy(page_addr, user_buf, last_page_wr);
      user_buf += last_page_wr;
      if (page_addr_node->next_offset != OFFSET_NULL)
        page_addr = nvm_off2addr(page_addr_node->next_offset);
    }

    // fill first and last page
    if (first_page_wr != 0) {
      part_page_off =
          get_bplustree_node(&inode->bplus_tree, start_page, BPLUSTREE_PAGE);
      if (part_page_off != OFFSET_NULL) {
        part_page = nvm_off2addr(part_page_off);
        memcpy(node2page, part_page, offset);
      }
    }
    if (last_page_wr != 0) {
      part_page_off = get_bplustree_node(
          &inode->bplus_tree, start_page + (alloc_num - 1) * PAGE_SIZE,
          BPLUSTREE_PAGE);
      if (part_page_off != OFFSET_NULL) {
        part_page = nvm_off2addr(part_page_off);
        memcpy(page_addr + last_page_wr, part_page + last_page_wr,
               PAGE_SIZE - last_page_wr);
      }
    }

    file_off += wr_size;
    if (file_off > file_size) {
      file_size = file_off;
    }

    start_write_off = start_page;
    end_write_off = start_write_off + wr_size;
    init_log_entry(entry, pages->offset, file_size, alloc_num, start_write_off,
                   end_write_off);

    page_nums -= alloc_num;
    total_size -= wr_size;
    start_page += alloc_num * PAGE_SIZE;
    offset = 0;
  }

  old_log_tail = inode->log_tail;

  update_inode_tail(inode, log_tail);

  update_radixtree(inode, old_log_tail);

  ATFS_DEBUG("nvmio_write success");
  return cnt;
}

ssize_t nvmio_read(struct atfs_inode *inode, void *buf, size_t cnt,
                   u64 file_off) {
  u64 rd_size = 0;
  u64 offset;
  u64 start_page;
  u64 total_size = cnt;
  u64 part_page_off;
  char *page_addr = NULL;
  char *user_buf = buf;
  char fill_zero[PAGE_SIZE];

  ATFS_DEBUG("nvmio_read start, inode=%p, fd=%d", inode, (int)inode->fd);

  memset(fill_zero, 0, PAGE_SIZE);

  if (file_off + cnt > inode->i_size) {
    ATFS_LOG("nvmio_read oversize. file_off=%llu, file_size=%llu, cnt=%llu",
             file_off, inode->i_size, cnt);
    return -1;
  }

  offset = file_off & (~PAGE_MASK);
  start_page = file_off & PAGE_MASK;

  while (total_size > 0) {
    part_page_off =
        get_bplustree_node(&inode->bplus_tree, start_page, BPLUSTREE_PAGE);
    // to adapt lseek
    if (part_page_off != OFFSET_NULL) {
      page_addr = nvm_off2addr(part_page_off);
    } else {
      page_addr = fill_zero;
    }

    if (offset) {
      if (total_size < PAGE_SIZE - offset) {
        rd_size = total_size;
      } else {
        rd_size = PAGE_SIZE - offset;
      }
    } else if (total_size < PAGE_SIZE) {
      rd_size = total_size;
    } else {
      rd_size = PAGE_SIZE;
    }
    memcpy(user_buf, page_addr + offset, rd_size);

    user_buf += rd_size;
    total_size -= rd_size;
    start_page += PAGE_SIZE;
    offset = 0;
  }

  ATFS_DEBUG("nvmio_read success");

  return cnt;
}