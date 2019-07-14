/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2018,      Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>,
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "ctest_lib.h"

#include <igloo/logmsg.h>

static void test_create_unref(void)
{
    igloo_logmsg_t *msg;
    igloo_objecthandler_t *formater;
    igloo_filter_t *filter;

    msg = igloo_logmsg_new(NULL, igloo_RO_NULL, NULL, NULL, NULL, NULL, -1, NULL, igloo_LOGLEVEL__NONE, igloo_LOGMSG_OPT_NONE, NULL, "test");
    ctest_test("logmsg created", !igloo_RO_IS_NULL(msg));
    ctest_test("un-referenced", igloo_ro_unref(msg) == 0);

    formater = igloo_logmsg_formarter(igloo_RO_NULL, NULL, NULL, igloo_RO_NULL);
    ctest_test("formater created", !igloo_RO_IS_NULL(formater));
    ctest_test("un-referenced", igloo_ro_unref(formater) == 0);

    filter = igloo_logmsg_filter(igloo_LOGLEVEL__NONE, igloo_LOGLEVEL__NONE, igloo_LOGMSG_OPT_NONE, igloo_LOGMSG_OPT_NONE, NULL, NULL, NULL, NULL, igloo_RO_NULL);
    ctest_test("filter created", !igloo_RO_IS_NULL(filter));
    ctest_test("un-referenced", igloo_ro_unref(filter) == 0);
}

int main (void)
{
    ctest_init();

    test_create_unref();

    ctest_fin();

    return 0;
}
