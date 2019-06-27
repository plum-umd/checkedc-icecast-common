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

struct igloo_io_tag {
    igloo_interface_base(io)
};

igloo_RO_PUBLIC_TYPE(igloo_io_t,
        igloo_RO_TYPEDECL_FREE(igloo_interface_base_free)
        );
