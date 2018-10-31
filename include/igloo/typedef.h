/* Copyright (C) 2018       Marvin Scholz <epirat07@gmail.com>
 * Copyright (C) 2012       Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
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

#ifndef _LIBIGLOO__TYPEDEF_H_
#define _LIBIGLOO__TYPEDEF_H_

#ifdef __cplusplus
extern "C" {
#endif

/* This header includes only macros needed to define types.
 * This header must be included before "types.h" and "ro.h" if used.
 */

#define igloo_RO_TYPE(type)             type * subtype__ ## type;
#define igloo_RO_FORWARD_TYPE(type)     extern const igloo_ro_type_t *igloo_ro__type__ ## type

#ifdef __cplusplus
}
#endif

#endif
