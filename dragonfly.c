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

#include "libinput.h"
#include "libinput-util.h"
#include "libinput-private.h"

extern void	libinput_seat_init(struct libinput_seat *seat,
		    struct libinput *libinput, const char *physical_name,
		    const char *logical_name);
extern void	sysmouse_device_dispatch(void *data);


static const char default_seat[] = "seat0";
static const char default_seat_name[] = "default";

static struct libinput_seat*
dragonfly_seat_get(struct libinput *libinput, const char *seat_name_physical,
	const char *seat_name_logical)
{
	struct libinput_seat *seat;

	list_for_each(seat, &libinput->seat_list, link) {
		if (streq(seat->physical_name, seat_name_physical) &&
		    streq(seat->logical_name, seat_name_logical)) {
			libinput_seat_ref(seat);
			return seat;
		}
	}

	seat = calloc(1, sizeof(*seat));
	if (seat == NULL)
		return NULL;

	libinput_seat_init(seat, libinput, seat_name_physical,
		seat_name_logical);

	return seat;
}

struct libinput *
libinput_udev_create_context(const struct libinput_interface *interface,
	void *user_data, struct udev *udev)
{
	/*
	 * We do not support udev, hence creating a context from udev will
	 * fail.
	 */
	return NULL;
}

int
libinput_udev_assign_seat(struct libinput *libinput, const char *seat_id)
{
	/* Multiseat is not supported. */
	return -1;
}


LIBINPUT_EXPORT struct libinput *
libinput_path_create_context(const struct libinput_interface *interface,
     void *user_data)
{
	struct libinput *libinput;

	libinput = calloc(1, sizeof(*libinput));
	if (libinput == NULL)
		return NULL;

	if (libinput_init(libinput, interface, user_data) != 0) {
		free(libinput);
		return NULL;
	}

	return libinput;
}

LIBINPUT_EXPORT struct libinput_device *
libinput_path_add_device(struct libinput *libinput,
	const char *path)
{
	struct libinput_seat *seat = NULL;
	struct libinput_device *device;
	int fd;

	/* determine device kind (tty-keyboard or sysmouse) via devattr */

	fd = open_restricted(libinput, path,
			     O_RDWR | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) {
		log_info(libinput,
			 "opening input device '%s' failed (%s).\n",
			 path, strerror(-fd));
		return NULL;
	}

	device = calloc(1, sizeof(*device));
	if (device == NULL)
		return NULL;

	/* Only one (default) seat is supported. */
	seat = dragonfly_seat_get(libinput, default_seat, default_seat_name);
	if (seat == NULL)
		goto err;

	libinput_device_init(device, seat);

	device->fd = fd;
	device->devname = strdup(path);
	if (device->devname == NULL)
		goto err;

	/* XXX set _dispatch method depending on device kind */
	device->source =
		libinput_add_fd(libinput, fd, sysmouse_device_dispatch, device);
	if (!device->source)
		goto err;

	list_insert(&seat->devices_list, &device->link);

	return device;

err:
	close_restricted(libinput, device->fd);
	free(device);
	return NULL;
}

LIBINPUT_EXPORT void
libinput_path_remove_device(struct libinput_device *device)
{
	struct libinput *libinput = device->seat->libinput;

	libinput_remove_source(libinput, device->source);
	device->source = NULL;

	close_restricted(libinput, device->fd);
	device->fd = -1;

	libinput_device_unref(device);
}
