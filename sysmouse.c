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
sysmouse_process(struct libinput_device *device, struct wscons_event *wsevent)
{
	enum libinput_button_state state;
	struct normalized_coords accel;
	struct device_float_coords raw;
	uint64_t time;
	int button, key;

	time = s2us(wsevent->time.tv_sec) + ns2us(wsevent->time.tv_nsec);

	switch (wsevent->type) {
	case WSCONS_EVENT_KEY_UP:
	case WSCONS_EVENT_KEY_DOWN:
		key = wsevent->value;
		if (wsevent->type == WSCONS_EVENT_KEY_UP)
			state = LIBINPUT_KEY_STATE_RELEASED;
		else
			state = LIBINPUT_KEY_STATE_PRESSED;
		keyboard_notify_key(device, time, key, state);
		break;

	case WSCONS_EVENT_MOUSE_UP:
	case WSCONS_EVENT_MOUSE_DOWN:
		/*
		 * Do not return wscons(4) values directly because
		 * the left button value being 0 it will be
		 * interpreted as an error.
		 */
		button = wsevent->value + 1;
		if (wsevent->type == WSCONS_EVENT_MOUSE_UP)
			state = LIBINPUT_BUTTON_STATE_RELEASED;
		else
			state = LIBINPUT_BUTTON_STATE_PRESSED;
		pointer_notify_button(device, time, button, state);
		break;

	case WSCONS_EVENT_MOUSE_DELTA_X:
	case WSCONS_EVENT_MOUSE_DELTA_Y:
		memset(&raw, 0, sizeof(raw));
		memset(&accel, 0, sizeof(accel));

		if (wsevent->type == WSCONS_EVENT_MOUSE_DELTA_X)
			accel.x = wsevent->value;
		else
			accel.y = -wsevent->value;

		pointer_notify_motion(device, time, &accel, &raw);
		break;

	case WSCONS_EVENT_MOUSE_ABSOLUTE_X:
	case WSCONS_EVENT_MOUSE_ABSOLUTE_Y:
		//return LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE;

	case WSCONS_EVENT_SYNC:
		break;

	case WSCONS_EVENT_MOUSE_ABSOLUTE_Z:
	case WSCONS_EVENT_MOUSE_ABSOLUTE_W:
		/* ignore those */
	default:
		assert(1 == 0);
	}
}

void
sysmouse_device_dispatch(void *data)
{
	struct libinput_device *device = data;
	struct wscons_event wsevents[32];
	ssize_t len;
	int count, i;

	len = read(device->fd, wsevents, sizeof(struct wscons_event));
	if (len <= 0 || (len % sizeof(struct wscons_event)) != 0)
		return;

	count = len / sizeof(struct wscons_event);
        for (i = 0; i < count; i++) {
		wscons_process(device, &wsevents[i]);
	}
}
