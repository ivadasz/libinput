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

#include "kbdev.h"
#include "libinput.h"
#include "libinput-util.h"
#include "libinput-private.h"

void
keyboard_device_dispatch(void *data)
{
	struct libinput_device *device = data;
	struct kbdev_event evs[64];
	struct timespec ts;
	uint64_t time;
	int i, n;

	n = kbdev_read_events(device->kbdst, evs, 64);
	if (n <= 0)
		return;

        clock_gettime(CLOCK_MONOTONIC, &ts);
        time = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

	for (i = 0; i < n; i++) {
		keyboard_notify_key(device, time, evs[i].keycode,
		    evs[i].pressed ? LIBINPUT_KEY_STATE_PRESSED
				   : LIBINPUT_KEY_STATE_RELEASED);
	}
}
