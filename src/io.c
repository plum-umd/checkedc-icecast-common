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

    /* Mutex for igloo_ro_*(). */
    igloo_mutex_t lock;

    int touched;
#if defined(IGLOO_CTC_HAVE_SYS_SELECT_H) || defined(IGLOO_CTC_HAVE_POLL)
    int fd;
#endif
};

static void __free(igloo_ro_t self)
{
    igloo_io_t *io = igloo_RO_TO_TYPE(self, igloo_io_t);

    igloo_thread_mutex_lock(&(io->lock));
    igloo_thread_mutex_unlock(&(io->lock));
    igloo_thread_mutex_destroy(&(io->lock));
    igloo_interface_base_free(self);
}

igloo_RO_PUBLIC_TYPE(igloo_io_t,
        igloo_RO_TYPEDECL_FREE(__free)
        );


igloo_io_t * igloo_io_new(const igloo_io_ifdesc_t *ifdesc, igloo_ro_t backend_object, void *backend_userdata, const char *name, igloo_ro_t associated)
{
    igloo_io_t *io = igloo_interface_base_new(igloo_io_t, ifdesc, backend_object, backend_userdata, name, associated);
    if (!io)
        return NULL;

    igloo_thread_mutex_create(&(io->lock));

    io->touched = 1;

    return io;
}


ssize_t igloo_io_read(igloo_io_t *io, void *buffer, size_t len)
{
    ssize_t ret = -1;

    if (!io || !buffer)
        return -1;

    if (!len)
        return 0;

    igloo_thread_mutex_lock(&(io->lock));
    io->touched = 1;

    if (io->ifdesc->read)
        ret = io->ifdesc->read(igloo_INTERFACE_BASIC_CALL(io), buffer, len);
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
}

ssize_t igloo_io_write(igloo_io_t *io, const void *buffer, size_t len)
{
    ssize_t ret = -1;

    if (!io || !buffer)
        return -1;

    if (!len)
        return 0;

    igloo_thread_mutex_lock(&(io->lock));
    io->touched = 1;

    if (io->ifdesc->write)
        ret = io->ifdesc->write(igloo_INTERFACE_BASIC_CALL(io), buffer, len);
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
}

int igloo_io_flush(igloo_io_t *io, igloo_io_opflag_t flags)
{
    int ret = -1;

    if (!io)
        return -1;

    igloo_thread_mutex_lock(&(io->lock));
    io->touched = 1;

    if (io->ifdesc->flush)
        ret = io->ifdesc->flush(igloo_INTERFACE_BASIC_CALL(io), flags);
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
}

int igloo_io_sync(igloo_io_t *io, igloo_io_opflag_t flags)
{
    int ret;

    if (!io)
        return -1;

    igloo_thread_mutex_lock(&(io->lock));
    if (io->ifdesc->flush)
        io->ifdesc->flush(igloo_INTERFACE_BASIC_CALL(io), flags);

    if (io->ifdesc->sync) {
        ret = io->ifdesc->sync(igloo_INTERFACE_BASIC_CALL(io), flags);
        if (ret != 0) {
            igloo_thread_mutex_unlock(&(io->lock));
            return ret;
        }

        io->touched = 0;

        igloo_thread_mutex_unlock(&(io->lock));
        return ret;
    }
    igloo_thread_mutex_unlock(&(io->lock));

    return -1;
}

int igloo_io_set_blockingmode(igloo_io_t *io, libigloo_io_blockingmode_t mode)
{
    int ret = -1;

    if (!io)
        return -1;

    if (mode != igloo_IO_BLOCKINGMODE_NONE && mode != igloo_IO_BLOCKINGMODE_FULL)
        return -1;

    igloo_thread_mutex_lock(&(io->lock));
    io->touched = 1;

    if (io->ifdesc->set_blockingmode)
        ret = io->ifdesc->set_blockingmode(igloo_INTERFACE_BASIC_CALL(io), mode);
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
}
libigloo_io_blockingmode_t igloo_io_get_blockingmode(igloo_io_t *io)
{
    libigloo_io_blockingmode_t ret = igloo_IO_BLOCKINGMODE_ERROR;

    if (!io)
        return igloo_IO_BLOCKINGMODE_ERROR;

    igloo_thread_mutex_lock(&(io->lock));
    if (io->ifdesc->get_blockingmode)
        ret = io->ifdesc->get_blockingmode(igloo_INTERFACE_BASIC_CALL(io));
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
}

#ifdef IGLOO_CTC_HAVE_SYS_SELECT_H
int igloo_io_select_set(igloo_io_t *io, fd_set *set, int *maxfd, igloo_io_opflag_t flags)
{
    int ret;

    if (!io || !set || !maxfd)
        return -1;

    igloo_thread_mutex_lock(&(io->lock));
    if (!io->ifdesc->get_fd_for_systemcall) {
        igloo_thread_mutex_unlock(&(io->lock));
        return -1;
    }

    if (io->touched || !(flags & igloo_IO_OPFLAG_NOWRITE)) {
        igloo_thread_mutex_unlock(&(io->lock));
        ret = igloo_io_sync(io, igloo_IO_OPFLAG_DEFAULTS|(flags & igloo_IO_OPFLAG_NOWRITE));

        if (ret != 0)
            return ret;

        igloo_thread_mutex_lock(&(io->lock));
        if (io->touched) {
            igloo_thread_mutex_unlock(&(io->lock));
            return -1;
        }
    }

    io->fd = io->ifdesc->get_fd_for_systemcall(igloo_INTERFACE_BASIC_CALL(io));
    if (io->fd < 0)
        return -1;

    FD_SET(io->fd, set);
    if (*maxfd < io->fd)
        *maxfd = io->fd;

    igloo_thread_mutex_unlock(&(io->lock));

    return 0;
}

int igloo_io_select_clear(igloo_io_t *io, fd_set *set)
{
    if (!io || !set)
        return -1;

    igloo_thread_mutex_lock(&(io->lock));
    if (io->touched || io->fd < 0) {
        igloo_thread_mutex_unlock(&(io->lock));
        return -1;
    }

    FD_CLR(io->fd, set);
    igloo_thread_mutex_unlock(&(io->lock));

    return 0;
}

int igloo_io_select_isset(igloo_io_t *io, fd_set *set)
{
    int ret = -1;

    if (!io || !set)
        return -1;

    igloo_thread_mutex_lock(&(io->lock));
    if (!io->touched && io->fd >= 0)
        ret = FD_ISSET(io->fd, set) ? 1 : 0;
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
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

    is_safe = !((events|safe_events) - safe_events);

    igloo_thread_mutex_lock(&(io->lock));
    if (!io->ifdesc->get_fd_for_systemcall) {
        igloo_thread_mutex_unlock(&(io->lock));
        return -1;
    }

    if (io->touched || !is_safe) {
        igloo_thread_mutex_unlock(&(io->lock));
        ret = igloo_io_sync(io, igloo_IO_OPFLAG_DEFAULTS|(is_safe ? 0 : igloo_IO_OPFLAG_NOWRITE));

        if (ret != 0)
            return ret;

        igloo_thread_mutex_lock(&(io->lock));
        if (io->touched) {
            igloo_thread_mutex_unlock(&(io->lock));
            return -1;
        }
    }

    io->fd = io->ifdesc->get_fd_for_systemcall(igloo_INTERFACE_BASIC_CALL(io));
    if (io->fd < 0) {
        igloo_thread_mutex_unlock(&(io->lock));
        return -1;
    }

    memset(fd, 0, sizeof(*fd));
    fd->fd = io->fd;
    fd->events = events;

    igloo_thread_mutex_unlock(&(io->lock));

    return 0;
}
#endif

int igloo_io_control(igloo_io_t *io, igloo_io_opflag_t flags, igloo_io_control_t control, ...)
{
#ifdef IGLOO_CTC_STDC_HEADERS
    int ret = -1;
    va_list ap;

    if (!io)
        return -1;

    igloo_thread_mutex_lock(&(io->lock));
    if (io->ifdesc->control) {
        va_start(ap, control);
        ret = io->ifdesc->control(igloo_INTERFACE_BASIC_CALL(io), flags, control, ap);
        va_end(ap);
    }
    igloo_thread_mutex_unlock(&(io->lock));

    return ret;
#else
    return -1;
#endif
}
