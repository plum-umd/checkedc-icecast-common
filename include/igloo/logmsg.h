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

#ifndef _LIBIGLOO__LOGMSG_H_
#define _LIBIGLOO__LOGMSG_H_
/**
 * @file
 * Put a good description of this file here
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <igloo/config.h>

#ifdef IGLOO_CTC_HAVE_STDINT_H
#include <stdint.h>
#endif
#include <time.h>

#include <igloo/config.h>
#include "ro.h"
#include "list.h"

/* About thread safety:
 * This set of functions is thread safe.
 */

igloo_RO_FORWARD_TYPE(igloo_logmsg_t);

/* Log level for log messages */
typedef enum {
    /* Used to report errors with function calls. */
    IGLOO_LOGLEVEL__ERROR    = -1,
    /* Unset log level. */
    IGLOO_LOGLEVEL__NONE     =  0,
    /* Logmsg reports an error. */
    IGLOO_LOGLEVEL_ERROR,
    /* Logmsg reports a warning. */
    IGLOO_LOGLEVEL_WARN,
    /* Logmsg is of information level. */
    IGLOO_LOGLEVEL_INFO,
    /* Logmsg is for debugging only. */
    IGLOO_LOGLEVEL_DEBUG
} igloo_loglevel_t;

/* Type for logmsg options.
 */
#ifdef IGLOO_CTC_HAVE_STDINT_H
typedef uint_least32_t igloo_logmsg_opt_t;
#else
typedef unsigned long int igloo_logmsg_opt_t;
#endif

/* No options set. */
#define igloo_LOGMSG_OPT_NONE           ((igloo_logmsg_opt_t)0x000)
/* Logmsg is only useful for developing the software itself. */
#define igloo_LOGMSG_OPT_DEVEL          ((igloo_logmsg_opt_t)0x001)
/* Logmsg should be acknowledged by the user. */
#define igloo_LOGMSG_OPT_ASKACK         ((igloo_logmsg_opt_t)0x002)

/* This creates a new log message.
 * Parameters:
 *  name, associated
 *      See refobject_new().
 *  msgid
 *      Message ID used to correlate messages of the same type.
 *      Used e.g. with igloo_LOGMSG_OPT_ASKACK.
 *      Must be globally unique. Could be URI (e.g. UUID based URN)
 *      or message@domain.
 *  cat
 *      Message category/module.
 *  func
 *      Function generating the log message.
 *  codefile
 *      Code file generating the message.
 *  codeline
 *      Code line generating the message.
 *  ts
 *  	Timestamp of the message. If NULL a timestamp is generated.
 *  level
 *      Loglevel of the message.
 *  options
 *      Message and delivery options.
 *  referenced
 *      List of objects relevant to the context generating the message.
 *  format
 *      Format string for the log message.
 *  ...
 *      Parameters according to the format string.
 */
igloo_logmsg_t * igloo_logmsg_new(const char *name, igloo_ro_t associated, const char *msgid, const char *cat, const char *func, const char *codefile, const ssize_t codeline, const struct timespec * ts, igloo_loglevel_t level, igloo_logmsg_opt_t options, igloo_list_t *referenced, const char *format, ...);


int igloo_logmsg_get_context(igloo_logmsg_t *msg, const char **msgid, const char **cat, const char **func, const char **codefile, const ssize_t *codeline, struct timespec **ts);
int igloo_logmsg_get_message(igloo_logmsg_t *msg, igloo_loglevel_t *level, const char **string);
int igloo_logmsg_get_extra(igloo_logmsg_t *msg, igloo_logmsg_opt_t *options, igloo_list_t **list);

/* This creates a formater that allows writing of log messages to a logfile.
 * Parameters:
 *  backend
 *  	The backend to write data to. Must be a igloo_io_t handle.
 *  subformat
 *  	Subformat to use. NULL for default.
 *  	Must be NULL.
 *  name, associated
 *      See refobject_new().
 */
igloo_objecthandler_t 	* igloo_logmsg_formarter(igloo_ro_t backend, const char *subformat, const char *name, igloo_ro_t associated);

/* This creates a filter for log messages.
 * Parameters:
 *  level_min
 *  	Minimum log level.
 *  level_max
 *  	Maximum log leve.
 *  options_required
 *  	Options a message must have set.
 *  options_absent
 *  	Options a message must not have set.
 *  ts_min
 *  	Minimum timestamp or NULL.
 *  ts_max
 *  	Maximum timestamp or NULL.
 *  cat
 *      Message category/module or NULL.
 *  name, associated
 *      See refobject_new().
 */
igloo_filter_t 		* igloo_logmsg_filter(igloo_loglevel_t level_min, igloo_loglevel_t level_max, igloo_logmsg_opt_t options_required, igloo_logmsg_opt_t options_absent, const struct timespec * ts_min, const struct timespec * ts_max, const char *cat, const char *name, igloo_ro_t associated);

#ifdef __cplusplus
}
#endif

#endif /* ! _LIBIGLOO__LOGMSG_H_ */
