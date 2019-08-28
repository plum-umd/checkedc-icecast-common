/* Httpp.c
**
** http parsing engine
**
** Copyright (C) 2014 Michael Smith <msmith@icecast.org>,
**                    Ralph Giles <giles@xiph.org>,
**                    Ed "oddsock" Zaleski <oddsock@xiph.org>,
**                    Karl Heyes <karl@xiph.org>,
** Copyright (C) 2012-2019 by Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Library General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.
**
** You should have received a copy of the GNU Library General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
** Boston, MA  02110-1301, USA.
**
*/

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

#include <stdio_checked.h>

#include <stdlib_checked.h>
#include <string_checked.h>
#include <ctype.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <avl/avl.h>
#include "httpp.h"

#define MAX_HEADERS 32

/* internal functions */

/* misc */
static char *_lowercase(_Array_ptr<char> str) : itype(_Array_ptr<char> ) ;

/* for avl trees */
static int _compare_vars(void* compare_arg, void *a, void *b);
static int _free_vars(void *key);

/* For avl tree manipulation */
static void parse_query(_Ptr<avl_tree> tree, _Nt_array_ptr<const char> query : count(len) , size_t len);
static const char *_httpp_get_param(_Ptr<avl_tree> tree, const char *name) : itype(_Nt_array_ptr<const char> ) ;
static void _httpp_set_param_nocopy(_Ptr<avl_tree> tree, char *name, _Nt_array_ptr<char> value, int replace);
static void _httpp_set_param(_Ptr<avl_tree> tree, _Nt_array_ptr<const char> name, _Nt_array_ptr<const char> value);
static http_var_t *_httpp_get_param_var(_Ptr<avl_tree> tree, const char *name) : itype(_Ptr<http_var_t> ) ;

httpp_request_info_t httpp_request_info(httpp_request_type_e req)
{
#if 0
#define HTTPP_REQUEST_IS_SAFE                       ((httpp_request_info_t)0x0001U)
#define HTTPP_REQUEST_IS_IDEMPOTENT                 ((httpp_request_info_t)0x0002U)
#define HTTPP_REQUEST_IS_CACHEABLE                  ((httpp_request_info_t)0x0004U)
#define HTTPP_REQUEST_HAS_RESPONSE_BODY             ((httpp_request_info_t)0x0010U)
#define HTTPP_REQUEST_HAS_REQUEST_BODY              ((httpp_request_info_t)0x0100U)
#define HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY     ((httpp_request_info_t)0x0200U)
#endif
    switch (req) {
        /* offical methods */
        case httpp_req_get:
            return HTTPP_REQUEST_IS_SAFE|HTTPP_REQUEST_IS_IDEMPOTENT|HTTPP_REQUEST_IS_CACHEABLE|HTTPP_REQUEST_HAS_RESPONSE_BODY|HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY;
        break;
        case httpp_req_post:
            return HTTPP_REQUEST_IS_CACHEABLE|HTTPP_REQUEST_HAS_RESPONSE_BODY|HTTPP_REQUEST_HAS_REQUEST_BODY;
        break;
        case httpp_req_put:
            return HTTPP_REQUEST_IS_IDEMPOTENT|HTTPP_REQUEST_HAS_RESPONSE_BODY|HTTPP_REQUEST_HAS_REQUEST_BODY;
        break;
        case httpp_req_head:
            return HTTPP_REQUEST_IS_SAFE|HTTPP_REQUEST_IS_IDEMPOTENT|HTTPP_REQUEST_IS_CACHEABLE;
        break;
        case httpp_req_options:
            return HTTPP_REQUEST_IS_SAFE|HTTPP_REQUEST_IS_IDEMPOTENT|HTTPP_REQUEST_HAS_RESPONSE_BODY|HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY;
        break;
        case httpp_req_delete:
            return HTTPP_REQUEST_IS_IDEMPOTENT|HTTPP_REQUEST_HAS_RESPONSE_BODY;
        break;
        case httpp_req_trace:
            return HTTPP_REQUEST_IS_SAFE|HTTPP_REQUEST_IS_IDEMPOTENT|HTTPP_REQUEST_HAS_RESPONSE_BODY;
        break;
        case httpp_req_connect:
            return HTTPP_REQUEST_HAS_RESPONSE_BODY|HTTPP_REQUEST_HAS_REQUEST_BODY;
        break;

        /* Icecast specific methods */
        case httpp_req_source:
            return HTTPP_REQUEST_HAS_RESPONSE_BODY|HTTPP_REQUEST_HAS_REQUEST_BODY;
        break;
        case httpp_req_play:
            return HTTPP_REQUEST_IS_SAFE|HTTPP_REQUEST_HAS_RESPONSE_BODY|HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY;
        break;
        case httpp_req_stats:
            return HTTPP_REQUEST_IS_SAFE|HTTPP_REQUEST_HAS_RESPONSE_BODY|HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY;
        break;

        /* Virtual and other methods */
        case httpp_req_none:
        case httpp_req_unknown:
        default:
            return HTTPP_REQUEST_HAS_RESPONSE_BODY|HTTPP_REQUEST_HAS_OPTIONAL_REQUEST_BODY;
        break;
    }
}

_Ptr<http_parser_t> httpp_create_parser(void)
{
    _Ptr<http_parser_t> parser =  calloc(1, sizeof(http_parser_t));

    parser->refc = 1;
    parser->req_type = httpp_req_none;
    parser->uri = NULL;
    parser->vars = avl_tree_new(_compare_vars, NULL);
    parser->queryvars = avl_tree_new(_compare_vars, NULL);
    parser->postvars = avl_tree_new(_compare_vars, NULL);

    return parser;
}

void httpp_initialize(_Ptr<http_parser_t> parser, _Ptr<http_varlist_t> defaults)
{
    _Ptr<http_varlist_t> list = NULL;

    /* now insert the default variables */
    list = defaults;
    while (list != NULL) {
        size_t i;

        for (i = 0; i < list->var.values; i++) {
            httpp_setvar(parser, list->var.name, list->var.value[i]);
        }

        list = list->next;
    }
}

static int split_headers(_Nt_array_ptr<char> data : count(len), unsigned long len, _Array_ptr<_Nt_array_ptr<char>> line : count(MAX_HEADERS))
{
    /* first we count how many lines there are 
    ** and set up the line[] array     
    */
    int lines = 0;
    unsigned long i;
    line[lines] = data;
    for (i = 0; i < len && lines < MAX_HEADERS; i++) {
        if (data[i] == '\r')
            data[i] = '\0';
        if (data[i] == '\n') {
            lines++;
            data[i] = '\0';
            if (lines >= MAX_HEADERS)
                return MAX_HEADERS;
            if (i + 1 < len) {
                if (data[i + 1] == '\n' || data[i + 1] == '\r')
                    break;
                line[lines] = (_Nt_array_ptr<char>)&data[i + 1]; /* Hasan: The compiler should be able to handle this assignment, but I have to use a cast for now. Prod    uces warning, need 'where' clause to provide bounds */
            }
        }
    }

    i++;
    while (i < len && data[i] == '\n') i++;

    return lines;
}

static void parse_headers(_Ptr<http_parser_t> parser, _Array_ptr<_Nt_array_ptr<char>> line : count(lines) , int lines)
{
    int i, l;
    int whitespace, slen;
    _Nt_array_ptr<char> name =  NULL;
    _Nt_array_ptr<char> value =  NULL;

    /* parse the name: value lines. */
    for (l = 1; l < lines; l++) {
        whitespace = 0;
        name = line[l];
        value = NULL;
        slen = strlen(line[l]);
        for (i = 0; i < slen; i++) {
            if (line[l][i] == ':') {
                whitespace = 1;
                line[l][i] = '\0';
            } else {
                if (whitespace) {
                    whitespace = 0;
                    while (i < slen && line[l][i] == ' ')
                        i++;

                    if (i < slen)
                        value = (_Nt_array_ptr<char>)&line[l][i]; /* Hasan: The compiler should be able to handle this assignment, but I have to use a cast for now. Produces warning, need 'where' clause to provide bounds */
                    
                    break;
                }
            }
        }
        
        if (name != NULL && value != NULL) {
            httpp_setvar(parser, _lowercase(name), value);
            name = NULL; 
            value = NULL;
        }
    }
}

int httpp_parse_response(_Ptr<http_parser_t> parser, _Ptr<const char> http_data, unsigned long len, const char *uri : itype(_Nt_array_ptr<const char> ) )
{
    _Nt_array_ptr<char> data : count(len + 1) = NULL;
    _Nt_array_ptr<char> line _Checked[MAX_HEADERS] = {0}; /* Hasan: The tool erased the MAX_HEADERS variable */
    int lines, slen,i, whitespace=0, where=0,code;
   _Nt_array_ptr<char> version = NULL; /* Hasan: The NULL macro was expanded to ((void *)0) during the conversion process */
_Nt_array_ptr<char> resp_code = NULL; /* Hasan: The NULL macro was expanded to ((void *)0) during the conversion process */
_Nt_array_ptr<char> message = NULL; /* Hasan: The NULL macro was expanded to ((void *)0) during the conversion process */
 
    
    if(http_data == NULL)
        return 0;

    /* make a local copy of the data, including 0 terminator */
    data = (_Nt_array_ptr<char>)malloc(len+1);
    if (data == NULL) return 0;
    memcpy(data, http_data, len);
    data[len] = 0;

    lines = split_headers(data, len, line);

    /* In this case, the first line contains:
     * VERSION RESPONSE_CODE MESSAGE, such as HTTP/1.0 200 OK
     */
    slen = strlen(line[0]);
    version = line[0];
    for(i=0; i < slen; i++) {
        if(line[0][i] == ' ') {
            line[0][i] = 0;
            whitespace = 1;
        } else if(whitespace) {
            whitespace = 0;
            where++;
            if(where == 1)
                resp_code = (_Nt_array_ptr<char>)&line[0][i]; /* Hasan: The compiler should be able to handle this assignment, but I have to use a cast for now. Prod    uces warning, need 'where' clause to provide bounds */
            else {
                message = (_Nt_array_ptr<char>)&line[0][i]; /* Hasan: The compiler should be able to handle this assignment, but I have to use a cast for now. Prod    uces warning, need 'where' clause to provide bounds */
                break;
            }
        }
    }

    if(version == NULL || resp_code == NULL || message == NULL) {
        free(data);
        return 0;
    }

    httpp_setvar(parser, HTTPP_VAR_ERROR_CODE, resp_code);
    code = atoi(resp_code);
    if(code < 200 || code >= 300) {
        httpp_setvar(parser, HTTPP_VAR_ERROR_MESSAGE, message);
    }

    httpp_setvar(parser, HTTPP_VAR_URI, uri);
    httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "NONE");

    parse_headers(parser, line, lines);

    free(data);

    return 1;
}

int httpp_parse_postdata(_Ptr<http_parser_t> parser, _Nt_array_ptr<const char> body_data : count(len), size_t len)
{
    const char *header = httpp_getvar(parser, "content-type");

    if (strcasecmp(header, "application/x-www-form-urlencoded") != 0) {
        return -1;
    }

    parse_query(parser->postvars, body_data, len);

    return 0;
}

static int hex(char c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    else if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else
        return -1;
}

static _Nt_array_ptr<char> url_unescape(_Array_ptr<const char> src : count(len), size_t len)
{
    //unsigned char *decoded;
    _Nt_array_ptr<char> decoded : byte_count(len + 1) = (_Nt_array_ptr<char>)calloc(1, len + 1); /* Hasan: Cast from 'calloc' causes warning. Should use Checked calloc */
    size_t i;
    //char *dst;
    _Nt_array_ptr<char> dst : bounds(decoded, ((char*)decoded) + len + 1) = decoded;
    int done = 0;

    //decoded = calloc(1, len + 1);

    //dst = (char *)decoded;

    for(i=0; i < len; i++) {
        switch(src[i]) {
        case '%':
            if(i+2 >= len) {
                free(decoded);
                return NULL;
            }
            if(hex(src[i+1]) == -1 || hex(src[i+2]) == -1 ) {
                free(decoded);
                return NULL;
            }

            *dst++ = hex(src[i+1]) * 16  + hex(src[i+2]);
            i+= 2;
            break;
        case '+':
            *dst++ = ' ';
            break;
        case '#':
            done = 1;
            break;
        case 0:
            free(decoded);
            return NULL;
            break;
        default:
            *dst++ = src[i];
            break;
        }
        if(done)
            break;
    }

    *dst = 0; /* null terminator */

    return decoded;
}

static void parse_query_element(_Ptr<avl_tree> tree, _Nt_array_ptr<const char> start : bounds(start, end), _Nt_array_ptr<const char> mid, _Nt_array_ptr<const char> end)
{
    size_t keylen;
    _Nt_array_ptr<char> key : count(keylen + 1) = NULL;
    size_t valuelen;
    _Nt_array_ptr<char> value : count(valuelen) = NULL;

    if (start >= end)
        return;

    if (!mid)
        return;

    keylen = mid - start;
    valuelen = end - mid - 1;

    /* REVIEW: We ignore keys with empty values. */
    if (!keylen || !valuelen)
        return;

    key = (_Nt_array_ptr<char>)malloc(keylen + 1);
    memcpy(key, start, keylen);
    key[keylen] = 0;

    value = url_unescape(mid + 1, valuelen);

    _httpp_set_param_nocopy(tree, (char*)key, value, 0); /* Hasan: TODO: See why the second parameter of '_httpp_set_param_nocopy' is wild */
}

static void parse_query(_Ptr<avl_tree> tree, _Nt_array_ptr<const char> query : count(len) , size_t len)
{
    _Nt_array_ptr<const char> start =  query;
    _Nt_array_ptr<const char> mid =  NULL;
    size_t i;

    if (!query || !*query)
        return;

    for (i = 0; i < len; i++) {
        switch (query[i]) {
            case '&':
                parse_query_element(tree, start, mid, (_Nt_array_ptr<char>)&(query[i])); /* Hasan: The cast is a result of a compiler mishandeling of the '&' operator */
                start = (_Nt_array_ptr<char>)&(query[i + 1]); /* Hasan: The cast is a result of a compiler mishandeling of the '&' operator */
                mid = NULL;
            break;
            case '=':
                mid = (_Nt_array_ptr<char>)&(query[i]); /* Hasan: The cast is a result of a compiler mishandeling of the '&' operator */
            break;
        }
    }

    parse_query_element(tree, start, mid, (_Nt_array_ptr<char>)&(query[i])); /* Hasan: The cast is a result of a compiler mishandeling of the '&' operator */
}

int httpp_parse(_Ptr<http_parser_t> parser, _Ptr<const char> http_data , unsigned long len)
{
   _Nt_array_ptr<char> data : count(len + 1) = NULL;
_Nt_array_ptr<char> tmp = NULL;
 
    _Nt_array_ptr<char> line _Checked[MAX_HEADERS] = {}; /* limited to 32 lines, should be more than enough */
    int i;
    int lines;
    _Nt_array_ptr<char> req_type = NULL;
    _Nt_array_ptr<char> uri =  NULL;
    _Nt_array_ptr<char> version =  NULL;
    int whitespace, where, slen;
    size_t call_for_strlen; /* Hasan: Compiler doesn't like function calls in arguments, so made temp variable */

    if (http_data == NULL)
        return 0;

    /* make a local copy of the data, including 0 terminator */
    data = (_Nt_array_ptr<char>)malloc(len+1); /* Hasan: TODO: Use Checked malloc */
    if (data == NULL) return 0;
    memcpy(data, http_data, len);
    data[len] = 0;

    lines = split_headers(data, len, line);

    /* parse the first line special
    ** the format is:
    ** REQ_TYPE URI VERSION
    ** eg:
    ** GET /index.html HTTP/1.0
    */
    where = 0;
    whitespace = 0;
    slen = strlen(line[0]);
    req_type = line[0];
    for (i = 0; i < slen; i++) {
        if (line[0][i] == ' ') {
            whitespace = 1;
            line[0][i] = '\0';
        } else {
            /* we're just past the whitespace boundry */
            if (whitespace) {
                whitespace = 0;
                where++;
                switch (where) {
                    case 1:
                        uri = (_Nt_array_ptr<char>)&line[0][i]; /* Hasan: Compiler issue with '&' */
                    break;
                    case 2:
                        version = (_Nt_array_ptr<char>)&line[0][i]; /* Hasan: Compiler issue with '&' */
                    break;
                    case 3:
                        /* There is an extra element in the request line. This is not HTTP. */
                        free(data);
                        return 0;
                    break;
                }
            }
        }
    }

    parser->req_type = httpp_str_to_method((const char*)req_type); /* Hasan: TODO: See if this parameter can be changed to _Nt_array */

    if (uri != NULL && strlen(uri) > 0) {
        _Nt_array_ptr<char> query = NULL;
        if((query = (_Nt_array_ptr<char>)strchr(uri, '?')) != NULL) { /* Hasan: Cast is because of itype. Shouldn't be needed if in checked scope */
            httpp_setvar(parser, HTTPP_VAR_RAWURI, uri);
            httpp_setvar(parser, HTTPP_VAR_QUERYARGS, query);
            *query = 0;
            query++;
	    call_for_strlen = strlen(query); /* Hasan: Compiler doesn't like function calls in arguments, so made temp variable */
            parse_query(parser->queryvars, query, call_for_strlen);
        }

        parser->uri = (_Nt_array_ptr<char>)strdup(uri); /* Hasan: Cast is because of itype. Shouldn't be needed if in checked scope */
    } else {
        free(data);
        return 0;
    }

    if ((version != NULL) && ((tmp = (_Nt_array_ptr<char>)strchr(version, '/')) != NULL)) { /* Hasan: Cast is because of itype. Shouldn't be needed if in checked scope */
        tmp[0] = '\0';
        if ((strlen(version) > 0) && (strlen(&tmp[1]) > 0)) {
            httpp_setvar(parser, HTTPP_VAR_PROTOCOL, version);
            httpp_setvar(parser, HTTPP_VAR_VERSION, &tmp[1]);
        } else {
            free(data);
            return 0;
        }
    } else {
        free(data);
        return 0;
    }

    if (parser->req_type != httpp_req_none && parser->req_type != httpp_req_unknown) {
        switch (parser->req_type) {
        case httpp_req_get:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "GET");
            break;
        case httpp_req_post:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "POST");
            break;
        case httpp_req_put:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "PUT");
            break;
        case httpp_req_head:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "HEAD");
            break;
        case httpp_req_options:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "OPTIONS");
            break;
        case httpp_req_delete:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "DELETE");
            break;
        case httpp_req_trace:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "TRACE");
            break;
        case httpp_req_connect:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "CONNECT");
            break;
        case httpp_req_source:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "SOURCE");
            break;
        case httpp_req_play:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "PLAY");
            break;
        case httpp_req_stats:
            httpp_setvar(parser, HTTPP_VAR_REQ_TYPE, "STATS");
            break;
        default:
            break;
        }
    } else {
        free(data);
        return 0;
    }

    if (parser->uri != NULL) {
        httpp_setvar(parser, HTTPP_VAR_URI, parser->uri);
    } else {
        free(data);
        return 0;
    }

    parse_headers(parser, line, lines);

    free(data);

    return 1;
}

void httpp_deletevar(_Ptr<http_parser_t> parser, const char *name)
{
    http_var_t var = {};

    if (parser == NULL || name == NULL)
        return;
    memset(&var, 0, sizeof(var));

    var.name = (char*)name;

    avl_delete(parser->vars, (void *)&var, _free_vars);
}

void httpp_setvar(_Ptr<http_parser_t> parser, const char *name : itype(_Nt_array_ptr<const char> ) , const char *value : itype(_Nt_array_ptr<const char> ) )
{
    http_var_t *var;

    if (name == NULL || value == NULL)
        return;

    var = (http_var_t *)calloc(1, sizeof(http_var_t));
    if (var == NULL) return;

    var->value = calloc(1, sizeof(*var->value));
    if (!var->value) {
        free(var);
        return;
    }

    var->name = strdup(name);
    var->values = 1;
    var->value[0] = (_Nt_array_ptr<char>)strdup(value); /* Hasan: Cast due to itype */

    if (httpp_getvar(parser, name) == NULL) {
        avl_insert(parser->vars, (void *)var);
    } else {
        avl_delete(parser->vars, (void *)var, _free_vars);
        avl_insert(parser->vars, (void *)var);
    }
}

const char *httpp_getvar(_Ptr<http_parser_t> parser, const char *name) : itype(_Nt_array_ptr<const char> ) 
{
    http_var_t var = {};
    _Ptr<http_var_t> found = NULL;
    void* fp = NULL;

    if (parser == NULL || name == NULL)
        return NULL;

    fp = &found;
    memset(&var, 0, sizeof(var));
    var.name = (char*)name;

    if (avl_get_by_key(parser->vars, &var, fp) == 0) {
        if (!found->values)
            return NULL;
        return found->value[0];
    } else {
        return NULL;
    }
}

static void _httpp_set_param_nocopy(_Ptr<avl_tree> tree, char *name, _Nt_array_ptr<char> value, int replace)
{
    http_var_t *var, *found;
    size_t nbounds = 0; /* Hasan: Created to give bounds to n. If we had 'where' clause it can be replaced */
    _Array_ptr<_Nt_array_ptr<char>> n : count(nbounds) = NULL;

    if (name == NULL || value == NULL)
        return;

    found = _httpp_get_param_var(tree, name);

    if (replace || !found) {
        var = (http_var_t *)calloc(1, sizeof(http_var_t));
        if (var == NULL) {
            free(name);
            free(value);
            return;
        }

        var->name = name;
    } else {
        free(name);
        var = found;
    }


    nbounds = var->values + 1; /* Hasan: Assigned bounds value before realloc */

    n = realloc(var->value, sizeof(*n)*(nbounds));
    if (!n) {
        if (replace || !found) {
            free(name);
            free(var);
        }
        free(value);
        return;
    }

    var->value = n;

    nbounds = var->values++; /* Hasan: Used local variable in place of modifying expression */
    var->value[nbounds] = value;

    if (replace && found) {
        avl_delete(tree, (void *)found, _free_vars);
        avl_insert(tree, (void *)var);
    } else if (!found) {
        avl_insert(tree, (void *)var);
    }
}

static void _httpp_set_param(_Ptr<avl_tree> tree, _Nt_array_ptr<const char> name, _Nt_array_ptr<const char> value)
{
    size_t call_for_strlen = strlen(value); /* Hasan: Compiler doesn't like function calls in arguments, so made temp variable */
    if (name == NULL || value == NULL)
        return;

    _httpp_set_param_nocopy(tree, strdup(name), url_unescape(value, call_for_strlen), 1); /* Hasan: Compiler doesn't like function calls in arguments, so made temp variable */
}

static http_var_t *_httpp_get_param_var(_Ptr<avl_tree> tree, const char *name) : itype(_Ptr<http_var_t> ) 
{
    http_var_t var = {};
    _Ptr<http_var_t> found = NULL;
    void* fp = NULL;

    fp = &found;
    memset(&var, 0, sizeof(var));
    var.name = (char *)name;

    if (avl_get_by_key(tree, (void *)&var, fp) == 0)
        return found;
    else
        return NULL;
}
static const char *_httpp_get_param(_Ptr<avl_tree> tree, const char *name) : itype(_Nt_array_ptr<const char> ) 
{
    _Ptr<http_var_t> res =  _httpp_get_param_var(tree, name);

    if (!res)
        return NULL;

    if (!res->values)
        return NULL;

    return res->value[0];
}

void httpp_set_query_param(_Ptr<http_parser_t> parser, _Nt_array_ptr<const char> name, const char *value : itype(_Nt_array_ptr<const char> ) )
{
    return _httpp_set_param(parser->queryvars, name, (_Nt_array_ptr<char>)value); /* Hasan: TODO: See if itype can be removed */
}

const char *httpp_get_query_param(_Ptr<http_parser_t> parser, _Ptr<const char> name) : itype(_Nt_array_ptr<const char> ) 
{
    return _httpp_get_param(parser->queryvars, (const char *)name);
}

void httpp_set_post_param(_Ptr<http_parser_t> parser, _Nt_array_ptr<const char> name, _Nt_array_ptr<const char> value)
{
    return _httpp_set_param(parser->postvars, name, value);
}

const char *httpp_get_post_param(_Ptr<http_parser_t> parser, _Ptr<const char> name) : itype(_Ptr<const char> ) 
{
    return _httpp_get_param(parser->postvars, (const char *)name);
}

const http_var_t *httpp_get_param_var(_Ptr<http_parser_t> parser, const char *name) : itype(_Ptr<const http_var_t> ) 
{
    _Ptr<http_var_t> ret =  _httpp_get_param_var(parser->postvars, name);

    if (ret)
        return ret;

    return _httpp_get_param_var(parser->queryvars, name);
}

const http_var_t *httpp_get_any_var(_Ptr<http_parser_t> parser, httpp_ns_t ns, const char *name) : itype(_Ptr<const http_var_t> ) 
{
    _Ptr<avl_tree> tree =  NULL;

    if (!parser || !name)
        return NULL;

    switch (ns) {
        case HTTPP_NS_VAR:
            if (name[0] != '_' || name[1] != '_')
                return NULL;
            tree = parser->vars;
        break;
        case HTTPP_NS_HEADER:
            if (name[0] == '_' && name[1] == '_')
                return NULL;
            tree = parser->vars;
        break;
        case HTTPP_NS_QUERY_STRING:
            tree = parser->queryvars;
        break;
        case HTTPP_NS_POST_BODY:
            tree = parser->postvars;
        break;
    }

    if (!tree)
        return NULL;

    return _httpp_get_param_var(tree, name);
}

_Ptr<_Ptr<char>> httpp_get_any_key(_Ptr<http_parser_t> parser, httpp_ns_t ns)
{
    _Ptr<avl_tree> tree =  NULL;
    avl_node *avlnode;
    size_t len;
    _Array_ptr<_Nt_array_ptr<char>> ret : count(len) = NULL;
    size_t pos = 0;

    if (!parser)
        return NULL;

    switch (ns) {
        case HTTPP_NS_VAR:
        case HTTPP_NS_HEADER:
            tree = parser->vars;
        break;
        case HTTPP_NS_QUERY_STRING:
            tree = parser->queryvars;
        break;
        case HTTPP_NS_POST_BODY:
            tree = parser->postvars;
        break;
    }

    if (!tree)
        return NULL;

    ret = calloc(8, sizeof(*ret));
    if (!ret)
        return NULL;

    len = 8;

    for (avlnode = avl_get_first(tree); avlnode; avlnode = avl_get_next(avlnode)) {
        http_var_t *var = avlnode->key;

        if (ns == HTTPP_NS_VAR) {
            if (var->name[0] != '_' || var->name[1] != '_') {
                continue;
            }
        } else if (ns == HTTPP_NS_HEADER) {
            if (var->name[0] == '_' && var->name[1] == '_') {
                continue;
            }
        }

        if (pos == (len-1)) {
	    size_t nlen = sizeof(*ret)*(len + 8); /* Hasan: local var to get count */
            _Array_ptr<_Nt_array_ptr<char>> n : count(nlen) =  realloc(ret, nlen);
            if (!n) {
                httpp_free_any_key((char**)ret);
                return NULL;
            }
            memset(n + len, 0, sizeof(*n)*8);
            ret = n;
            len += 8;
        }

        ret[pos] = (_Nt_array_ptr<char>)strdup(var->name); /* Hasan: itype issue. Should be fixed with checked scope */
        if (!ret[pos]) {
            httpp_free_any_key((char**)ret);
            return NULL;
        }

        pos++;
    }

    return (_Ptr<_Ptr<char>>)ret; /* Hasan: Added a cast to get rid of return type error. This shouldn't be needed. Compiler error? */
}

void httpp_free_any_key(char **keys)
{
    char **p;

    if (!keys)
        return;

    for (p = keys; *p; p++) {
        free(*p);
    }
    free(keys);
}

const char *httpp_get_param(_Ptr<http_parser_t> parser, _Ptr<const char> name) : itype(_Nt_array_ptr<const char> ) 
{
    _Nt_array_ptr<const char> ret =  (_Nt_array_ptr<const char>)_httpp_get_param(parser->postvars, (const char *)name); /* Hasan: Cast for return type due to itype */

    if (ret)
        return ret;

    return _httpp_get_param(parser->queryvars, (const char *)name);
}

static void httpp_clear(_Ptr<http_parser_t> parser)
{
    parser->req_type = httpp_req_none;
    if (parser->uri)
        free(parser->uri);
    parser->uri = NULL;
    avl_tree_free(parser->vars, _free_vars);
    avl_tree_free(parser->queryvars, _free_vars);
    avl_tree_free(parser->postvars, _free_vars);
    parser->vars = NULL;
}

int httpp_addref(_Ptr<http_parser_t> parser)
{
    if (!parser)
        return -1;

    parser->refc++;

    return 0;
}

int httpp_release(_Ptr<http_parser_t> parser)
{
    if (!parser)
        return -1;

    parser->refc--;
    if (parser->refc)
        return 0;

    httpp_clear(parser);
    free(parser);

    return 0;
}

static char *_lowercase(_Array_ptr<char> str) : itype(_Array_ptr<char> ) 
{
    char *p = (char*)str; /* Hasan: Made p wild because we don't know the bounds of str. Maybe add strlen(str)? Would need 'where' clause becuase both sides of assignment need to have bounds... */
    for (; *p != '\0'; p++)
        *p = tolower(*p);

    return str;
}

static int _compare_vars(void* compare_arg, void *a, void *b)
{
    http_var_t *vara, *varb;

    vara = (http_var_t *)a;
    varb = (http_var_t *)b;

    return strcmp(vara->name, varb->name);
}

static int _free_vars(void *key)
{
    http_var_t *var = (http_var_t *)key;
    size_t i;

    free(var->name);

    for (i = 0; i < var->values; i++) {
        free(var->value[i]);
    }
    free(var->value);
    free(var);

    return 1;
}

httpp_request_type_e httpp_str_to_method(const char *method) {
    if (strcasecmp("GET", method) == 0) {
        return httpp_req_get;
    } else if (strcasecmp("POST", method) == 0) {
        return httpp_req_post;
    } else if (strcasecmp("PUT", method) == 0) {
        return httpp_req_put;
    } else if (strcasecmp("HEAD", method) == 0) {
        return httpp_req_head;
    } else if (strcasecmp("OPTIONS", method) == 0) {
        return httpp_req_options;
    } else if (strcasecmp("DELETE", method) == 0) {
        return httpp_req_delete;
    } else if (strcasecmp("TRACE", method) == 0) {
        return httpp_req_trace;
    } else if (strcasecmp("CONNECT", method) == 0) {
        return httpp_req_connect;
    } else if (strcasecmp("SOURCE", method) == 0) {
        return httpp_req_source;
    } else if (strcasecmp("PLAY", method) == 0) {
        return httpp_req_play;
    } else if (strcasecmp("STATS", method) == 0) {
        return httpp_req_stats;
    } else {
        return httpp_req_unknown;
    }
}

