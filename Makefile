LIB=		input
SHLIB_MAJOR=	0

WARNS?=	1

PREFIX?=	/usr/local
LIBDIR=		${PREFIX}/lib
INCLUDEDIR=	${PREFIX}/include
MANDIR=		${PREFIX}/man/man

CFLAGS+=	-I${.CURDIR}
CFLAGS+=	-I${PREFIX}/include
INCS= 		libinput.h
SRCS=		libinput.c libinput-util.c dragonfly.c sysmouse.c
PKGCONFIG=	libinput.pc

LINUX_INCS=	input.h

MAN=

.include <bsd.lib.mk>
