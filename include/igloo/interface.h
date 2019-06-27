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

#ifndef _LIBIGLOO__INTERFACE_H_
#define _LIBIGLOO__INTERFACE_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Put stuff here */

#include "ro.h"

/* Basic arguments that should be passed to all interface functions */
#define igloo_INTERFACE_BASIC_ARGS igloo_ro_t self, igloo_ro_t *backend_object, void **backend_userdata

/* Basic free callback.
 * If this does not set *backend_object to igloo_RO_NULL
 * backend_object will be freed by calling igloo_ro_unref().
 * If this does not set *backend_userdata to NULL
 *
 */
typedef int (*igloo_interface_free_t)(igloo_INTERFACE_BASIC_ARGS);


typedef struct {
    igloo_interface_free_t free;
} igloo_interface_base_ifdesc_t;

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__INTERFACE_H_ */
