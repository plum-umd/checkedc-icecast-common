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

#ifndef _LIBIGLOO__OBJECTHANDLER_H_
#define _LIBIGLOO__OBJECTHANDLER_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "ro.h"
#include "interface.h"
#include "filter.h"

/* About thread safety:
 * This set of functions is thread safe.
 */

igloo_RO_FORWARD_TYPE(igloo_objecthandler_t);

/* Interface description */
typedef struct {
    igloo_interface_base_ifdesc_t __base;

    /* Whether the this backend is thread safe. */
    int is_thread_safe;

    /* Perform the actual handling of the object. */
    igloo_filter_result_t (*handle)(igloo_INTERFACE_BASIC_ARGS, igloo_ro_t object);

    /* Flush all data left in the handler.
     * It should also flush the backend if the backend can be flushed.
     */
    int (*flush)(igloo_INTERFACE_BASIC_ARGS);
    /* Detach old and attach new backend if operation is possible.
     * This is always done locked. So this function must not be thread safe.
     * igloo_objecthandler_flush() is automatically called before this
     * and is also protected by the lock so that there is no race between
     * the flush and the backend update.
     */
    int (*set_backend)(igloo_INTERFACE_BASIC_ARGS, igloo_ro_t backend);
} igloo_objecthandler_ifdesc_t;

/* This creates a new objecthandler from a interface description and state.
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
igloo_objecthandler_t * igloo_objecthandler_new(const igloo_objecthandler_ifdesc_t *ifdesc, igloo_ro_t backend_object, void *backend_userdata, const char *name, igloo_ro_t associated);

/* This handles a object according to the filter.
 * Parameters:
 *  handler
 *      The handler to use.
 *  object
 *      The object to test.
 * Returns:
 *  Whether the object was accepted by the handler or not.
 */
igloo_filter_result_t igloo_objecthandler_handle(igloo_objecthandler_t *handler, igloo_ro_t object);

/* Flushes the object handler. All data still in queue will be
 * written to the backend.
 * Parameters:
 *  handler
 *      The handler to use.
 */
int igloo_objecthandler_flush(igloo_objecthandler_t *handler);

/* Detaches the old and attaches a new backend.
 * This is useful for e.g. reopening files.
 *
 * This also calls igloo_objecthandler_flush().
 *
 * Parameters:
 *  handler
 *      The handler to use.
 *  backend
 *      The new backend to use.
 */
int igloo_objecthandler_set_backend(igloo_objecthandler_t *handler, igloo_ro_t backend);

/* This adds a filter to the handler.
 * Parameters:
 *  handler
 *      The handler to add the filter to.
 *  filter
 *      The filter to add.
 */
int igloo_objecthandler_push_filter(igloo_objecthandler_t *handler, igloo_filter_t *filter);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__OBJECTHANDLER_H_ */
