/* Copyright (C) 2018       Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
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

#ifndef _LIBIGLOO__PRIVATE_H_
#define _LIBIGLOO__PRIVATE_H_

#ifdef STDC_HEADERS
#include <stdarg.h>
#endif

#include <igloo/interface.h>

/* init/shutdown of the library */
void igloo_thread_initialize(void);
void igloo_thread_initialize_with_log_id(int log_id);
void igloo_thread_shutdown(void);
void igloo_sock_initialize(void);
void igloo_sock_shutdown(void);
void igloo_resolver_initialize(void);
void igloo_resolver_shutdown(void);
void igloo_log_initialize(void);
void igloo_log_shutdown(void);

/* Basic interface */
#define igloo_interface_base(type) \
    /* The base object. */ \
    igloo_ro_base_t __base; \
    /* The interface description */ \
    const igloo_ ## type ## _ifdesc_t *ifdesc; \
    /* Backend object */ \
    igloo_ro_t backend_object; \
    /* Backend userdata */ \
    void *backend_userdata;

typedef struct {
    igloo_interface_base(interface_base)
} igloo__interface_base_t;

void igloo_interface_base_free(igloo_ro_t self);

#define igloo_INTERFACE_CAST(obj) ((igloo__interface_base_t*)igloo_RO__GETBASE(obj))
#define igloo_INTERFACE_BASIC_CALL(obj) (obj), &(igloo_INTERFACE_CAST(obj)->backend_object), &(igloo_INTERFACE_CAST(obj)->backend_userdata)

igloo_ro_t igloo_interface_base_new_real(const igloo_ro_type_t *type, size_t description_length, const igloo_interface_base_ifdesc_t *ifdesc, igloo_ro_t backend_object, void *backend_userdata, const char *name, igloo_ro_t associated);
#define igloo_interface_base_new(type, ifdesc, backend_object, backend_userdata, name, associated) igloo_RO_TO_TYPE(igloo_interface_base_new_real(igloo_ro__type__ ## type, sizeof(*(ifdesc)), (const igloo_interface_base_ifdesc_t*)(ifdesc), (backend_object), (backend_userdata), (name), (associated)), type)

void igloo_private__vsnprintf(char *str, size_t size, const char *format, va_list ap);

#endif
