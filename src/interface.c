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

#include <igloo/ro.h>
#include <igloo/io.h>
#include "private.h"

void igloo_interface_base_free(igloo_ro_t self)
{
    igloo__interface_base_t *iface = igloo_INTERFACE_CAST(self);

    if (iface->ifdesc->free)
        iface->ifdesc->free(igloo_INTERFACE_BASIC_CALL(self));

    if (!igloo_RO_IS_NULL(iface->backend_object))
        igloo_ro_unref(iface->backend_object);

    if (iface->backend_userdata)
        free(iface->backend_userdata);
}

