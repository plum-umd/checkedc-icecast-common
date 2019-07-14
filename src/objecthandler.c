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
#include <igloo/list.h>
#include <igloo/objecthandler.h>
#include <igloo/thread.h>
#include "private.h"

struct igloo_objecthandler_tag {
    igloo_interface_base(objecthandler)
    igloo_rwlock_t rwlock;

    /* filters */
    igloo_filter_t *filter_a;
    igloo_filter_t *filter_b;
    igloo_list_t   *filter_list;
};

static void __free(igloo_ro_t self)
{
    igloo_objecthandler_t *handler = igloo_RO_TO_TYPE(self, igloo_objecthandler_t);

    igloo_thread_rwlock_wlock(&(handler->rwlock));
    igloo_thread_rwlock_unlock(&(handler->rwlock));
    igloo_thread_rwlock_destroy(&(handler->rwlock));

    igloo_interface_base_free(self);
}

igloo_RO_PUBLIC_TYPE(igloo_objecthandler_t,
        igloo_RO_TYPEDECL_FREE(__free)
        );

igloo_objecthandler_t * igloo_objecthandler_new(const igloo_objecthandler_ifdesc_t *ifdesc, igloo_ro_t backend_object, void *backend_userdata, const char *name, igloo_ro_t associated)
{
    igloo_objecthandler_t *handler = igloo_interface_base_new(igloo_objecthandler_t, ifdesc, backend_object, backend_userdata, name, associated);

    if (!handler)
        return NULL;

    igloo_thread_rwlock_create(&(handler->rwlock));

    return handler;
}

igloo_filter_result_t igloo_objecthandler_handle(igloo_objecthandler_t *handler, igloo_ro_t object)
{
    igloo_filter_result_t result = igloo_FILTER_RESULT_PASS;
    int require_wlock = 0;

    if (!igloo_RO_IS_VALID(handler, igloo_objecthandler_t))
        return igloo_FILTER_RESULT_ERROR;

    if (!handler->ifdesc->handle)
        return igloo_FILTER_RESULT_ERROR;

    /* Accessing handler->ifdesc->is_thread_safe is thread-safe. handler->filter_list is not, so we will recheck that later. */
    if (!handler->ifdesc->is_thread_safe || handler->filter_list)
        require_wlock = 1;

    if (require_wlock) {
        igloo_thread_rwlock_wlock(&(handler->rwlock));
    } else {
        igloo_thread_rwlock_rlock(&(handler->rwlock));
        
        /* now re-check if we got a handler->filter_list now... If so, we re-lock.
         * This is no problem as we can run with wlock anyway and handler->filter_list may not
         * be reset to NULL as well.
         */

        if (handler->filter_list) {
            igloo_thread_rwlock_unlock(&(handler->rwlock));
            require_wlock = 1;
            igloo_thread_rwlock_wlock(&(handler->rwlock));
        }
    }

    /* Ok, now we are locked. Let's process input! */

    if (handler->filter_list) {
        igloo_list_iterator_storage_t iterator_storage;
        igloo_list_iterator_t *iterator = igloo_list_iterator_start(handler->filter_list, &iterator_storage, sizeof(iterator_storage));
        igloo_filter_t *filter;

        for (; !igloo_RO_IS_NULL(filter = igloo_RO_TO_TYPE(igloo_list_iterator_next(iterator), igloo_filter_t)); ) {
            result = igloo_filter_test(filter, object);
            igloo_ro_unref(filter);
            if (result != igloo_FILTER_RESULT_PASS) {
                igloo_list_iterator_end(iterator);
                igloo_thread_rwlock_unlock(&(handler->rwlock));
                return result;
            }
        }
        igloo_list_iterator_end(iterator);
    } else {
        if (handler->filter_a)
            result = igloo_filter_test(handler->filter_a, object);

        if (result == igloo_FILTER_RESULT_PASS && handler->filter_b)
            result = igloo_filter_test(handler->filter_b, object);

        if (result != igloo_FILTER_RESULT_PASS) {
            igloo_thread_rwlock_unlock(&(handler->rwlock));
            return result;
        }
    }

    result = handler->ifdesc->handle(igloo_INTERFACE_BASIC_CALL(handler), object);

    igloo_thread_rwlock_unlock(&(handler->rwlock));

    return result;
}

int igloo_objecthandler_push_filter(igloo_objecthandler_t *handler, igloo_filter_t *filter)
{
    int ret = -1;

    if (!igloo_RO_IS_VALID(handler, igloo_objecthandler_t) || !igloo_RO_IS_VALID(filter, igloo_filter_t))
        return -1;

    igloo_thread_rwlock_wlock(&(handler->rwlock));
    if (!handler->filter_list && handler->filter_a && handler->filter_b) {
        handler->filter_list = igloo_ro_new(igloo_list_t);
        if (!handler->filter_list) {
            igloo_thread_rwlock_unlock(&(handler->rwlock));
            return -1;
        }

        igloo_list_set_type(handler->filter_list, igloo_filter_t);
        igloo_list_preallocate(handler->filter_list, 3);

        igloo_list_push(handler->filter_list, handler->filter_a);
        igloo_ro_unref(handler->filter_a);
        handler->filter_a = NULL;
        igloo_list_push(handler->filter_list, handler->filter_b);
        igloo_ro_unref(handler->filter_b);
        handler->filter_b = NULL;
    }

    if (handler->filter_list) {
        ret = igloo_list_push(handler->filter_list, filter);
    } else {
        ret = igloo_ro_ref(filter);
        if (ret == 0) {
            if (!handler->filter_a) {
                handler->filter_a = filter;
            } else {
                handler->filter_b = filter;
            }
        }
    }
    igloo_thread_rwlock_unlock(&(handler->rwlock));

    return ret;
}

int igloo_objecthandler_flush(igloo_objecthandler_t *handler)
{
    int ret = 0;

    if (!igloo_RO_IS_VALID(handler, igloo_objecthandler_t))
        return -1;

    if (!handler->ifdesc->flush)
        return 0;

    if (handler->ifdesc->is_thread_safe) {
        igloo_thread_rwlock_wlock(&(handler->rwlock));
    } else {
        igloo_thread_rwlock_rlock(&(handler->rwlock));
    }

    ret = handler->ifdesc->flush(igloo_INTERFACE_BASIC_CALL(handler));

    igloo_thread_rwlock_unlock(&(handler->rwlock));

    return ret;
}

int igloo_objecthandler_set_backend(igloo_objecthandler_t *handler, igloo_ro_t backend)
{
    int ret = 0;

    if (!igloo_RO_IS_VALID(handler, igloo_objecthandler_t))
        return -1;

    if (!handler->ifdesc->set_backend)
        return -1;

    igloo_thread_rwlock_wlock(&(handler->rwlock));

    if (handler->ifdesc->flush)
        ret = handler->ifdesc->flush(igloo_INTERFACE_BASIC_CALL(handler));

    if (ret == 0)
        ret = handler->ifdesc->set_backend(igloo_INTERFACE_BASIC_CALL(handler), backend);

    igloo_thread_rwlock_unlock(&(handler->rwlock));

    return ret;
}
