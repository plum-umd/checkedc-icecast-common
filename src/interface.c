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

    if (iface->ifdesc && iface->ifdesc->free)
        iface->ifdesc->free(igloo_INTERFACE_BASIC_CALL(self));

    if (!igloo_RO_IS_NULL(iface->backend_object))
        igloo_ro_unref(iface->backend_object);

    if (iface->backend_userdata)
        free(iface->backend_userdata);
}

igloo_ro_t igloo_interface_base_new_real(const igloo_ro_type_t *type, size_t description_length, const igloo_interface_base_ifdesc_t *ifdesc, igloo_ro_t backend_object, void *backend_userdata, const char *name, igloo_ro_t associated)
{
    igloo_ro_t self;
    igloo__interface_base_t *base;

    if (!ifdesc)
        return igloo_RO_NULL;

    if (ifdesc->base_length != sizeof(igloo_interface_base_ifdesc_t) || ifdesc->base_version != igloo_INTERFACE_DESCRIPTION_BASE__VERSION || ifdesc->description_length != description_length)
        return igloo_RO_NULL;

    self = igloo_ro_new__raw(type, name, associated);
    base = igloo_INTERFACE_CAST(self);

    if (igloo_RO_IS_NULL(self))
        return igloo_RO_NULL;

    if (!igloo_RO_IS_NULL(backend_object)) {
        if (igloo_ro_ref(backend_object) != 0) {
            igloo_ro_unref(self);
            return igloo_RO_NULL;
        }
    }

    base->ifdesc = ifdesc;
    base->backend_object = backend_object;
    base->backend_userdata = backend_userdata;

    return self;
}
