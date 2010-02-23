#if LAB >= 4
// Definitions for input/output via parent/child communication
// See COPYRIGHT for copyright information.
#ifndef PIOS_INC_IO_H
#define PIOS_INC_IO_H


// I/O event numbers
typedef enum iotype {
	IO_NONE = 0,			// Null iotype, means no I/O to do
	IO_CONS,			// Console I/O character data
	IO_DISK,			// Disk I/O request/reply
#if LAB >= 5
	IO_NET,				// Network packet
#endif
} iotype;

#define IOCONS_BUFSIZE	512		// Console buffer size
typedef struct iocons {
	iotype	type;			// I/O event type == IO_CONS
	char	data[IOCONS_BUFSIZE+1];	// Null-terminated character string
} iocons;

#define IODISK_SECSIZE	512		// Disk block size
typedef struct iodisk {
	iotype	type;			// I/O event type == IO_DISK
	bool	write;			// 0 = read, 1 = write
	int	secno;			// Disk sector number to read or write
	char	data[IODISK_SECSIZE];	// Disk sector contents
} iodisk;
                                        
#if LAB >= 5                            
typedef struct ionet {                  
	iotype	type;			// I/O event type == IO_NET
} ionet;
                                        
#endif	// LAB >= 5                     
typedef union ioevent {                 
	iotype	type;			// I/O event type from enum above
	iocons	cons;			// Console I/O event
	iodisk	disk;			// Disk I/O event
#if LAB >= 5                            
	ionet	net;			// Network I/O event
#endif
} ioevent;


#endif /* !PIOS_INC_IO_H */
#endif // LAB >= 4
