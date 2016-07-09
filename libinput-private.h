/*
 * Copyright © 2013 Jonas Ådahl
 * Copyright © 2013-2015 Red Hat, Inc.
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

#ifndef LIBINPUT_PRIVATE_H
#define LIBINPUT_PRIVATE_H

#include <linux/input.h>

struct libinput_source;

/* A coordinate pair in device coordinates */
struct device_coords {
	int x, y;
};

/*
 * A coordinate pair in device coordinates, capable of holding non discrete
 * values, this is necessary e.g. when device coordinates get averaged.
 */
struct device_float_coords {
	double x, y;
};

/* A dpi-normalized coordinate pair */
struct normalized_coords {
	double x, y;
};

/* A discrete step pair (mouse wheels) */
struct discrete_coords {
	int x, y;
};

/* A pair of angles in degrees */
struct tilt_degrees {
	double x, y;
};

struct tablet_axes {
	struct device_coords point;
	struct normalized_coords delta;
	double distance;
	double pressure;
	struct tilt_degrees tilt;
	double rotation;
	double slider;
	double wheel;
	int wheel_discrete;
};


struct libinput {
	int kq;
	struct udev *udev_ctx;
	struct list source_destroy_list;

	struct list seat_list;

	struct libinput_event **events;
	size_t events_count;
	size_t events_len;
	size_t events_in;
	size_t events_out;

	const struct libinput_interface *interface;

	libinput_log_handler log_handler;
	enum libinput_log_priority log_priority;
	void *user_data;
	int refcount;
};

struct libinput_seat {
	struct libinput *libinput;
	struct list link;
	struct list devices_list;
	void *user_data;
	int refcount;

	char *physical_name;
	char *logical_name;

	uint32_t button_count[KEY_CNT];
};

struct libinput_device_group {
};

enum devkind {
	TTYKBD = 1,
	SYSMOUSE
};

struct libinput_device {
	struct libinput_seat *seat;
	struct list link;
	void *user_data;
	int refcount;

	struct libinput_source *source;
	char *devname;
	enum devkind kind;
	int sysmouse_oldmask;
	int fd;
};

enum libinput_tablet_tool_axis {
	LIBINPUT_TABLET_TOOL_AXIS_X = 1,
	LIBINPUT_TABLET_TOOL_AXIS_Y = 2,
	LIBINPUT_TABLET_TOOL_AXIS_DISTANCE = 3,
	LIBINPUT_TABLET_TOOL_AXIS_PRESSURE = 4,
	LIBINPUT_TABLET_TOOL_AXIS_TILT_X = 5,
	LIBINPUT_TABLET_TOOL_AXIS_TILT_Y = 6,
	LIBINPUT_TABLET_TOOL_AXIS_ROTATION_Z = 7,
	LIBINPUT_TABLET_TOOL_AXIS_SLIDER = 8,
	LIBINPUT_TABLET_TOOL_AXIS_REL_WHEEL = 9,
};

#define LIBINPUT_TABLET_TOOL_AXIS_MAX LIBINPUT_TABLET_TOOL_AXIS_REL_WHEEL

struct libinput_event {
	enum libinput_event_type type;
	struct libinput_device *device;
};

typedef void (*libinput_source_dispatch_t)(void *data);

#define log_debug(li_, ...) log_msg((li_), LIBINPUT_LOG_PRIORITY_DEBUG, __VA_ARGS__)
#define log_info(li_, ...) log_msg((li_), LIBINPUT_LOG_PRIORITY_INFO, __VA_ARGS__)
#define log_error(li_, ...) log_msg((li_), LIBINPUT_LOG_PRIORITY_ERROR, __VA_ARGS__)
#define log_bug_kernel(li_, ...) log_msg((li_), LIBINPUT_LOG_PRIORITY_ERROR, "kernel bug: " __VA_ARGS__)
#define log_bug_libinput(li_, ...) log_msg((li_), LIBINPUT_LOG_PRIORITY_ERROR, "libinput bug: " __VA_ARGS__)
#define log_bug_client(li_, ...) log_msg((li_), LIBINPUT_LOG_PRIORITY_ERROR, "client bug: " __VA_ARGS__)

#define log_debug_ratelimit(li_, r_, ...) log_msg_ratelimit((li_), (r_), LIBINPUT_LOG_PRIORITY_DEBUG, __VA_ARGS__)
#define log_info_ratelimit(li_, r_, ...) log_msg_ratelimit((li_), (r_), LIBINPUT_LOG_PRIORITY_INFO, __VA_ARGS__)
#define log_error_ratelimit(li_, r_, ...) log_msg_ratelimit((li_), (r_), LIBINPUT_LOG_PRIORITY_ERROR, __VA_ARGS__)
#define log_bug_kernel_ratelimit(li_, r_, ...) log_msg_ratelimit((li_), (r_), LIBINPUT_LOG_PRIORITY_ERROR, "kernel bug: " __VA_ARGS__)
#define log_bug_libinput_ratelimit(li_, r_, ...) log_msg_ratelimit((li_), (r_), LIBINPUT_LOG_PRIORITY_ERROR, "libinput bug: " __VA_ARGS__)
#define log_bug_client_ratelimit(li_, r_, ...) log_msg_ratelimit((li_), (r_), LIBINPUT_LOG_PRIORITY_ERROR, "client bug: " __VA_ARGS__)

void
log_msg_ratelimit(struct libinput *libinput,
		  struct ratelimit *ratelimit,
		  enum libinput_log_priority priority,
		  const char *format, ...)
	LIBINPUT_ATTRIBUTE_PRINTF(4, 5);

void
log_msg(struct libinput *libinput,
	enum libinput_log_priority priority,
	const char *format, ...)
	LIBINPUT_ATTRIBUTE_PRINTF(3, 4);

void
log_msg_va(struct libinput *libinput,
	   enum libinput_log_priority priority,
	   const char *format,
	   va_list args)
	LIBINPUT_ATTRIBUTE_PRINTF(3, 0);

int
libinput_init(struct libinput *libinput,
	      const struct libinput_interface *interface,
	      void *user_data);

struct libinput_source *
libinput_add_fd(struct libinput *libinput,
		int fd,
		libinput_source_dispatch_t dispatch,
		void *data);

void
libinput_remove_source(struct libinput *libinput,
		       struct libinput_source *source);

int
open_restricted(struct libinput *libinput,
		const char *path, int flags);

void
close_restricted(struct libinput *libinput, int fd);

void
libinput_device_init(struct libinput_device *device,
		     struct libinput_seat *seat);

void
keyboard_notify_key(struct libinput_device *device,
		    uint64_t time,
		    uint32_t key,
		    enum libinput_key_state state);

void
pointer_notify_motion(struct libinput_device *device,
		      uint64_t time,
		      const struct normalized_coords *delta,
		      const struct device_float_coords *raw);

void
pointer_notify_button(struct libinput_device *device,
		      uint64_t time,
		      int32_t button,
		      enum libinput_button_state state);

#endif /* LIBINPUT_PRIVATE_H */
