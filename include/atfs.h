/**
 *@file atfs.h
 * @author Delong (1158112005@qq.com)
 * @brief 文件系统头文件
 * @version 0.1
 * @date 2023-02-08
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef _ATFS_H
#define _ATFS_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

#include "../src/atfsrw.h"

// 初始化
extern void init_ATFS(void);

// 创建
extern int atfscreat(const char *filename, mode_t mode);
#define creat(filename, mode) atfscreat(filename, mode)

// 打开文件
extern int atfsopen(const char *Path, int flags, ...);
#define open(...) atfsopen(__VA_ARGS__)
#define open64(...) atfsopen(__VA_ARGS__)

// 关闭文件
extern int atfsclose(int fd);
#define close(fd) atfsclose(fd)

// 读取文件
extern ssize_t atfsread(int fd, void *buf, size_t cnt);
#define read(fd, buf, cnt) atfsread(fd, buf, cnt)

// 写入文件
extern ssize_t atfswrite(int fd, const void *buf, size_t cnt);
#define write(fd, buf, cnt) atfswrite(fd, buf, cnt)

// 文件重定位
extern off_t atfslseek(int fd, off_t offset, int whence);
#define lseek(fd, offset, whence) atfslseek(fd, offset, whence)
#define lseek64(fd, offset, whence) atfslseek(fd, offset, whence)
extern int atfsftruncate(int fd, off_t length);

// 文件的清空截断
#define truncate(fd, length) atfsftruncate(fd, length)
#define truncate64(fd, length) atfsftruncate(fd, length)
#define ftruncate(fd, length) atfsftruncate(fd, length)
#define ftruncate64(fd, length) atfsftruncate(fd, length)

// 文件同步
extern int ATFSsync(int fd);
#define fsync(fd) ATFSsync(fd)

// 原子操作
// extern int atfsfreemem(int fdfd, off64_t size);

// 带偏移量地原子的从文件中读取数据
extern int atfspread(int fd, void *buf, size_t cnt, off_t offset);
#define pread(fd, buf, cnt, offset) atfspread(fd, buf, cnt, offset)
extern int atfspread64(int fd, void *buf, size_t cnt, off_t offset);
#define pread64(fd, buf, cnt, offset) atfspread64(fd, buf, cnt, offset)

// 原子写
extern int atfspwrite(int fd, const void *buf, size_t cnt, off_t offset);
#define pwrite(fd, buf, cnt, offset) atfspwrite(fd, buf, cnt, offset)
extern int atfspwrite64(int fd, const void *buf, size_t cnt, off_t offset);
#define pwrite64(fd, buf, cnt, offset) atfspwrite(fd, buf, cnt, offset)

// 重命名
extern int atfsrename(const char *oldpath, const char *newpath);
#define rename(oldpath, newpath) atfsrename(oldpath, newpath)

// 创建软硬链接
extern int atfslink(const char *existing, const char *new);
#define link(existing, new) atfslink(existing, new)
extern int atfssymlink(const char *existing, const char *new);
#define symlink(existing, new) atfssymlink(existing, new)
extern int atfsunlink(char *pathname);

// 卸载链接
#define unlink(pathname) atfsunlink(pathname)
extern ssize_t atfsreadlink(const char *path, char *buf, size_t buf_size);
#define readlink(path, buf, buf_size) atfsreadlink(path, buf, buf_size)

// mkdir
extern int atfsmkdir(char *path, int perm);
#define mkdir(path, perm) atfsmkdir(path, perm)

// rmdir
extern int atfsrmdir(char *path);
#define rmdir(path) atfsrmdir(path)

// opendir
extern DIR *atfsopendir(char *path);
#define atfsopendir(path) atfsopendir(path)

// readdir
extern struct dirent *atfsreaddir(DIR *dirp);
#define readdir(dirp) atfsreaddir(dirp)

// closedir
extern int atfsclosedir(DIR *dirp);
#define closedir(dirp) atfsclosedir(dirp)

// stat命令用于显示文件或文件系统的详细信息
extern int atfsstat(char *path, struct stat *statbufp);
#define stat(path, statbufp) atfsstat(path, statbufp)
#define stat64(path, statbufp) atfsstat(path, statbufp)
extern int ATFSfstat(int fd, struct stat *statbufp);
#define fstat(fd, statbufp) ATFSfstat(fd, statbufp)
#define fstat64(fd, statbufp) ATFSfstat(fd, statbufp)

// 权限管理
extern int atfsaccess(const char *path, int amode);
#define access(path, amode) atfsaccess(path, amode)

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _ATFS_H
