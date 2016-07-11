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
#include "filter.h"
#include "libinput-private.h"

int sysmouse_init_accel(struct libinput_device *device,
			enum libinput_config_accel_profile which);

static int
sysmouse_accel_config_available(struct libinput_device *device)
{
	return 1;
}

static enum libinput_config_status
sysmouse_accel_config_set_speed(struct libinput_device *device, double speed)
{
	if (!filter_set_speed(device->filter, speed))
		return LIBINPUT_CONFIG_STATUS_INVALID;

	return LIBINPUT_CONFIG_STATUS_SUCCESS;
}

static double
sysmouse_accel_config_get_speed(struct libinput_device *device)
{
	return filter_get_speed(device->filter);
}

static double
sysmouse_accel_config_get_default_speed(struct libinput_device *device)
{
	return 0.0;
}

static uint32_t
sysmouse_accel_config_get_profiles(struct libinput_device *device)
{
	if (!device->filter)
		return LIBINPUT_CONFIG_ACCEL_PROFILE_NONE;

	return LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE |
		LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
}

static enum libinput_config_status
sysmouse_accel_config_set_profile(struct libinput_device *device,
				  enum libinput_config_accel_profile profile)
{
	struct motion_filter *filter;
	double speed;

	filter = device->filter;
	if (filter_get_type(filter) == profile)
		return LIBINPUT_CONFIG_STATUS_SUCCESS;

	speed = filter_get_speed(filter);
	device->filter = NULL;

	if (sysmouse_init_accel(device, profile) == 0) {
		sysmouse_accel_config_set_speed(device, speed);
		filter_destroy(filter);
	} else {
		device->filter = filter;
	}

	return LIBINPUT_CONFIG_STATUS_SUCCESS;
}

static enum libinput_config_accel_profile
sysmouse_accel_config_get_profile(struct libinput_device *device)
{
	return filter_get_type(device->filter);
}

static enum libinput_config_accel_profile
sysmouse_accel_config_get_default_profile(struct libinput_device *device)
{
	if (!device->filter)
		return LIBINPUT_CONFIG_ACCEL_PROFILE_NONE;

	/* No device has a flat profile as default */
	return LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
}

static struct libinput_device_config_accel sysmouse_accel = {
	&sysmouse_accel_config_available,
	&sysmouse_accel_config_set_speed,
	&sysmouse_accel_config_get_speed,
	&sysmouse_accel_config_get_default_speed,
	&sysmouse_accel_config_get_profiles,
	&sysmouse_accel_config_set_profile,
	&sysmouse_accel_config_get_profile,
	&sysmouse_accel_config_get_default_profile
};

static int
sysmouse_device_init_pointer_acceleration(struct libinput_device *device,
	struct motion_filter *filter)
{
	device->filter = filter;

	if (device->config.accel == NULL) {
		device->config.accel = &sysmouse_accel;
		sysmouse_accel_config_set_speed(device,
		    sysmouse_accel_config_get_default_speed(device));
	}

	return 0;
}

int
sysmouse_init_accel(struct libinput_device *device,
	enum libinput_config_accel_profile which)
{
	struct motion_filter *filter;

	/* Just use some constant dpi value for sysmouse */
	if (which == LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT)
		filter = create_pointer_accelerator_filter_flat(100);
	else
		filter = create_pointer_accelerator_filter_linear(100);

	if (!filter)
		return -1;

	return sysmouse_device_init_pointer_acceleration(device, filter);
}

static void
sysmouse_process(struct libinput_device *device, char *pkt)
{
	enum libinput_button_state state;
	struct normalized_coords unaccel, accel;
	struct discrete_coords disc;
	struct device_float_coords raw;
	struct timespec ts;
	uint64_t time;
	int button;
	int xdelta, ydelta, zdelta;
	int nm;

	if ((pkt[0] & 0x80) == 0 || (pkt[7] & 0x80) != 0)
		return;

	xdelta = pkt[1] + pkt[3];
	ydelta = pkt[2] + pkt[4];
	ydelta = -ydelta;
	zdelta = ((char)(pkt[5] << 1) + (char)(pkt[6] << 1)) >> 1;

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
		memset(&unaccel, 0, sizeof(unaccel));
		memset(&accel, 0, sizeof(accel));

		unaccel.x = xdelta;
		unaccel.y = ydelta;

		if (device->filter) {
			/* Apply pointer acceleration. */
			accel = filter_dispatch(device->filter,
						&unaccel,
						device,
						time);
		} else {
#if 0
			log_bug_libinput(libinput,
					 "%s: accel filter missing\n",
					 device->devname);
#endif
			accel = unaccel;
		}

		if (!normalized_is_zero(accel) || !normalized_is_zero(unaccel))
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
        for (i = 0; i < count; i++)
		sysmouse_process(device, &pkts[i*8]);
}
