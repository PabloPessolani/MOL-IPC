#ifndef _MNX_SELECT_H
#define _MNX_SELECT_H 1

//#ifndef _POSIX_SOURCE
//#define _POSIX_SOURCE 1
//#endif

/* Use this datatype as basic storage unit in mol_fd_set */
typedef u32_t mol_fd_mask;	

/* This many bits fit in an mol_fd_set word. */
#define _FDSETBITSPERWORD	(sizeof(mol_fd_mask)*8)

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
	mol_fd_mask	mol_fds_bits[_FDSETWORDS];
} mol_fd_set;
//_PROTOTYPE( int select, (int nfds, mol_fd_set *readfds, mol_fd_set *writefds, mol_fd_set *errorfds, struct timeval *timeout) );

#define MOL_FD_ZERO(s) do { int _i; for(_i = 0; _i < _FDSETWORDS; _i++) { (s)->mol_fds_bits[_i] = 0; } } while(0)
#define MOL_FD_SET(f, s) do { (s)->mol_fds_bits[_FD_BITWORD(f)] |= _FD_BITMASK(f); } while(0)
#define MOL_FD_CLR(f, s) do { (s)->mol_fds_bits[_FD_BITWORD(f)] &= ~(_FD_BITMASK(f)); } while(0)
#define MOL_FD_ISSET(f, s) ((s)->mol_fds_bits[_FD_BITWORD(f)] & _FD_BITMASK(f))

/* possible select() operation types; read, write, errors */
/* (FS/driver internal use only) */
#define MOL_SEL_RD		(1 << 0)
#define MOL_SEL_WR		(1 << 1)
#define MOL_SEL_ERR		(1 << 2)
#define MOL_SEL_NOTIFY	(1 << 3) /* not a real select operation */

int mol_select(int nfds, mol_fd_set *readfds, mol_fd_set *writefds, mol_fd_set *errorfds, struct timeval *timeout);

#endif /* _MNX_SELECT_H */

