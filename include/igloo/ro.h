/* Copyright (C) 2018       Marvin Scholz <epirat07@gmail.com>
 * Copyright (C) 2018-2019  Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
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

#ifndef _LIBIGLOO__RO_H_
#define _LIBIGLOO__RO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <igloo/config.h>

#ifdef IGLOO_CTC_HAVE_STDINT_H
#include <stdint.h>
#endif
#include <stdarg.h>

#include <igloo/config.h>
#include "types.h"
#include "thread.h"

/* Type used for callback called then the object is actually freed
 * That is once all references to it are gone.
 *
 * This function must not try to deallocate or alter self.
 */
typedef void (*igloo_ro_free_t)(igloo_ro_t self);

/* Type used for callback called then the object is created
 * using the generic igloo_ro_new().
 *
 * Additional parameters passed to igloo_ro_new() are passed
 * in the list ap. All limitations of <stdarg.h> apply.
 *
 * This function must return zero in case of success and
 * non-zero in case of error. In case of error igloo_ro_unref()
 * is called internally to clear the object.
 */
typedef int (*igloo_ro_new_t)(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap);

/* Type used to store flags for clone operations.
 */
#ifdef IGLOO_CTC_HAVE_STDINT_H
typedef uint_least32_t igloo_ro_cf_t;
#else
typedef unsigned long int igloo_ro_cf_t;
#endif

/* No clone flags set. Usefull for variable initialization. */
#define igloo_RO_CF_NONE            ((igloo_ro_cf_t)0x0000)
/* Make a shallow copy of the object */
#define igloo_RO_CF_SHALLOW         ((igloo_ro_cf_t)0x0001)
/* Make a deep copy of the object */
#define igloo_RO_CF_DEEP            ((igloo_ro_cf_t)0x0002)
/* Make a copy of the object that shares part of it's state with the original.
 * This is similar to using dup() on a POSIX filehandle.
 * This is useful when interacting with IO.
 */
#define igloo_RO_CF_DUP             ((igloo_ro_cf_t)0x0004)
/* Defaults to use if required and allowed is igloo_RO_CF_NONE */
#define igloo_RO_CF_DEFAULT         (igloo_RO_CF_SHALLOW|igloo_RO_CF_DEEP)

/* Type uses for callback called when the object should be cloned.
 *
 * A clone of the object must never have the same address of the object that was cloned.
 *
 * Parameters:
 *  self
 *      The object to clone.
 *  required, allowed
 *      See igloo_ro_clone().
 *  name, associated
 *      See igloo_ro_new().
 */
typedef igloo_ro_t (*igloo_ro_clone_t)(igloo_ro_t self, igloo_ro_cf_t required, igloo_ro_cf_t allowed, const char *name, igloo_ro_t associated);

/* Type used for callback called when the object needs to be converted to another type.
 *
 * This callback is not used when a object of the same type is requested.
 * On that case clone is requested.
 *
 * There are two cases that must be handled by the callback:
 * 0) The object self is of the same type as this callback is set for and type is pointing to different type.
 * 1) The object self is of any type and type is pointing to the type this callback was set to.
 *
 * If the callback can not convert it must return igloo_RO_NULL.
 *
 * Parameters:
 *  self
 *      The object to convert.
 *  type
 *      The type to convert to.
 *  required, allowed
 *      See igloo_ro_clone().
 *  name, associated
 *      See igloo_ro_new().
 */
typedef igloo_ro_t (*igloo_ro_convert_t)(igloo_ro_t self, const igloo_ro_type_t *type, igloo_ro_cf_t required, igloo_ro_cf_t allowed, const char *name, igloo_ro_t associated);

/* Type used for callback called when a specific interface to the object is requested.
 *
 * There are two cases that must be handled by the callback:
 * 0) The object self is of the same type as this callback is set for and type is pointing to different type that is an interface.
 * 1) The object self is of any type and type is pointing to the type this callback was set to which is an interface.
 *
 * This must not be used for converting to non-interface types. Use igloo_ro_convert() for that.
 *
 * Parameters:
 *  self
 *      The object to convert.
 *  type
 *      The type of the interface requested.
 *  name, associated
 *      See igloo_ro_new().
 */
typedef igloo_ro_t (*igloo_ro_get_interface_t)(igloo_ro_t self, const igloo_ro_type_t *type, const char *name, igloo_ro_t associated);

/* Type used for callback called when the object needs to be converted to a string.
 *
 * This is used mostly for debugging or preseting the object to the user.
 * The callback is not expected to return a string that can be used to reconstruct the object.
 */
typedef char * (*igloo_ro_stringify_t)(igloo_ro_t self);

/* Type used as a result of a compare between objects.
 */
typedef enum {
    /* The objects could not be compared.
     * Common cases may be that when one is NULL and the other is not NULL
     * or when both are of difference types that can not be compared to each other
     * (e.g. a IO handle and a buffer.)
     */
    igloo_RO_CR__ERROR  = -1,
    /* Both objects represent the same thing but are not the same object.
     * See also igloo_RO_CR_SAME.
     */
    igloo_RO_CR_EQUAL   =  0,
    /* Both objects are distinct and no other relation can be applied. */
    igloo_RO_CR_NOTEQUAL,
    /* Object A is less than object B. */
    igloo_RO_CR_ALESSTHANB,
    /* Object A is greather than object B. */
    igloo_RO_CR_AGREATERTHANB,
    /* Both objects are the same.
     * That is then they share the same address in memory.
     */
    igloo_RO_CR_SAME
} igloo_ro_cr_t;

/* Type used for callback called then two objects must be compared.
 *
 * Object a is always a object that belongs to the type this callback was set for.
 * Objects a, and b are not igloo_RO_NULL.
 * Objects a, and b may be of diffrent type.
 * Objects a, and b are not the same (See igloo_RO_CR_SAME).
 */
typedef igloo_ro_cr_t (*igloo_ro_compare_t)(igloo_ro_t a, igloo_ro_t b);

/* Meta type used to defined types.
 * DO NOT use any of the members in here directly!
 */

/* ---[ PRIVATE ]--- */
/*
 * Those types are defined here as they must be known to the compiler.
 * Nobody should ever try to access them directly.
 */
struct igloo_ro_type_tag {
    /* Size of this control structure */
    size_t                      control_length;
    /* ABI version of this structure */
    int                         control_version;

    /* Total length of the objects to be created */
    size_t                      type_length;
    /* Name of type */
    const char *                type_name;

    /* STILL UNUSED: Parent type */
    const igloo_ro_type_t *     type_parent;

    /* Callback to be called on final free() */
    igloo_ro_free_t             type_freecb;
    /* Callback to be called by igloo_ro_new() */
    igloo_ro_new_t              type_newcb;

    /* Callback to be called by igloo_ro_clone() */
    igloo_ro_clone_t            type_clonecb;
    /* Callback to be called by igloo_ro_convert() */
    igloo_ro_convert_t          type_convertcb;
    /* Callback to be called by igloo_ro_get_interface() */
    igloo_ro_get_interface_t    type_get_interfacecb;
    /* Callback to be called by igloo_ro_stringify() */
    igloo_ro_stringify_t        type_stringifycb;
    /* Callback to be called by igloo_ro_compare() */
    igloo_ro_compare_t          type_comparecb;
};
struct igloo_ro_base_tag {
    /* Type of the object */
    const igloo_ro_type_t * type;
    /* Reference counters */
    size_t refc;
    size_t wrefc;
    /* Mutex for igloo_ro_*(). */
    igloo_mutex_t lock;
    /* Name of the object. */
    char * name;
    /* Associated objects */
    igloo_ro_t associated;
    /* STILL UNUSED: Instance objects */
    igloo_ro_t instance;
};
int igloo_ro_new__return_zero(igloo_ro_t self, const igloo_ro_type_t *type, va_list ap);
/* ---[ END PRIVATE ]--- */

igloo_ro_t      igloo_RO_TO_TYPE_raw(igloo_ro_t object, const igloo_ro_type_t *type);
#define igloo_RO_GET_TYPE_BY_SYMBOL(x)  (igloo_ro__type__ ## x)

#ifdef IGLOO_CTC_HAVE_TYPE_ATTRIBUTE_TRANSPARENT_UNION
#define igloo_RO__GETBASE(x)        (((igloo_ro_t)(x)).subtype__igloo_ro_base_t)
#define igloo_RO_NULL               ((igloo_ro_t)(igloo_ro_base_t*)NULL)
#define igloo_RO_IS_NULL(x)         (igloo_RO__GETBASE((x)) == NULL)
#define igloo_RO_TO_TYPE(x,type)    (((igloo_ro_t)igloo_RO_TO_TYPE_raw((x), igloo_RO_GET_TYPE_BY_SYMBOL(type))).subtype__ ## type)
#else
#define igloo_RO__GETBASE(x)        ((igloo_ro_base_t*)(x))
#define igloo_RO_NULL               NULL
#define igloo_RO_IS_NULL(x)         ((x) == NULL)
#define igloo_RO_TO_TYPE(x,type)    ((type*)igloo_RO_TO_TYPE_raw((x), igloo_RO_GET_TYPE_BY_SYMBOL(type)))
#endif

#define igloo_RO_GET_TYPE(x)        (igloo_RO__GETBASE((x)) == NULL ? NULL : igloo_RO__GETBASE((x))->type)
#define igloo_RO_GET_TYPENAME(x)    (igloo_RO_GET_TYPE((x)) == NULL ? NULL : igloo_RO_GET_TYPE((x))->type_name)
int             igloo_RO_IS_VALID_raw(igloo_ro_t object, const igloo_ro_type_t *type);
#define igloo_RO_IS_VALID(x,type)   igloo_RO_IS_VALID_raw((x), igloo_RO_GET_TYPE_BY_SYMBOL(type))
int             igloo_RO_HAS_TYPE_raw(igloo_ro_t object, const igloo_ro_type_t *type);
#define igloo_RO_HAS_TYPE(x,type)   igloo_RO_HAS_TYPE_raw((x), (type))

/* Create a new refobject
 * The type argument gives the type for the new object,
 * the name for the object is given by name, and
 * the associated refobject is given by associated.
 */

igloo_ro_t      igloo_ro_new__raw(const igloo_ro_type_t *type, const char *name, igloo_ro_t associated);
#define         igloo_ro_new_raw(type, name, associated)  igloo_RO_TO_TYPE(igloo_ro_new__raw(igloo_RO_GET_TYPE_BY_SYMBOL(type), (name), (associated)), type)

igloo_ro_t      igloo_ro_new__simple(const igloo_ro_type_t *type, const char *name, igloo_ro_t associated, ...);
#define         igloo_ro_new(type, ...)  igloo_RO_TO_TYPE(igloo_ro_new__simple(igloo_RO_GET_TYPE_BY_SYMBOL(type), NULL, igloo_RO_NULL, ## __VA_ARGS__), type)
#define         igloo_ro_new_ext(type, name, associated, ...)  igloo_RO_TO_TYPE(igloo_ro_new__simple(igloo_RO_GET_TYPE_BY_SYMBOL(type), (name), (associated), ## __VA_ARGS__), type)

/* This increases the reference counter of the object */
int             igloo_ro_ref(igloo_ro_t self);
/* This decreases the reference counter of the object.
 * If the object's reference counter reaches zero the object is freed.
 */
int             igloo_ro_unref(igloo_ro_t self);

/* This is the same as igloo_ro_ref() and igloo_ro_unref() but increases/decreases the weak reference counter. */
int             igloo_ro_weak_ref(igloo_ro_t self);
int             igloo_ro_weak_unref(igloo_ro_t self);

/* This gets the object's name */
const char *    igloo_ro_get_name(igloo_ro_t self);

/* This gets the object's associated object. */
igloo_ro_t      igloo_ro_get_associated(igloo_ro_t self);
int             igloo_ro_set_associated(igloo_ro_t self, igloo_ro_t associated);

/* Clone the given object returning a copy of it.
 *
 * This creates a copy of the passed object if possible.
 * The mode of copy is selected by the parameters required and allowed.
 * Both list flags of modes. Modes listed as required are adhered to.
 * However the application may select any additional mode from the allowed
 * flags.
 *
 * If both required and allowed are zero a set of default flags is applied
 * as allowed flags.
 *
 * Parameters:
 *  self
 *      The object to copy.
 *  required
 *      Flags that must be adhered to when coping.
 *  allowed
 *      Flags that are allowed to be used.
 *      Flags that are listed as required are automatically added the list of allowed flags.
 *  name, associated
 *      See igloo_ro_new().
 */
igloo_ro_t      igloo_ro_clone(igloo_ro_t self, igloo_ro_cf_t required, igloo_ro_cf_t allowed, const char *name, igloo_ro_t associated);

/* Make a copy of a object but converts it to another type as well.
 *
 * This makes makes a copy of the object that is also of a diffrent type.
 * If the type is the same as the type of self igloo_ro_clone() is called.
 *
 * Parameters:
 *  self
 *      The object to copy and convert.
 *  type
 *      The type to convert to.
 *  required, allowed
 *      See igloo_ro_clone().
 *  name, associated
 *      See igloo_ro_new().
 */
igloo_ro_t      igloo_ro_convert(igloo_ro_t self, const igloo_ro_type_t *type, igloo_ro_cf_t required, igloo_ro_cf_t allowed, const char *name, igloo_ro_t associated);

/* Request a specific interface from the object.
 *
 * This must not be used to convert the object. This can only be used for requesting a specific interface.
 *
 * The object may return the same object for the same requested interface multiple times.
 * However each time the same object is returned a new reference to it is returned.
 *
 * Parameters:
 *  self
 *      The object to copy and convert.
 *  type
 *      The interface requested.
 *  name, associated
 *      See igloo_ro_new().
 * Returns:
 *  An object that represents the given interface or igloo_RO_NULL.
 */
igloo_ro_t igloo_ro_get_interface(igloo_ro_t self, const igloo_ro_type_t *type, const char *name, igloo_ro_t associated);

/* Convert a object to a string.
 * This is used for debugging and presenting to the user.
 *
 * The resulting string can not be used to recreate the object.
 *
 * Parameters:
 *  self
 *      The object to convert to a string.
 * Returns:
 *  A string as allocated using malloc(3). The caller must call free(3).
 */
char *          igloo_ro_stringify(igloo_ro_t self);

/* Compare two objects.
 *
 * Parameters:
 *  a, b
 *      The objects to compare.
 * Returns:
 *  Thre result of the compare. See igloo_ro_cr_t.
 */
igloo_ro_cr_t   igloo_ro_compare(igloo_ro_t a, igloo_ro_t b);

#ifdef __cplusplus
}
#endif

#endif
