/*	sys/ioc_tty.h - Terminal ioctl() command codes.
 *							Author: Kees J. Bot
 *								23 Nov 2002
 *
 */

#ifndef _S_I_TTY_H
#define _S_I_TTY_H

#include "./ioctl.h"

/* Terminal ioctls. */
#define TCGETS		_IOR('T',  8, struct mnx_termios_s) /* tcgetattr */
#define TCSETS		_IOW('T',  9, struct mnx_termios_s) /* tcsetattr, TCSANOW */
#define TCSETSW		_IOW('T', 10, struct mnx_termios_s) /* tcsetattr, TCSADRAIN */
#define TCSETSF		_IOW('T', 11, struct mnx_termios_s) /* tcsetattr, TCSAFLUSH */
#define TCSBRK		_IOW('T', 12, int)	      /* tcsendbreak */
#define TCDRAIN		_IO ('T', 13)		      /* tcdrain */
#define TCFLOW		_IOW('T', 14, int)	      /* tcflow */
#define TCFLSH		_IOW('T', 15, int)	      /* tcflush */
#define	TIOCGWINSZ	_IOR('T', 16, struct mnx_winsize_s)
#define	TIOCSWINSZ	_IOW('T', 17, struct mnx_winsize_s)
#define	TIOCGPGRP	_IOW('T', 18, int)
#define	TIOCSPGRP	_IOW('T', 19, int)
#define TIOCSFON	_IOW('T', 20, u8_t [8192])

/* Legacy <sgtty.h> */
#define TIOCGETP	_IOR('t',  1, struct sgttyb)
#define TIOCSETP	_IOW('t',  2, struct sgttyb)
#define TIOCGETC	_IOR('t',  3, struct tchars)
#define TIOCSETC	_IOW('t',  4, struct tchars)
#define	TIOCGETCFG  _IOR('t',  5, struct tty_conf_s)	// get VTTY config 
#define	TIOCGETPSE  _IOR('t',  6, struct tty_pseudo_s)	// get VTTY pseudo terminal 

/* Keyboard ioctls. */
#define KIOCBELL        _IOW('k', 1, struct kio_bell)
#define KIOCSLEDS       _IOW('k', 2, struct kio_leds)
#define KIOCSMAP	_IOW('k', 3, keymap_t)

#endif /* _S_I_TTY_H */
