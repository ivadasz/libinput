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
keyboard_process(struct libinput_device *device, uint8_t byte)
{
	enum libinput_key_state state;
	struct timespec ts;
	uint64_t time;
	int key;

	/* XXX */
}

void
keyboard_device_dispatch(void *data)
{
	struct libinput_device *device = data;
	uint8_t buf[128];
	ssize_t len;
	int i;

	len = read(device->fd, buf, sizeof(buf));
	if (len <= 0)
		return;

        for (i = 0; i < len; i++)
		keyboard_process(device, buf[i]);
}
