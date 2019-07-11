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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <igloo/logmsg.h>
#include <igloo/interface.h>
#include <igloo/objecthandler.h>
#include <igloo/filter.h>
#include <igloo/io.h>
#include "private.h"

#define LOG_MAXLINELEN 1024

struct igloo_logmsg_tag {
    igloo_ro_base_t __base;
    char *msgid;
    char *cat;
    char *func;
    char *codefile;
    ssize_t codeline;
    struct timespec ts;
    igloo_loglevel_t level;
    igloo_logmsg_opt_t options;
    igloo_list_t *referenced;
    char *string;
};

static void __free(igloo_ro_t self)
{
    igloo_logmsg_t *logmsg = igloo_RO_TO_TYPE(self, igloo_logmsg_t);
    free(logmsg->msgid);
    free(logmsg->cat);
    free(logmsg->func);
    free(logmsg->codefile);
    free(logmsg->string);
    igloo_ro_unref(logmsg->referenced);
}

igloo_RO_PUBLIC_TYPE(igloo_logmsg_t,
        igloo_RO_TYPEDECL_FREE(__free)
        );


igloo_logmsg_t * igloo_logmsg_new(const char *name, igloo_ro_t associated,
                                  const char *msgid,
                                  const char *cat,
                                  const char *func, const char *codefile, const ssize_t codeline,
                                  const struct timespec * ts,
                                  igloo_loglevel_t level, igloo_logmsg_opt_t options,
                                  igloo_list_t *referenced,
                                  const char *format, ...)
{
    igloo_logmsg_t *logmsg = igloo_ro_new_raw(igloo_logmsg_t, name, associated);
    va_list ap;
    char string[LOG_MAXLINELEN];

    if (!logmsg)
        return NULL;

    logmsg->codeline = codeline;
    logmsg->level = level;
    logmsg->options = options;

    va_start(ap, format);
    igloo_private__vsnprintf(string, sizeof(string), format, ap);
    va_end(ap);

    do {
#define __set_str(x) \
        if (x) { \
            logmsg->x = strdup((x)); \
            if (!logmsg->x) \
                break; \
        }

        __set_str(msgid);
        __set_str(cat);
        __set_str(func);
        __set_str(codefile);

        logmsg->string = strdup(string);
        if (!logmsg->string)
            break;

        if (ts) {
            logmsg->ts = *ts;
        } else {
            if (clock_gettime(CLOCK_REALTIME, &(logmsg->ts)) != 0)
                break;
        }


        if (referenced) {
            if (igloo_ro_ref(referenced) != 0)
                break;

            logmsg->referenced = referenced;
        }

        return logmsg;
    } while (0);

    igloo_ro_unref(logmsg);
    return NULL;
}


#define __SETSTRING(x) \
    if ((x)) { \
        *(x) = msg->x; \
    }

int igloo_logmsg_get_context(igloo_logmsg_t *msg, const char **msgid, const char **cat, const char **func, const char **codefile, ssize_t *codeline, struct timespec *ts)
{
    if (!msg)
        return -1;

    __SETSTRING(msgid);
    __SETSTRING(cat);
    __SETSTRING(func);
    __SETSTRING(codefile);
    __SETSTRING(codeline);
    __SETSTRING(ts);

    return 0;
}

int igloo_logmsg_get_message(igloo_logmsg_t *msg, igloo_loglevel_t *level, const char **string)
{
    if (!msg)
        return -1;

    if (level)
        *level = msg->level;

    __SETSTRING(string);

    return 0;
}

int igloo_logmsg_get_extra(igloo_logmsg_t *msg, igloo_logmsg_opt_t *options, igloo_list_t **referenced)
{
    if (!msg)
        return -1;

    if (options)
        *options = msg->options;

    if (referenced) {
        if (msg->referenced) {
            if (igloo_ro_ref(msg->referenced) != 0)
                return -1;

            *referenced = msg->referenced;
        } else {
            *referenced = NULL;
        }
    }

    return 0;
}

static const char * __level2str(igloo_loglevel_t level)
{
    switch (level) {
        case IGLOO_LOGLEVEL__ERROR: return "<<<ERROR>>>"; break;
        case IGLOO_LOGLEVEL__NONE: return "NONE"; break;
        case IGLOO_LOGLEVEL_ERROR: return "EROR"; break;
        case IGLOO_LOGLEVEL_WARN: return "WARN"; break;
        case IGLOO_LOGLEVEL_INFO: return "INFO"; break;
        case IGLOO_LOGLEVEL_DEBUG: return "DBUG"; break;
    }

    return "<<<unknown log level>>>";
}

typedef enum {
    igloo_FST_FULL,
    igloo_FST_OLD,
    igloo_FST_NORMAL = igloo_FST_OLD
} igloo_logmsg_formarter_subtype_t;

static igloo_filter_result_t __handle(igloo_INTERFACE_BASIC_ARGS, igloo_ro_t object)
{
    igloo_logmsg_t *msg = igloo_RO_TO_TYPE(object, igloo_logmsg_t);
    igloo_logmsg_formarter_subtype_t sf = *(igloo_logmsg_formarter_subtype_t*)*backend_userdata;
    const char *level = NULL;
    time_t now;
    char pre[256+LOG_MAXLINELEN] = "";
    int datelen;
    char flags[3] = "  ";

    if (!msg)
        return igloo_FILTER_RESULT_DROP;

    level = __level2str(msg->level);

    switch (sf) {
        case igloo_FST_FULL:
            if (msg->options & igloo_LOGMSG_OPT_DEVEL)
                flags[0] = 'D';

            if (msg->options & igloo_LOGMSG_OPT_ASKACK)
                flags[1] = 'A';

            now = msg->ts.tv_sec;
            datelen = strftime(pre, sizeof(pre), "[%Y-%m-%d  %H:%M:%S UTC]", gmtime(&now));
            snprintf(pre+datelen, sizeof(pre)-datelen, " (%s) %s [%s] %s/%s(%s:%zi) %s\n", (msg->msgid ? msg->msgid : ""), level, flags, msg->cat, msg->func, msg->codefile, msg->codeline, msg->string);
        break;
        case igloo_FST_OLD:
            now = msg->ts.tv_sec;
            datelen = strftime(pre, sizeof(pre), "[%Y-%m-%d  %H:%M:%S]", localtime(&now));
            snprintf(pre+datelen, sizeof(pre)-datelen, " %s %s/%s %s\n", level, msg->cat, msg->func, msg->string);
        break;
    }

    igloo_io_write(igloo_RO_TO_TYPE(*backend_object, igloo_io_t), pre, strlen(pre));

    return igloo_FILTER_RESULT_PASS;
}

static const igloo_objecthandler_ifdesc_t igloo_logmsg_formarter_ifdesc = {
    igloo_INTERFACE_DESCRIPTION_BASE(igloo_objecthandler_ifdesc_t),
    .is_thread_safe = 1,
    .handle = __handle
};

igloo_objecthandler_t   * igloo_logmsg_formarter(igloo_ro_t backend, const char *subformat, const char *name, igloo_ro_t associated)
{
    igloo_logmsg_formarter_subtype_t *sf = NULL;
    igloo_objecthandler_t *objecthandler;

    if (!igloo_RO_IS_VALID(backend, igloo_io_t))
        return NULL;

    if (!subformat || strcmp(subformat, "default") == 0)
        subformat = "normal";

    sf = malloc(sizeof(*sf));
    if (!sf)
        return NULL;

    if (strcmp(subformat, "normal") == 0) {
        *sf = igloo_FST_NORMAL;
    } else if (strcmp(subformat, "full") == 0) {
        *sf = igloo_FST_FULL;
    } else if (strcmp(subformat, "old") == 0) {
        *sf = igloo_FST_OLD;
    } else {
        free(sf);
        return NULL;
    }

    objecthandler = igloo_objecthandler_new(&igloo_logmsg_formarter_ifdesc, backend, sf, name, associated);
    if (!objecthandler) {
        free(sf);
    }

    return objecthandler;
}

typedef struct {
    igloo_loglevel_t level_min;
    igloo_loglevel_t level_max;
    igloo_logmsg_opt_t options_required;
    igloo_logmsg_opt_t options_absent;
    struct timespec ts_min;
    struct timespec ts_max;
    char *cat;
} igloo_logmsg_filter_mask_t;

static inline int __gt_timespec(struct timespec *a, struct timespec *b)
{
    return a->tv_sec > b->tv_sec || (a->tv_sec == b->tv_sec && a->tv_nsec > b->tv_nsec);
}

static igloo_filter_result_t __test(igloo_INTERFACE_BASIC_ARGS, igloo_ro_t object)
{
    igloo_logmsg_t *msg = igloo_RO_TO_TYPE(object, igloo_logmsg_t);
    igloo_logmsg_filter_mask_t *mask = *backend_userdata;

    if (!msg)
        return igloo_FILTER_RESULT_DROP;

    if (msg->level < mask->level_min || msg->level > mask->level_max)
        return igloo_FILTER_RESULT_DROP;

    if ((msg->options & mask->options_required) != mask->options_required || (msg->options & mask->options_absent))
        return igloo_FILTER_RESULT_DROP;

    if (__gt_timespec(&(mask->ts_min), &(msg->ts)) || __gt_timespec(&(msg->ts), &(mask->ts_max)))
        return igloo_FILTER_RESULT_DROP;

    if (mask->cat) {
        if (!msg->cat)
            return igloo_FILTER_RESULT_DROP;

        if (strcmp(msg->cat, mask->cat) != 0)
            return igloo_FILTER_RESULT_DROP;
    }

    return igloo_FILTER_RESULT_PASS;
}

static const igloo_filter_ifdesc_t igloo_logmsg_filter_ifdesc = {
    igloo_INTERFACE_DESCRIPTION_BASE(igloo_filter_ifdesc_t),
    .test = __test
};

igloo_filter_t          * igloo_logmsg_filter(igloo_loglevel_t level_min, igloo_loglevel_t level_max, igloo_logmsg_opt_t options_required, igloo_logmsg_opt_t options_absent, const struct timespec * ts_min, const struct timespec * ts_max, const char *cat, const char *name, igloo_ro_t associated)
{
    igloo_filter_t *filter;
    igloo_logmsg_filter_mask_t *mask = calloc(1, sizeof(*mask));

    if (!mask)
        return NULL;

    mask->level_min = level_min;
    mask->level_max = level_max;
    mask->options_required = options_required;
    mask->options_absent = options_absent;

    /* No else needed, as {0, 0} is already in the past. */
    if (ts_min)
        mask->ts_min = *ts_min;

    if (ts_max) {
        mask->ts_max = *ts_max;
    } else {
        /* BEFORE YEAR 2038 IMPORTANT REWRITE: Update date. */
        mask->ts_max.tv_sec = 2145916800; /* this is 2038-01-01 */
    }

    if (cat) {
        mask->cat = strdup(cat);
        if (!mask->cat) {
            free(mask);
            return NULL;
        }
    }

    filter = igloo_filter_new(&igloo_logmsg_filter_ifdesc, igloo_RO_NULL, mask, name, associated);
    if (!filter) {
        free(mask);
    }
    return filter;
}
