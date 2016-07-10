/*
 * Copyright © 2015 Martin Pieuchot <mpi@openbsd.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

#include <sys/mouse.h>

#include "libinput.h"
#include "libinput-util.h"
#include "libinput-private.h"

static void
sysmouse_process(struct libinput_device *device, char *pkt)
{
	enum libinput_button_state state;
	struct normalized_coords accel;
	struct discrete_coords disc;
	struct device_float_coords raw;
	struct timespec ts;
	uint64_t time;
	int button, key;
	int xdelta, ydelta, zdelta;
	int nm;

	if ((pkt[0] & 0x80) == 0 || (pkt[7] & 0x80) != 0)
		return;

	xdelta = pkt[1] + pkt[3];
	ydelta = pkt[2] + pkt[4];
	ydelta = -ydelta;
	zdelta = (pkt[5] > 0 && pkt[6] == 0) ?
	    (char)(pkt[5] | 0x80) :
	    pkt[5] + pkt[6];

	clock_gettime(CLOCK_MONOTONIC, &ts);
	time = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

	if (zdelta != 0) {
		memset(&disc, 0, sizeof(disc));
		memset(&accel, 0, sizeof(accel));

		accel.y = zdelta;
		disc.y = zdelta;

		pointer_notify_axis(device, time,
		    AS_MASK(LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL),
		    LIBINPUT_POINTER_AXIS_SOURCE_WHEEL, &accel, &disc);
	}

	if (xdelta != 0 || ydelta != 0) {
		memset(&raw, 0, sizeof(raw));
		memset(&accel, 0, sizeof(accel));

		accel.x = xdelta;
		accel.y = ydelta;

		pointer_notify_motion(device, time, &accel, &raw);
	}

	nm = pkt[0] & 7;
	if (nm != device->sysmouse_oldmask) {
		if ((nm & 4) != (device->sysmouse_oldmask & 4)) {
			pointer_notify_button(device, time, BTN_LEFT,
			    (nm & 4) ? LIBINPUT_BUTTON_STATE_RELEASED
				     : LIBINPUT_BUTTON_STATE_PRESSED);
		}
		if ((nm & 2) != (device->sysmouse_oldmask & 2)) {
			pointer_notify_button(device, time, BTN_MIDDLE,
			    (nm & 2) ? LIBINPUT_BUTTON_STATE_RELEASED
				     : LIBINPUT_BUTTON_STATE_PRESSED);
		}
		if ((nm & 1) != (device->sysmouse_oldmask & 1)) {
			pointer_notify_button(device, time, BTN_RIGHT,
			    (nm & 1) ? LIBINPUT_BUTTON_STATE_RELEASED
				     : LIBINPUT_BUTTON_STATE_PRESSED);
		}
		device->sysmouse_oldmask = nm;
	}
}

void
sysmouse_device_dispatch(void *data)
{
	struct libinput_device *device = data;
	uint8_t pkts[128];
	ssize_t len;
	int count, i;

	len = read(device->fd, pkts, sizeof(pkts));
	if (len <= 0 || (len % 8) != 0)
		return;

	count = len / 8;
        for (i = 0; i < count; i++) {
		sysmouse_process(device, &pkts[i*8]);
	}
}
