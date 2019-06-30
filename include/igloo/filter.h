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

#ifndef _LIBIGLOO__FILTER_H_
#define _LIBIGLOO__FILTER_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "ro.h"
#include "interface.h"

/* About thread safety:
 * This set of functions is thread safe.
 */

igloo_RO_FORWARD_TYPE(igloo_filter_t);

/* Result of a test. */
typedef enum {
    /* The test resulted in an error condition. */
    igloo_FILTER_RESULT_ERROR   = -1,
    /* The object did not pass the test and should not be processed. */
    igloo_FILTER_RESULT_DROP    =  0,
    /* The object passed the test and should be processed. */
    igloo_FILTER_RESULT_PASS    =  1
} igloo_filter_result_t;

/* Interface description */
typedef struct {
    igloo_interface_base_ifdesc_t __base;

    /* Perform the actual test on the object.
     * This must implement thread-safety itself if necessary.
     */
    igloo_filter_result_t (*test)(igloo_INTERFACE_BASIC_ARGS, igloo_ro_t object);
} igloo_filter_ifdesc_t;

/* This creates a new filter from a interface description and state.
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
igloo_filter_t * igloo_filter_new(const igloo_filter_ifdesc_t *ifdesc, igloo_ro_t backend_object, void *backend_userdata, const char *name, igloo_ro_t associated);

/* This tests a object according to the filter.
 * Parameters:
 *  filter
 *      The filter to use.
 *  object
 *      The object to test.
 * Returns:
 *  Result of the test (error, drop, pass).
 */
igloo_filter_result_t igloo_filter_test(igloo_filter_t *filter, igloo_ro_t object);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__FILTER_H_ */
