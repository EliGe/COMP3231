/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#include <synch.h>
#include <syscall.h>


#define FILE_CLOSED -1
/*
 * Put your function declarations and data types here ...
 */


struct fd_table
{
	int fd_entries[OPEN_MAX];	/* array of of_t entries */
};

/* global open file table entry */
struct open_file
{
	struct vnode *vn;	/* the vnode this file represents */
	int am;			/* the access mode of this file */
	int rc;			/* the reference count of this file */
	off_t os;		/* read offset within the file */
};

/* global open file table */
struct file_table
{
	struct lock *oft_l;	/* open file table lock */
	struct open_file *openfiles[OPEN_MAX];	/* array of open files */
};

/* global open file table */
struct file_table *of_t;

int file_open(char *filename, int flags, mode_t mode, int *fd_ret);
int file_close(int fd);
int file_table_init(const char *stdin_path, const char *stdout_path,
		    const char *stderr_path);
int sys_open(userptr_t filename, int flags, mode_t mode, int *fd_ret);
int sys_close(int fd);
int sys_write(int fd, userptr_t buf, size_t nbytes, int *sz);
int sys_read(int fd, userptr_t buf, size_t buflen, int *sz);
int file_write(int fd, userptr_t buf, size_t nbytes, int *sz);
int file_read(int fd, userptr_t buf, size_t buflen, int *sz);

#endif /* _FILE_H_ */
