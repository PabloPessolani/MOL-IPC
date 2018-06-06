#ifndef _MNX_SYS_SELECT_H
#define _MNX_SYS_SELECT_H 1

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE 1
#endif

/* Use this datatype as basic storage unit in mnx_fd_set */
typedef u32_t mnx_fd_mask;	

/* This many bits fit in an mnx_fd_set word. */
#define _FDSETBITSPERWORD	(sizeof(mnx_fd_mask)*8)

/* Bit manipulation macros */
#define _FD_BITMASK(b)	(1L << ((b) % _FDSETBITSPERWORD))
#define _FD_BITWORD(b)	((b)/_FDSETBITSPERWORD)

/* Default FD_SETSIZE is OPEN_MAX. */
#ifndef FD_SETSIZE
#define FD_SETSIZE		OPEN_MAX
#endif

/* We want to store FD_SETSIZE bits. */
#define _FDSETWORDS	((FD_SETSIZE+_FDSETBITSPERWORD-1)/_FDSETBITSPERWORD)

typedef struct {
	mnx_fd_mask	fds_bits[_FDSETWORDS];
} mnx_fd_set;

int mol_select(int nfds, mnx_fd_set *readfds, mnx_fd_set *writefds, mnx_fd_set *errorfds, struct timeval *timeout);

#define MNX_FD_ZERO(s) do { int _i; for(_i = 0; _i < _FDSETWORDS; _i++) { (s)->fds_bits[_i] = 0; } } while(0)
#define MNX_FD_SET(f, s) do { (s)->fds_bits[_FD_BITWORD(f)] |= _FD_BITMASK(f); } while(0)
#define MNX_FD_CLR(f, s) do { (s)->fds_bits[_FD_BITWORD(f)] &= ~(_FD_BITMASK(f)); } while(0)
#define MNX_FD_ISSET(f, s) ((s)->fds_bits[_FD_BITWORD(f)] & _FD_BITMASK(f))

/* possible mol_select() operation types; read, write, errors */
/* (FS/driver internal use only) */
#define SEL_RD		(1 << 0)
#define SEL_WR		(1 << 1)
#define SEL_ERR		(1 << 2)
#define SEL_NOTIFY	(1 << 3) /* not a real mol_select operation */

#endif /* _MNX_SYS_SELECT_H */

