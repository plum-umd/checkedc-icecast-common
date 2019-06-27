/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2019,      Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>,
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <igloo/ro.h>
#include <igloo/io.h>
#include "private.h"

struct igloo_io_tag {
    igloo_interface_base(io)
    int touched;
#if defined(IGLOO_CTC_HAVE_SYS_SELECT_H) || defined(IGLOO_CTC_HAVE_POLL)
    int fd;
#endif
};

igloo_RO_PUBLIC_TYPE(igloo_io_t,
        igloo_RO_TYPEDECL_FREE(igloo_interface_base_free)
        );


igloo_io_t * igloo_io_new(const igloo_io_ifdesc_t *ifdesc, igloo_ro_t backend_object, void *backend_userdata, const char *name, igloo_ro_t associated)
{
    igloo_io_t *io;

    if (!ifdesc)
        return NULL;

    io = igloo_ro_new_raw(igloo_io_t, name, associated);

    if (!io)
        return NULL;

    if (!igloo_RO_IS_NULL(backend_object)) {
        if (igloo_ro_ref(backend_object) != 0) {
            igloo_ro_unref(io);
            return NULL;
        }
    }

    io->ifdesc = ifdesc;
    io->backend_object = backend_object;
    io->backend_userdata = backend_userdata;

    return io;
}


ssize_t igloo_io_read(igloo_io_t *io, void *buffer, size_t len)
{
    if (!io || !buffer)
        return -1;

    if (!len)
        return 0;

    io->touched = 1;

    if (io->ifdesc->read)
        return io->ifdesc->read(igloo_INTERFACE_BASIC_CALL(io), buffer, len);

    return -1;
}

ssize_t igloo_io_write(igloo_io_t *io, const void *buffer, size_t len)
{
    if (!io || !buffer)
        return -1;

    if (!len)
        return 0;

    io->touched = 1;

    if (io->ifdesc->write)
        return io->ifdesc->write(igloo_INTERFACE_BASIC_CALL(io), buffer, len);

    return -1;
}

int igloo_io_flush(igloo_io_t *io, igloo_io_opflag_t flags)
{
    if (!io)
        return -1;

    io->touched = 1;

    if (io->ifdesc->flush)
        return io->ifdesc->flush(igloo_INTERFACE_BASIC_CALL(io), flags);

    return -1;
}

int igloo_io_sync(igloo_io_t *io, igloo_io_opflag_t flags)
{
    int ret;

    if (!io)
        return -1;

    if (io->ifdesc->flush)
        io->ifdesc->flush(igloo_INTERFACE_BASIC_CALL(io), flags);

    if (io->ifdesc->sync) {
        ret = io->ifdesc->sync(igloo_INTERFACE_BASIC_CALL(io), flags);
        if (ret != 0)
            return ret;

        io->touched = 0;

        return ret;
    }

    return -1;
}

int igloo_io_set_blockingmode(igloo_io_t *io, libigloo_io_blockingmode_t mode)
{
    if (!io)
        return -1;

    if (mode != igloo_IO_BLOCKINGMODE_NONE && mode != igloo_IO_BLOCKINGMODE_FULL)
        return -1;

    io->touched = 1;

    if (io->ifdesc->set_blockingmode)
        return io->ifdesc->set_blockingmode(igloo_INTERFACE_BASIC_CALL(io), mode);

    return -1;
}
libigloo_io_blockingmode_t igloo_io_get_blockingmode(igloo_io_t *io)
{
    if (!io)
        return igloo_IO_BLOCKINGMODE_ERROR;

    if (io->ifdesc->get_blockingmode)
        return io->ifdesc->get_blockingmode(igloo_INTERFACE_BASIC_CALL(io));

    return igloo_IO_BLOCKINGMODE_ERROR;
}

#ifdef IGLOO_CTC_HAVE_SYS_SELECT_H
int igloo_io_select_set(igloo_io_t *io, fd_set *set, int *maxfd, igloo_io_opflag_t flags)
{
    int ret;

    if (!io || !set || !maxfd)
        return -1;

    if (!io->ifdesc->get_fd_for_systemcall)
        return -1;

    if (io->touched || !(flags & igloo_IO_OPFLAG_NOWRITE)) {
        ret = igloo_io_sync(io, igloo_IO_OPFLAG_DEFAULTS|(flags & igloo_IO_OPFLAG_NOWRITE));
        if (ret != 0)
            return ret;
        if (io->touched)
            return -1;
    }

    io->fd = io->ifdesc->get_fd_for_systemcall(igloo_INTERFACE_BASIC_CALL(io));
    if (io->fd < 0)
        return -1;

    FD_SET(io->fd, set);
    if (*maxfd < io->fd)
        *maxfd = io->fd;

    return 0;
}

int igloo_io_select_clear(igloo_io_t *io, fd_set *set)
{
    if (!io || !set)
        return -1;

    if (io->touched || io->fd < 0)
        return -1;

    FD_CLR(io->fd, set);

    return 0;
}

int igloo_io_select_isset(igloo_io_t *io, fd_set *set)
{
    if (!io || !set)
        return -1;

    if (io->touched || io->fd < 0)
        return -1;

    return FD_ISSET(io->fd, set) ? 1 : 0;
}
#endif

#ifdef IGLOO_CTC_HAVE_POLL
int igloo_io_poll_fill(igloo_io_t *io, struct pollfd *fd, short events)
{
    static const short safe_events = POLLIN|POLLPRI
#ifdef POLLRDHUP
        |POLLRDHUP
#endif
#ifdef POLLRDNORM
        |POLLRDNORM
#endif
    ;
    int is_safe;
    int ret;

    if (!io || !fd)
        return -1;

    if (!io->ifdesc->get_fd_for_systemcall)
        return -1;

    is_safe = !((events|safe_events) - safe_events);

    if (io->touched || !is_safe) {
        ret = igloo_io_sync(io, igloo_IO_OPFLAG_DEFAULTS|(is_safe ? 0 : igloo_IO_OPFLAG_NOWRITE));
        if (ret != 0)
            return ret;
        if (io->touched)
            return -1;
    }

    io->fd = io->ifdesc->get_fd_for_systemcall(igloo_INTERFACE_BASIC_CALL(io));
    if (io->fd < 0)
        return -1;

    memset(fd, 0, sizeof(*fd));
    fd->fd = io->fd;
    fd->events = events;

    return 0;
}
#endif

int igloo_io_control(igloo_io_t *io, igloo_io_opflag_t flags, igloo_io_control_t control, ...)
{
#ifdef IGLOO_CTC_STDC_HEADERS
    int ret;
    va_list ap;

    if (!io)
        return -1;

    if (!io->ifdesc->control)
        return -1;

    va_start(ap, control);
    ret = io->ifdesc->control(igloo_INTERFACE_BASIC_CALL(io), flags, control, ap);
    va_end(ap);

    return ret;
#else
    return -1;
#endif
}
