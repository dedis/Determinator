#if LAB >= 4
// User-space Unix API emulation state definitions.
// In PIOS, Unix-style I/O is implemented mostly in user space,
// with files kept within each process's own address space
// in the virtual address range 2GB-3GB (0x80000000-0xc0000000).
// Each file's contents occupies a 4MB area of this address region
// (regardless of the file's actual size), for up to 256 files total.
// The first 4MB region, for "file 0", is reserved for the Unix state area.
#ifndef PIOS_INC_UNIX_H
#define PIOS_INC_UNIX_H 1

#include <inc/types.h>


#define UNIX_INODES	256		// Max number of Unix files or "inodes"

#define UNIXSTATEVA	0x80000000	// Virtual address of Unix state area
#define UNIXFILEVA(fn)	(UNIXSTATEVA + ((fn) << 22))	// VA of a Unix file

struct unixdev;

typedef struct unixfd {			// Open Unix file descriptor
	struct unixdev *dev;		// Handling device, NULL if fd unused
	ino_t	ino;			// Opened inode number
	off_t	ofs;			// Current seek pointer in file
	int	omode;			// File open mode (see fcntl.h)
} unixfd;

struct stat;

typedef struct unixdev {	// Unix "device" - really an fd class
	void	(*ref)(unixfd *fd, int adjust);
				// Add or drop reference, close if none left
	ssize_t	(*read)(unixfd *fd, void *buf, size_t len, off_t ofs);
	ssize_t	(*write)(unixfd *fd, const void *buf, size_t len, off_t ofs);
	int 	(*stat)(unixfd *fd, struct stat *st);
	int	(*seek)(unixfd *fd, off_t pos, int whence);
	int	(*trunc)(unixfd *fd, off_t length);
} unixdev;

typedef struct unixinode {		// Unix file info - like an "inode"
	size_t	size;			// File's current size
	int	refcount;		// Number of outstanding references
	int	ver;			// Version number in this process
	int	parentver;		// Parent's version number, -1 if none
} unixfileinfo;

typedef struct unixstate {
	int		err;		// This process/thread's errno variable
	int		status;		// Exit status - set on exit
	unixfd		fd[OPEN_MAX];	// Open file descriptor table
	unixinode	ino[UNIX_INODES]; // "Inodes" describing actual files
} unixstate;

#define unixstate	((unixstate *) UNIXSTATEVA)

#define UNIX_ROOTDIR	1		// File/inode 1 is always the root dir


// Unix "device numbers"
enum {
	UNIXDEV_CONS,			// Kernel console
	UNIXDEV_FILE,			// User-space file
#if LAB >= 99
	UNIXDEV_SOCKET,			// Network socket
#endif
	UNIXDEV_MAX			// Number of devices
};

extern unixdev unix_consdev;		// Console device
extern unixdev unix_filedev;		// User-space file system
#if LAB >= 99
extern unixdev unix_sockdev;		// Network interface device
#endif

extern unixdev *unixdevs[UNIXDEV_MAX];	// Table listing all "devices"


#define unixfd_valid(ufd) \
	((ufd) >= &unixstate->fd && (ufd) < &unixstate->fd[OPEN_MAX])
#define unixfd_open(ufd) \
	(unixfd_valid(ufd) && (ufd)->dev != NULL)


int unixfd_alloc(void);		// Allocate a file descriptor
unixfd *unixfd_lookup(int fd)	// Lookup a file descriptor

#endif	// !PIOS_INC_UNIX_H
#endif	// LAB >= 4
