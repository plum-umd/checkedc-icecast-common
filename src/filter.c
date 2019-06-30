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

#include <igloo/ro.h>
#include <igloo/filter.h>
#include "private.h"

struct igloo_filter_tag {
    igloo_interface_base(filter)
};

igloo_RO_PUBLIC_TYPE(igloo_filter_t,
        igloo_RO_TYPEDECL_FREE(igloo_interface_base_free)
        );

igloo_filter_t * igloo_filter_new(const igloo_filter_ifdesc_t *ifdesc, igloo_ro_t backend_object, void *backend_userdata, const char *name, igloo_ro_t associated)
{
    return igloo_interface_base_new(igloo_filter_t, ifdesc, backend_object, backend_userdata, name, associated);
}

igloo_filter_result_t igloo_filter_test(igloo_filter_t *filter, igloo_ro_t object)
{
    if (!filter)
        return igloo_FILTER_RESULT_ERROR;

    if (filter->ifdesc->test)
        return filter->ifdesc->test(igloo_INTERFACE_BASIC_CALL(filter), object);

    return igloo_FILTER_RESULT_ERROR;
}
