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

#include "private.h"

static inline int __vsnprintf__is_print(int c, int allow_space)
{
    if ((c <= '"' || c == '`' || c == '\\') && !(allow_space && c == ' ')) {
        return 0;
    } else {
        return 1;
    }
}

static inline size_t __vsnprintf__strlen(const char *str, int is_alt, int allow_space)
{
    size_t ret = 0;

    if (!str) {
        if (is_alt) {
            return strlen("-");
        } else {
            return strlen("(null)");
        }
    }

    for (; *str; str++) {
        if (__vsnprintf__is_print(*str, allow_space)) {
            ret += 1;
        } else {
            ret += 4;
        }
    }

    if (is_alt) {
        ret += 2;
    }

    return ret;
}

void igloo_private__vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    static const char hextable[] = "0123456789abcdef";
    int in_block = 0;
    int block_size = 0;
    int block_len = 0;
    int block_space = 0;
    int block_alt = 0;
    const char * arg;
    char buf[80];

    for (; *format && size; format++)
    {
        if ( !in_block )
        {
            if ( *format == '%' ) {
                in_block = 1;
                block_size = 0;
                block_len  = 0;
                block_space = 0;
                block_alt = 0;
            }
            else
            {
                *(str++) = *format;
                size--;
            }
        }
        else
        {
            // TODO: %l*[sdupi] as well as %.4080s and "%.*s
            arg = NULL;
            switch (*format)
            {
                case 'l':
                    block_size++;
                    break;
                case 'z':
                    block_size = 'z';
                    break;
                case '.':
                    // just ignore '.'. If somebody cares: fix it.
                    break;
                case '*':
                    block_len = va_arg(ap, int);
                    break;
                case ' ':
                    block_space = 1;
                    break;
                case '#':
                    block_alt = 1;
                    break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    block_len = atoi(format);
                    for (; *format >= '0' && *format <= '9'; format++);
                    format--;
                    break;
                case 'p':
                    snprintf(buf, sizeof(buf), "%p", (void*)va_arg(ap, void *));
                    arg = buf;
                case 'd':
                case 'i':
                case 'u':
                    if (!arg)
                    {
                        switch (block_size)
                        {
                            case 0:
                                if (*format == 'u')
                                    snprintf(buf, sizeof(buf), "%u", (unsigned int)va_arg(ap, unsigned int));
                                else
                                    snprintf(buf, sizeof(buf), "%i", (int)va_arg(ap, int));
                                break;
                            case 1:
                                if (*format == 'u')
                                    snprintf(buf, sizeof(buf), "%lu", (unsigned long int)va_arg(ap, unsigned long int));
                                else
                                    snprintf(buf, sizeof(buf), "%li", (long int)va_arg(ap, long int));
                                break;
                            case 2:
                                if (*format == 'u')
                                    snprintf(buf, sizeof(buf), "%llu", (unsigned long long int)va_arg(ap, unsigned long long int));
                                else
                                    snprintf(buf, sizeof(buf), "%lli", (long long int)va_arg(ap, long long int));
                                break;
                            case 'z':
                                /* We do not use 'z' type of snprintf() here as it is not safe to use on a few outdated platforms. */
                                if (*format == 'u')
                                    snprintf(buf, sizeof(buf), "%llu", (unsigned long long int)va_arg(ap, size_t));
                                else
                                    snprintf(buf, sizeof(buf), "%lli", (long long int)va_arg(ap, ssize_t));
                                break;
                            default:
                                snprintf(buf, sizeof(buf), "<<<invalid>>>");
                                break;
                        }
                        arg = buf;
                    }
                case 's':
                case 'H':
                    // TODO.
                    if (!arg)
                        arg = va_arg(ap, const char *);
                    if (*format != 'H') {
                        block_alt = 0;
                    }
                    if (!arg && !block_alt)
                        arg = "(null)";
                    if (!block_len) {
                        block_len = __vsnprintf__strlen(arg, block_alt, block_space);
                    }

                    // the if() is the outer structure so the inner for()
                    // is branch optimized.
                    if (*format == 'H' && !arg)
                    {
                        if (size && block_len) {
                            *(str++) = '-';
                            size--;
                            block_len--;
                        }
                    }
                    else if (*format == 'H')
                    {
                        if (block_alt && size && block_len) {
                            *(str++) = '"';
                            size--;
                            block_len--;
                        }
                        for (; *arg && block_len && size; arg++, size--, block_len--)
                        {
                            if (!__vsnprintf__is_print(*arg, block_space)) {
                                if (size < 4 || block_len < 4) {
                                    /* Use old system if we do not have space for new one */
                                    *(str++) = '.';
                                } else {
                                    *(str++) = '\\';
                                    *(str++) = 'x';
                                    *(str++) = hextable[(*arg >> 0) & 0x0F];
                                    *(str++) = hextable[(*arg >> 4) & 0x0F];
                                    /* Also count the additional chars for string size and block length */
                                    size -= 3;
                                    block_len -= 3;
                                }
                            } else {
                                *(str++) = *arg;
                            }
                        }
                        if (block_alt && size && block_len) {
                            *(str++) = '"';
                            size--;
                            block_len--;
                        }
                    }
                    else
                    {
                        for (; *arg && block_len && size; arg++, size--, block_len--)
                            *(str++) = *arg;
                    }
                    in_block = 0;
                    break;
            }
        }
    }

    if ( !size )
        str--;

    *str = 0;
}

