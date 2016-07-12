LIB=		input
SHLIB_MAJOR=	0

WARNS?=	1

PREFIX?=	/usr/local
LIBDIR=		${PREFIX}/lib
INCLUDEDIR=	${PREFIX}/include

CFLAGS+=	-I${.CURDIR}
CFLAGS+=	-I${PREFIX}/include
LDADD+=		-ldevattr -lprop -lm
DPADD+=		${LIBDEVATTR} ${LIBPROP} ${LIBM}
INCS= 		libinput.h
SRCS=		libinput.c libinput-util.c filter.c dragonfly.c
SRCS+=		sysmouse.c keyboard.c kbdev.c

LINUX_INCS=	input.h

MAN=

.include <bsd.lib.mk>
