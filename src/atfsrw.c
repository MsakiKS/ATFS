#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "../include/allocator.h"
// #include "../include/hashmap.h"
#include "../include/atfsrw.h"
#include "../include/bplustree.h"
#include "../include/inode.h"
#include "../include/internal.h"
#include "../include/nvmio.h"
#include "../include/super.h"
#include "../include/types.h"

#ifndef NULL
#define NULL 0
#endif

extern bool process_initialized;
extern u64 g_write_times_cnt;
extern u64 g_read_times_cnt;

int atfscreat(const char *filename, mode_t mode) {
  s32 fd;
  struct atfs_super_block *sb = NULL;
  u64 inode_offset;
  struct atfs_inode *inode = NULL;
  ATFS_DEBUG("start");

  if (__glibc_unlikely(!process_initialized)) {
    init_atfs();
  }

  sb = get_superblock();
  fd = hashmap_hash_s32(filename);

  pthread_mutex_lock(&sb->mutex);
  // inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);

  pthread_mutex_unlock(&sb->mutex);
  if (inode_offset != OFFSET_NULL) {
    ATFS_LOG("create, inode exist");
    return EEXIST;
  }

  inode_offset = alloc_atfs_inode();
  if (inode_offset == OFFSET_NULL) {
    ATFS_LOG("create, alloc inode failed");
    return INODE_FAILED;
  }

  inode = nvm_off2addr(inode_offset);
  inode->fd = fd;

  pthread_mutex_lock(&sb->mutex);
  // set_radixtree_node(&sb->hash_root, inode_offset, fd, RADIXTREE_INODE);
  list_add(&inode->l_node, &sb->s_list);
  pthread_mutex_unlock(&sb->mutex);

  ATFS_DEBUG("atfs create success, sb=%p, inode=%p, fd=%d", sb, inode,
             (int)inode->fd);
  return fd;
}

int atfsopen(const char *path, int flags, ...) {
  s32 fd;
  struct atfs_super_block *sb = NULL;
  u64 inode_offset;
  struct atfs_inode *inode = NULL;

  ATMFS_DEBUG("start");

  if (__glibc_unlikely(!process_initialized)) {
    init_atfs();
  }

  sb = get_superblock();
  fd = hashmap_hash_s32(path);
  // while (pthread_rwlock_tryrdlock(&sb->rwlockp) != 0);
  pthread_mutex_lock(&sb->mutex);
  // inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
  pthread_mutex_unlock(&sb->mutex);

  if (inode_offset == OFFSET_NULL) {
    if (!(flags & O_CREAT)) {
      ATFS_LOG("open, inode not exist");
      return EEXIST;
    }
    inode_offset = alloc_atfsfs_inode();
    if (inode_offset == OFFSET_NULL) {
      ATFS_LOG("open, O_CREAT, alloc inode failed");
      return INODE_FAILED;
    }
  }

  inode = nvm_off2addr(inode_offset);
  if (flags & O_CREAT) {
    inode->fd = fd;
    pthread_mutex_lock(&sb->mutex);
    // set_radixtree_node(&sb->hash_root, inode_offset, fd, RADIXTREE_INODE);
    list_add(&inode->l_node, &sb->s_list);
    pthread_mutex_unlock(&sb->mutex);
  }

  ATFS_DEBUG("atfsopen success, sb=%p, inode=%p, fd=%d", sb, inode,
             (int)inode->fd);

  return fd;
}

int atfsclose(int fd) {
  // todo
  return 0;
}

ssize_t atfsread(int fd, void *buf, size_t cnt) {
  struct atfs_super_block *sb = get_superblock();
  u64 inode_offset;
  struct atfs_inode *inode = NULL;
  ssize_t ret;

  ATFS_DEBUG("start");

  pthread_mutex_lock(&sb->mutex);
  // inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
  pthread_mutex_unlock(&sb->mutex);
  if (inode_offset == OFFSET_NULL) {
    ATFS_LOG("read, no such inode, fd=%d", fd);
    return INODE_FAILED;
  }
  inode = nvm_off2addr(inode_offset);

  pthread_mutex_lock(&sb->mutex);
  list_move(&inode->l_node, &sb->s_list);
  pthread_mutex_unlock(&sb->mutex);

  ++g_read_times_cnt;

  pthread_mutex_lock(&inode->mutex);
  ret = nvmio_read(inode, buf, cnt, inode->file_off);
  update_inode_offset(inode, cnt);
  pthread_mutex_unlock(&inode->mutex);

  ATFS_DEBUG("atfs read success, sb=%p, inode=%p, fd=%d, inode->fd=%d", sb,
             inode, fd, (int)inode->fd);

  return ret;
}

ssize_t atfswrite(int fd, const void *buf, size_t cnt) {
  struct atfs_super_block *sb = get_superblock();
  u64 inode_offset;
  struct atfs_inode *inode = NULL;
  ssize_t ret;

  ATFS_DEBUG("start");

  pthread_mutex_lock(&sb->mutex);
  // inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
  pthread_mutex_unlock(&sb->mutex);
  if (inode_offset == OFFSET_NULL) {
    ATFS_LOG("write, no such inode, fd=%d", fd);
    return INODE_FAILED;
  }
  inode = nvm_off2addr(inode_offset);

  pthread_mutex_lock(&sb->mutex);
  list_move(&inode->l_node, &sb->s_list);
  pthread_mutex_unlock(&sb->mutex);
  ++g_write_times_cnt;

  pthread_mutex_lock(&inode->mutex);
  ret = nvmio_write(inode, buf, cnt, inode->file_off);
  update_inode_info(inode, cnt);
  pthread_mutex_unlock(&inode->mutex);

  ATFS_DEBUG("atfs write success, sb=%p, inode=%p, fd=%d, inode->fd=%d", sb,
             inode, fd, (int)inode->fd);

  return ret;
}

off_t atfslseek(int fd, off_t offset, int whence) {
  struct atfs_super_block *sb = get_superblock();
  u64 inode_offset;
  struct atfs_inode *inode = NULL;
  off_t off;

  ATFS_DEBUG("lseek start");

  pthread_mutex_lock(&sb->mutex);
  // inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
  pthread_mutex_unlock(&sb->mutex);
  if (inode_offset == OFFSET_NULL) {
    ATFS_LOG("lseek, no such inode");
    return INODE_FAILED;
  }
  inode = nvm_off2addr(inode_offset);

  pthread_mutex_lock(&inode->mutex);
  switch (whence) {
  case SEEK_SET:
    inode->file_off = offset;
    break;
  case SEEK_CUR:
    inode->file_off += offset;
    break;
  case SEEK_END:
    inode->file_off = inode->i_size;
    inode->file_off += offset;
    break;
  default:
    pthread_mutex_unlock(&inode->mutex);
    return EINVAL;
  }
  off = inode->file_off;
  pthread_mutex_unlock(&inode->mutex);

  ATFS_DEBUG("atfs lseek success, sb=%p, inode=%p, fd=%d", sb, inode,
             (int)inode->fd);

  return off;
}

int atfsftruncate(int fd, off_t length) {
  // todo
  return 0;
}

int atfsfsync(int fd) {
  // todo
  return 0;
}

ssize_t atfspread(int fd, void *buf, size_t cnt, off_t offset) {
  struct atfs_super_block *sb = get_superblock();
  u64 inode_offset;
  struct atfs_inode *inode = NULL;
  ssize_t ret;

  ATFS_DEBUG("pread start");

  pthread_mutex_lock(&sb->mutex);
  // inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
  pthread_mutex_unlock(&sb->mutex);
  if (inode_offset == OFFSET_NULL) {
    UNVMFS_LOG("read, no such inode");
    return INODE_FAILED;
  }
  inode = nvm_off2addr(inode_offset);

  pthread_mutex_lock(&sb->mutex);
  list_move(&inode->l_node, &sb->s_list);
  pthread_mutex_unlock(&sb->mutex);
  ++g_read_times_cnt;

  pthread_mutex_lock(&inode->mutex);
  ret = nvmio_read(inode, buf, cnt, offset);
  pthread_mutex_unlock(&inode->mutex);

  ATFS_DEBUG("atfs pread success, sb=%p, inode=%p, fd=%d, inode->fd=%d", sb,
             inode, fd, (int)inode->fd);

  return ret;
}

ssize_t atfspread64(int fd, void *buf, size_t cnt, off_t offset) {
  return atfspread(fd, buf, cnt, offset);
}

ssize_t atfspwrite(int fd, const void *buf, size_t cnt, off_t offset) {
  struct atfs_super_block *sb = get_superblock();
  u64 inode_offset;
  struct atfs_inode *inode = NULL;
  ssize_t ret;

  ATFS_DEBUG("start");

  pthread_mutex_lock(&sb->mutex);
  // inode_offset = get_radixtree_node(&sb->hash_root, fd, RADIXTREE_INODE);
  pthread_mutex_unlock(&sb->mutex);
  if (inode_offset == OFFSET_NULL) {
    UNVMFS_LOG("write, no such inode");
    return INODE_FAILED;
  }
  inode = nvm_off2addr(inode_offset);

  pthread_mutex_lock(&sb->mutex);
  list_move(&inode->l_node, &sb->s_list);
  pthread_mutex_unlock(&sb->mutex);
  ++g_write_times_cnt;

  pthread_mutex_lock(&inode->mutex);
  ret = nvmio_write(inode, buf, cnt, offset);
  update_inode_size(inode, cnt);
  pthread_mutex_unlock(&inode->mutex);

  ATFS_DEBUG("atfs pwrite success, sb=%p, inode=%p, fd=%d, inode->fd=%d", sb,
             inode, fd, (int)inode->fd);

  return ret;
}

ssize_t atfspwrite64(int fd, const void *buf, size_t cnt, off_t offset) {
  return atfspwrite(fd, buf, cnt, offset);
}

int atfsrename(const char *oldpath, const char *newpath) {
  // todo
  return 0;
}

int atfslink(const char *existing, const char *new) {
  // todo
  return 0;
}

int atfssymlink(const char *existing, const char *new) {
  // todo
  return 0;
}

int atfsunlink(char *pathname) {
  // todo
  return 0;
}

ssize_t atfsreadlink(const char *path, char *buf, size_t buf_size) {
  // todo
  return 0;
}

int atfsmkdir(char *path, int perm) { return (mkdir(path, perm)); }

int atfsrmdir(char *path) { return (rmdir(path)); }

DIR *atfsopendir(char *path) { return (opendir(path)); }

struct dirent *atfsreaddir(DIR *dirp) { return (readdir(dirp)); }

int atfsclosedir(DIR *dirp) { return (closedir(dirp)); }

int atfsstat(char *path, struct stat64 *statbufp) {
  // todo
  return 0;
}

int atfsfstat(int fd, struct stat64 *statbufp) {
  // todo
  return 0;
}

int atfsaccess(const char *path, int amode) {
  // todo
  return 0;
}
