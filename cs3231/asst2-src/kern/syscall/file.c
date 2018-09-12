#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>
#include <proc.h>

/*
 * Add your file-related functions here ...
 */

//when there is a pointer, we need to use copy in and out;


/*
	file descripter is an int or an index into the process file table
	every index in the file table refers to a file handle object maintained by the kernel:
		-> file handles store the current file offset, or the position in the file that the next read will come from or write will go to.
	file handle object contains a reference to a seperate file object maintained by the kernel: 
		-> file object are mapped by the file system to blocks on disk

	file des -> file handle
	file handle-> file object
	file obj -> blocks


	file descriptor: 
		-> integer: an index to file table
	file handle :
		-> struct: process-wide file meta information 
	file object :
		-> Vnode: system-wide file meta information



	FUNCTIONS HELPFULL:
	VOP_READ VOP_WRITE VOP_LSEEK

	VFS_OPEN  VFS_CLOSE

	
	TASK:
	design a file table

		-- a data structure that maps a file descriptor (integer) to file handle (struct)
		
		initialize the console file using vfs_open
		
		**file name should be "con:"
		**flag should be
			-- 0 is for stdin(read only);
			-- 1 is for stdout(write only); 
			-- 2 is for stderr(write only);
				



	design a file handle

		(VOP_read/write)
		-- offset 
		-- read only or read-write
		--reference counter
		--using locks to do the concurrent (only one procssor can read
		or write an a time)

	check if the passed arguments are valid
		--check flag
		--check error return 
		--check fd
		--use copyin and  out with user pointer to detect violations


		
	
	sys_open:
		-> check if the filename is valid: since it is a pointer, 
			using copy in 
		-> check the flag is valid (e.g. O_RDONLY|OWRONLY is not valid)

	sys_close:
		-> check if fd
		-> how to recycle the fd
		-> when it is safe to delete the filehandle

	sys_read/write:
		-> check if the fd is valid
		-> check the buffer pointer is valid 
		-> can the user read/write
		-> how to read/write 
			read and unterstand kern/include/uio.h
				-> iovec is the buffer
				-> offset is the file handle offset
				-> resid is the buffer length
				-> rw-> UIOREAD/UIOWRITE
				-> segfile is the userspace
	
	sys_lseek(change the location):
		-> one of the argument and the return value are 64-bits
		-> check if the fd is valid
		-> how to calculate the new offset

	sys_dup2(make the new fd point to the olde fd):
		-> if the new fd already point to an open file,
			we need to close that first 
		-> check the fd valid or not



*/


int
file_table_init(const char *stdin_path, const char *stdout_path,
		const char *stderr_path)
{
	int i, fd, result;
	char path[PATH_MAX];

	/* ---- initialising global open file table, do not free --- */
	/* if there is no open file table created yet, create it */
	if (of_t == NULL) {
		of_t = kmalloc(sizeof(struct file_table));
		if (of_t == NULL) {
			return ENOMEM;
		}

		/* create the lock for the open file table */
		struct lock *oft_lk = lock_create("of_table_lock");
		if (oft_lk == NULL) {
			return ENOMEM;
		}

		/* assign the lock to the table */
		of_t->oft_l = oft_lk;

		/* empty the table */
		for (i = 0; i < OPEN_MAX; i++) {
			of_t->openfiles[i] = NULL;
		}

	}
	/* ---- end of global file table initialisation ------------ */


	/* if there is no file descriptor table for proc - make one! */
	curproc->fd_t = kmalloc(sizeof(struct fd_table));
	if (curproc->fd_t == NULL) {
		return ENOMEM;
	}

	/* empty the new table */
	for (i = 0; i < OPEN_MAX; i++) {
		curproc->fd_t->fd_entries[i] = FILE_CLOSED;
	}

	/* create stdin file desc */
	strcpy(path, stdin_path);
	result = file_open(path, O_RDONLY, 0, &fd);
	if (result) {
		kfree(curproc->fd_t);
		return result;
	}

	/* create stdout file desc */
	strcpy(path, stdout_path);
	result = file_open(path, O_WRONLY, 0, &fd);
	if (result) {
		kfree(curproc->fd_t);
		return result;
	}

	/* create stderror file desc */
	strcpy(path, stderr_path);
	result = file_open(path, O_WRONLY, 0, &fd);
	if (result) {
		kfree(curproc->fd_t);
		return result;
	}

	/* ensure that stdin is closed to begin with */
	//file_close(0);

	return 0;
}

int
sys_open(userptr_t filename, int flags, mode_t mode, int *fd_ret)
{
	char fn[PATH_MAX];
	int result;

	/* copy file name from user to kernel space */
	result = copyinstr(filename, fn, PATH_MAX, NULL);
	if (result) {
		return result;
	}

	/* call system function to open file */
	return file_open(fn, flags, mode, fd_ret);
}

int
sys_close(int fd)
{
	return file_close(fd);
}


/*
 * Write system call: write to a file.
 */
int sys_write(int fd, userptr_t buf, size_t nbytes, int *sz)
{
	/* check to see if the file descriptor is sensible */
	if (fd < 0 || fd >= OPEN_MAX) {
		return EBADF;
	}

	/* call system function to write to a file */
	return file_write(fd, buf, nbytes, sz);
}


/*
 * Read system call: read from a file.
 * this syscall will read buflen bytes from the fd file descriptor into the
 * userland buffer buf and return the number of bytes read in sz.
 */
int sys_read(int fd, userptr_t buf, size_t buflen, int *sz)
{
	/* check to see if the file descriptor is sensible */
	if (fd < 0 || fd >= OPEN_MAX) {
		return EBADF;
	}

	return file_read(fd, buf, buflen, sz);
}



int
file_read(int fd, userptr_t buf, size_t buflen, int *sz)
{
	int result;
	struct iovec iovec_tmp;
	struct uio uio_tmp;

	/* get the desired file and ensure it is actually open */
	int of_entry = curproc->fd_t->fd_entries[fd];
	if (of_entry < 0 || of_entry >= OPEN_MAX) {
		return EBADF;
	}

	/* lock the lock for concurrency support */
	lock_acquire(of_t->oft_l);

	struct open_file *of = of_t->openfiles[of_entry];
	if (of == NULL) {
		lock_release(of_t->oft_l);
		return EBADF;
	}

	/* see if the fd can be read */
	if ((of->am & O_ACCMODE) == O_WRONLY) {
		lock_release(of_t->oft_l);
		return EBADF;
	}

	/* get open file vnode for reading from */
	struct vnode *vn = of->vn;

	/* initialize a uio with the read flag set, pointing into our buffer */
	uio_uinit(&iovec_tmp, &uio_tmp, buf, buflen, of->os, UIO_READ);

	/* read from vnode into our uio object */
	result = VOP_READ(vn, &uio_tmp);
	if (result) {
		lock_release(of_t->oft_l);
		return result;
	}

	/* set the amount of bytes read */
	*sz = uio_tmp.uio_offset - of->os;

	/* update the seek pointer in the open file */
	of->os = uio_tmp.uio_offset;

	/* release the lock of the file because we are done */
	lock_release(of_t->oft_l);

	return 0;
}

/*
 * file_write
 * write to a file with the provided file descriptor
 */
int
file_write(int fd, userptr_t buf, size_t nbytes, int *sz)
{
	int result;
	struct iovec iovec_tmp;
	struct uio uio_tmp;

	/* get the desired file and ensure it is actually open */
	int of_entry = curproc->fd_t->fd_entries[fd];
	if (of_entry < 0 || of_entry >= OPEN_MAX) {
		return EBADF;
	}

	/* acquire lock on the open file table */
	lock_acquire(of_t->oft_l);

	struct open_file *of = of_t->openfiles[of_entry];
	if (of == NULL) {
		lock_release(of_t->oft_l);
		return EBADF;
	}

	/* see if the fd can be written to */
	if ((of->am & O_ACCMODE) == O_RDONLY) {
		lock_release(of_t->oft_l);
		return EBADF;
	}

	/* get open file vnode for writing to */
	struct vnode *vn = of->vn;

	/* initialize a uio with the write flag set, pointing into our buffer */
	uio_uinit(&iovec_tmp, &uio_tmp, buf, nbytes, of->os, UIO_WRITE);

	/* write into vnode from our uio object */
	result = VOP_WRITE(vn, &uio_tmp);
	if (result) {
		lock_release(of_t->oft_l);
		return result;
	}

	/* find the number of bytes written */
	*sz = uio_tmp.uio_offset - of->os;

	/* update the seek pointer in the open file */
	of->os = uio_tmp.uio_offset;

	/* release the lock on the file */
	lock_release(of_t->oft_l);

	return 0;
}



int
file_close(int fd)
{
	/* check to see if fd is legit */
	if (fd < 0 || fd >= OPEN_MAX) {
		return EBADF;
	}

	/* check to see the open file index is legit */
	int of_entry = curproc->fd_t->fd_entries[fd];
	if (of_entry < 0 || of_entry >= OPEN_MAX) {
		return EBADF;
	}

	/* flag for being called from another function */
	int sysCalled = 1;

	/* get exclusive access to the oft */
	if (!lock_do_i_hold(of_t->oft_l)) {
		lock_acquire(of_t->oft_l);
		sysCalled = 0;	/* not called from a syscall */
	}

	/* get the file pointer */
	struct open_file *of = of_t->openfiles[of_entry];
	if (of == NULL) {
		lock_release(of_t->oft_l);
		return EBADF;
	}

	/* close the file for this process */
	curproc->fd_t->fd_entries[fd] = FILE_CLOSED;

	/* this is the last reference to the file */
	if (of->rc == 1) {
		/* free memory */
		vfs_close(of->vn);
		kfree(of);
		of_t->openfiles[of_entry] = NULL;
	}
	else {
		of->rc = of->rc - 1;
	}

	/* release exclusive access to the fd table */
	if (!sysCalled) {
		lock_release(of_t->oft_l);
	}

	return 0;
}


int
file_open(char *filename, int flags, mode_t mode, int *fd_ret)
{
	int i, result, fd = -1, of = -1;
	struct vnode *vn;

	/* open the vnode */
	result = vfs_open(filename, flags, mode, &vn);
	if (result) {
		return result;
	}

	/* get this process' file descriptor table */
	struct fd_table *fd_t = curproc->fd_t;

	/* lock the open file table for the proc */
	lock_acquire(of_t->oft_l);

	/* find the next available file descriptor in process table */
	for (i = 0; i < OPEN_MAX; i++) {
		if (fd_t->fd_entries[i] == FILE_CLOSED) {
			fd = i;
			break;
		}
	}

	/* find the next available spot in global open file table */
	for (i = 0; i < OPEN_MAX; i++) {
		if (of_t->openfiles[i] == NULL) {
			of = i;
			break;
		}
	}

	/* file descriptor table and/or open file table is full */
	if (fd == -1 || of == -1) {
		vfs_close(vn);
		lock_release(of_t->oft_l);
		return EMFILE;
	}

	/* create new file record */
	struct open_file *of_entry = kmalloc(sizeof(struct open_file));
	if (of_entry == NULL) {
		vfs_close(vn);
		lock_release(of_t->oft_l);
		return ENOMEM;
	}

	/* initialise file descriptor entry */
	fd_t->fd_entries[fd] = of;

	/* make all the assignments to the file */
	of_entry->vn = vn;	/* assign vnode */
	of_entry->rc = 1;	/* refcount starts as 1 */
	of_entry->am = flags;	/* assign access mode */
	of_entry->os = 0;	/* inital offset is 0 */
	of_t->openfiles[of] = of_entry;

	/* unlock the open file table */
	lock_release(of_t->oft_l);

	/* assign the return value */
	*fd_ret = fd;

	return 0;
}