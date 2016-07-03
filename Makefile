LIB=		input

CFLAGS+=	-I${.CURDIR} -I/usr/local/include

INCS= 		libinput.h
SRCS=		libinput.c libinput-util.c dragonfly.c sysmouse.c
PKGCONFIG=	libinput.pc

LINUX_INCS=	input.h

.include <bsd.lib.mk>
