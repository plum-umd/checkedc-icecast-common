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

#include <stdio.h>

#include <igloo/io.h>

static int __free(igloo_INTERFACE_BASIC_ARGS)
{
    fclose(*backend_userdata);
    *backend_userdata = NULL;
    return 0;
}


static ssize_t __read(igloo_INTERFACE_BASIC_ARGS, void *buffer, size_t len)
{
    return fread(buffer, 1, len, *backend_userdata);
}
static ssize_t __write(igloo_INTERFACE_BASIC_ARGS, const void *buffer, size_t len)
{
    return fwrite(buffer, 1, len, *backend_userdata);
}

static int __flush(igloo_INTERFACE_BASIC_ARGS, igloo_io_opflag_t flags)
{
    return fflush(*backend_userdata) == 0 ? 0 : -1;
}
static int __sync(igloo_INTERFACE_BASIC_ARGS, igloo_io_opflag_t flags)
{
    return 0;
}
static libigloo_io_blockingmode_t __get_blockingmode(igloo_INTERFACE_BASIC_ARGS)
{
    return igloo_IO_BLOCKINGMODE_FULL;
}
static int __get_fd_for_systemcall(igloo_INTERFACE_BASIC_ARGS)
{
    return fileno(*backend_userdata);
}

static const igloo_io_ifdesc_t igloo_stdio_ifdesc = {
    igloo_INTERFACE_DESCRIPTION_BASE(igloo_io_ifdesc_t,
            .free = __free
            ),
    .read = __read,
    .write = __write,
    .flush = __flush,
    .sync = __sync,
    .get_blockingmode = __get_blockingmode,
    .get_fd_for_systemcall = __get_fd_for_systemcall
};

igloo_io_t * igloo_stdio_new_file(const char *filename, const char *mode, const char *name, igloo_ro_t associated)
{
    FILE *file = fopen(filename, mode);
    igloo_io_t *io;

    if (!file)
        return NULL;

    io = igloo_io_new(&igloo_stdio_ifdesc, igloo_RO_NULL, file, name, associated);
    if (!io) {
        fclose(file);
        return io;
    }

    return io;
}
