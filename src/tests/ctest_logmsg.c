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
#include <igloo/filter.h>

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

static void test_logmsg(void)
{
    static const struct timespec tv_in = {.tv_sec = -4242134, .tv_nsec = 1234789};
    igloo_logmsg_t *msg;
    const char *msgid_out, *cat_out, *func_out, *codefile_out, *string_out;
    ssize_t codeline_out;
    struct timespec tv_out;
    igloo_loglevel_t level_out;
    igloo_logmsg_opt_t options_out;
    igloo_list_t *referenced_out;
    int ret;

    msg = igloo_logmsg_new("name", igloo_RO_NULL, "msgid", "cat", "func", "codefile", 13374242, &tv_in, igloo_LOGLEVEL_INFO, igloo_LOGMSG_OPT_ASKACK, NULL, "test %i %s", 5, "msg");
    ctest_test("logmsg created", !igloo_RO_IS_NULL(msg));

    ctest_test("got context", (ret = igloo_logmsg_get_context(msg, &msgid_out, &cat_out, &func_out, &codefile_out, &codeline_out, &tv_out)) == 0);
    if (ret == 0) {
        ctest_test("got msgid", msgid_out != NULL && strcmp(msgid_out, "msgid") == 0);
        ctest_test("got cat", cat_out != NULL && strcmp(cat_out, "cat") == 0);
        ctest_test("got func", func_out != NULL && strcmp(func_out, "func") == 0);
        ctest_test("got codefile", codefile_out != NULL && strcmp(codefile_out, "codefile") == 0);
        ctest_test("got codeline", codeline_out == 13374242);
        ctest_test("got ts", tv_out.tv_sec == -4242134 && tv_out.tv_nsec == 1234789);
    } else {
        ctest_test("got msgid", 0);
        ctest_test("got cat", 0);
        ctest_test("got func", 0);
        ctest_test("got codefile", 0);
        ctest_test("got codeline", 0);
        ctest_test("got ts", 0);
    }

    ctest_test("got message", (ret = igloo_logmsg_get_message(msg, &level_out, &string_out)) == 0);
    if (ret == 0) {
        ctest_test("got level", level_out == igloo_LOGLEVEL_INFO);
        ctest_test("got string", string_out != NULL && strcmp(string_out, "test 5 msg") == 0);
    } else {
        ctest_test("got level", 0);
        ctest_test("got string", 0);
    }

    ctest_test("got extra", (ret = igloo_logmsg_get_extra(msg, &options_out, &referenced_out)) == 0);
    if (ret == 0) {
        ctest_test("got options", options_out == igloo_LOGMSG_OPT_ASKACK);
        ctest_test("got referenced", referenced_out == NULL);
    } else {
        ctest_test("got options", 0);
        ctest_test("got referenced", 0);
    }

    ctest_test("un-referenced", igloo_ro_unref(msg) == 0);
}

static void test_filter(void)
{
    igloo_filter_t *filter;
    igloo_logmsg_t *msg;
    igloo_ro_base_t *base;

    filter = igloo_logmsg_filter(igloo_LOGLEVEL_ERROR, igloo_LOGLEVEL_WARN, igloo_LOGMSG_OPT_NONE, igloo_LOGMSG_OPT_NONE, NULL, NULL, NULL, NULL, igloo_RO_NULL);
    ctest_test("filter created", !igloo_RO_IS_NULL(filter));

    base = igloo_ro_new(igloo_ro_base_t);
    ctest_test("base created", base != NULL);
    ctest_test("droping base", igloo_filter_test(filter, base) == igloo_FILTER_RESULT_DROP);
    ctest_test("base un-referenced", igloo_ro_unref(base) == 0);

    msg = igloo_logmsg_new(NULL, igloo_RO_NULL, NULL, NULL, NULL, NULL, -1, NULL, igloo_LOGLEVEL_INFO, igloo_LOGMSG_OPT_NONE, NULL, "test");
    ctest_test("logmsg created", !igloo_RO_IS_NULL(msg));
    ctest_test("droping logmsg", igloo_filter_test(filter, msg) == igloo_FILTER_RESULT_DROP);
    ctest_test("un-referenced", igloo_ro_unref(msg) == 0);

    msg = igloo_logmsg_new(NULL, igloo_RO_NULL, NULL, NULL, NULL, NULL, -1, NULL, igloo_LOGLEVEL_WARN, igloo_LOGMSG_OPT_NONE, NULL, "test");
    ctest_test("logmsg created", !igloo_RO_IS_NULL(msg));
    ctest_test("passing logmsg", igloo_filter_test(filter, msg) == igloo_FILTER_RESULT_PASS);
    ctest_test("un-referenced", igloo_ro_unref(msg) == 0);

    ctest_test("un-referenced", igloo_ro_unref(filter) == 0);
}

int main (void)
{
    ctest_init();

    test_create_unref();

    test_logmsg();
    test_filter();

    ctest_fin();

    return 0;
}
