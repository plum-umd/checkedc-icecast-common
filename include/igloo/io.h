/* Copyright (C) 2019       Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef _LIBIGLOO__IO_H_
#define _LIBIGLOO__IO_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Put stuff here */

#include <igloo/config.h>

#ifdef IGLOO_CTC_HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef IGLOO_CTC_HAVE_POLL
#include <poll.h>
#endif
#ifdef IGLOO_CTC_HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef IGLOO_CTC_STDC_HEADERS
#include <stdarg.h>
#endif

#include "ro.h"
#include "interface.h"

/* About thread safety:
 * This set of functions is thread safe.
 */

igloo_RO_FORWARD_TYPE(igloo_io_t);

/* Type of blocking.
 */
typedef enum {
    /* Error state, used as return value only. */
    igloo_IO_BLOCKINGMODE_ERROR = -1,
    /* No blocking is done. This is the eqivalent of setting O_NONBLOCK on a POSIX system. */
    igloo_IO_BLOCKINGMODE_NONE = 0,
    /* Full blocking is done. This is the eqivalent of unsetting O_NONBLOCK on a POSIX system. */
    igloo_IO_BLOCKINGMODE_FULL
} libigloo_io_blockingmode_t;

/* Type for operaton flags.
 */
#ifdef IGLOO_CTC_HAVE_STDINT_H
typedef uint_least32_t igloo_io_opflag_t;
#else
typedef unsigned long int igloo_io_opflag_t;
#endif

/* No operation flags set. Usefull for variable initialization. */
#define igloo_IO_OPFLAG_NONE            ((igloo_io_opflag_t)0x0000)
/* Use default flags. Other flags can be set in addition. */
#define igloo_IO_OPFLAG_DEFAULTS        ((igloo_io_opflag_t)0x8000)

/* Operate on actual data. */
#define igloo_IO_OPFLAG_DATA            ((igloo_io_opflag_t)0x0001)
/* Operate on metadata. */
#define igloo_IO_OPFLAG_METADATA        ((igloo_io_opflag_t)0x0002)
/* Operate on data and metadata. */
#define igloo_IO_OPFLAG_FULL            (igloo_IO_OPFLAG_DATAONLY|igloo_IO_OPFLAG_METADATAONLY)
/* Instructs the operation that it should ignore the output state of the object.
 * This may improve performance as buffer flushes may be skipped.
 * However with this set any external software interacting with this object
 * MUST NOT interact with the output side of this object.
 */
#define igloo_IO_OPFLAG_NOWRITE         ((igloo_io_opflag_t)0x0010)

/* Get the referenced value */
#define igloo_IO_OPFLAG_GET             ((igloo_io_opflag_t)0x0100)
/* Set the referenced value */
#define igloo_IO_OPFLAG_SET             ((igloo_io_opflag_t)0x0200)

/* Advanced control functions.
 */
typedef enum {
    igloo_IO_CONTROL_NONE = 0
} igloo_io_control_t;

/* Interface description */
typedef struct {
    igloo_interface_base_ifdesc_t __base;

    ssize_t (*read)(igloo_INTERFACE_BASIC_ARGS, void *buffer, size_t len);
    ssize_t (*write)(igloo_INTERFACE_BASIC_ARGS, const void *buffer, size_t len);
    int (*flush)(igloo_INTERFACE_BASIC_ARGS, igloo_io_opflag_t flags);
    int (*sync)(igloo_INTERFACE_BASIC_ARGS, igloo_io_opflag_t flags);
    int (*set_blockingmode)(igloo_INTERFACE_BASIC_ARGS, libigloo_io_blockingmode_t mode);
    libigloo_io_blockingmode_t (*get_blockingmode)(igloo_INTERFACE_BASIC_ARGS);
    int (*get_fd_for_systemcall)(igloo_INTERFACE_BASIC_ARGS);
#ifdef IGLOO_CTC_STDC_HEADERS
    int (*control)(igloo_INTERFACE_BASIC_ARGS, igloo_io_opflag_t flags, igloo_io_control_t control, va_list ap);
#else
    int (*control)(igloo_INTERFACE_BASIC_ARGS, igloo_io_opflag_t flags, igloo_io_control_t control, ...);
#endif
} igloo_io_ifdesc_t;

/* This creates a new IO handle from a interface description and state.
 * Parameters:
 *  ifdesc
 *      The interface description to use.
 *  backend_object
 *      A object used by the backend or igloo_RO_NULL.
 *  backend_userdata
 *      A userdata pointer used by the backend or NULL.
 *  name, associated
 *      See refobject_new().
 */
igloo_io_t * igloo_io_new(const igloo_io_ifdesc_t *ifdesc, igloo_ro_t backend_object, void *backend_userdata, const char *name, igloo_ro_t associated);

/* Read data from a IO handle.
 * Parameters:
 *  io
 *      The IO handle to operate on.
 *  buffer
 *      The buffer to read to.
 *  len
 *      The maximum amount of bytes to read.
 * Returns:
 *  The actual amount of bytes read.
 */
ssize_t igloo_io_read(igloo_io_t *io, void *buffer, size_t len);
/* Write data to a IO handle.
 * Parameters:
 *  io
 *      The IO handle to operate on.
 *  buffer
 *      The buffer to write from.
 *  len
 *      The maximum amount of bytes to write.
 * Returns:
 *  The actual amount of bytes written.
 */
ssize_t igloo_io_write(igloo_io_t *io, const void *buffer, size_t len);

/* Flush internal buffers to the underlying object.
 * This does not guarantee that all data has been written to the physical level
 * on return. It just queues the flush.
 * Parameters:
 *  io
 *      The IO handle to operate on.
 *  flags
 *      Flags for this operation.
 */
int igloo_io_flush(igloo_io_t *io, igloo_io_opflag_t flags);
/* Sync object with physical state. This is used to get the object into a state
 * that allows passing the underlying object to other software.
 * This may also flush internal buffers.
 * Parameters:
 *  io
 *      The IO handle to operate on.
 *  flags
 *      Flags for this operation.
 */
int igloo_io_sync(igloo_io_t *io, igloo_io_opflag_t flags);

/* Set and get the blocking state of the object.
 * Parameters:
 *  io
 *      The IO handle to operate on.
 *  mode
 *      The new blocking mode.
 */
int igloo_io_set_blockingmode(igloo_io_t *io, libigloo_io_blockingmode_t mode);
libigloo_io_blockingmode_t igloo_io_get_blockingmode(igloo_io_t *io);

#ifdef IGLOO_CTC_HAVE_SYS_SELECT_H
/* Those functions act as igloo's replacement for FD_SET(), FD_CLR(), and FD_SET() and
 * allow the caller to use the select() system call.
 * Parameters:
 *  io
 *      The IO handle to operate on.
 *  set
 *      The fd_set to operate on.
 *  maxfd
 *      This parameter is updated to reflect the added handle(s)
 *      in select()'s nfds parameter.
 *  flags
 *      Flags for this operation.
 *      It may be useful to igloo_IO_OPFLAG_NOWRITE in some cases. See it's definition.
 * Notes:
 *  This may call igloo_io_sync(). If the object is touched between calls to igloo_io_select_set(),
 *  select() the user MUST call igloo_io_sync() with the correct flags. It is RECOMMENDED
 *  to avoid this, as finding the correct parameters for igloo_io_sync() might be tricky.
 *  The object MUST NOT be touched between select() and igloo_io_select_isset().
 */
int igloo_io_select_set(igloo_io_t *io, fd_set *set, int *maxfd, igloo_io_opflag_t flags);
int igloo_io_select_clear(igloo_io_t *io, fd_set *set);
int igloo_io_select_isset(igloo_io_t *io, fd_set *set);
#endif

#ifdef IGLOO_CTC_HAVE_POLL
/* This function initializes the pollfd structure to use this handle
 * with the poll() system call.
 * Parameters:
 *  io
 *      The IO handle to operate on.
 *  fd
 *      The structure to initialize.
 *  events
 *      The events to set. libigloo uses different internal parameters based
 *      on this parameter's value. The parameter MUST NOT be changed in the structre
 *      after initialization.
 * Notes:
 *  This may call igloo_io_sync(). If the object is touched between calls to igloo_io_poll_fill(),
 *  poll() the user MUST call igloo_io_sync() with the correct flags. It is RECOMMENDED
 *  to avoid this, as finding the correct parameters for igloo_io_sync() might be tricky.
 */
int igloo_io_poll_fill(igloo_io_t *io, struct pollfd *fd, short events);
#endif

/* On a listen socket accept a new connection.
 * Parameters:
 *  io
 *      The IO handle to operate on.
 * Returns:
 *  The new connection or igloo_RO_NULL.
 */
/* TODO: Allow accept to accept()'s and accept4()'s additional parameters.
igloo_io_t *igloo_io_accept(igloo_io_t *io);
*/

/* Advanced control interface.
 * Parameters:
 *  io
 *      The IO handle to operate on.
 *  flags
 *      Flags for this operation.
 *      Most controls accept igloo_IO_OPFLAG_GET and igloo_IO_OPFLAG_SET.
 *      If both are set the value is first set as with igloo_IO_OPFLAG_SET and
 *      the OLD value is returned as with igloo_IO_OPFLAG_GET.
 *  control
 *      The control command to use.
 *  ...
 *      Additional parameters if any.
 *      Values are always passed as pointers.
 */
int igloo_io_control(igloo_io_t *io, igloo_io_opflag_t flags, igloo_io_control_t control, ...);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__IO_H_ */
