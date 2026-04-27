/* -*- mode: c; c-file-style: "gnu" -*-
 * ccze-pcre.h -- PCRE1-compatible API implemented on top of PCRE2.
 *
 * This shim allows the rest of ccze to keep using the original PCRE1
 * function names and semantics while linking against libpcre2-8 only,
 * since libpcre3 (PCRE1) has been removed from modern distributions.
 */

#ifndef _CCZE_PCRE_H
#define _CCZE_PCRE_H 1

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <stddef.h>

typedef struct ccze_pcre_code  pcre;
typedef struct ccze_pcre_extra pcre_extra;

#define pcre_compile        ccze_pcre_compile
#define pcre_study          ccze_pcre_study
#define pcre_exec           ccze_pcre_exec
#define pcre_get_substring  ccze_pcre_get_substring
#define pcre_free           ccze_pcre_free
#define pcre_free_substring ccze_pcre_free_substring

pcre        *ccze_pcre_compile (const char *pattern, int options,
                                const char **errptr, int *erroffset,
                                const unsigned char *tableptr);
pcre_extra  *ccze_pcre_study   (const pcre *code, int options,
                                const char **errptr);
int          ccze_pcre_exec    (const pcre *code, const pcre_extra *extra,
                                const char *subject, int length,
                                int startoffset, int options,
                                int *ovector, int ovecsize);
int          ccze_pcre_get_substring (const char *subject, int *ovector,
                                      int stringcount, int stringnumber,
                                      const char **stringptr);
void         ccze_pcre_free    (void *ptr);
void         ccze_pcre_free_substring (const char *stringptr);

/* PCRE1 option constants mapped to PCRE2 equivalents.  None of ccze's
 * built-in plugins actually pass options, but external plugins might. */
#define PCRE_CASELESS         PCRE2_CASELESS
#define PCRE_MULTILINE        PCRE2_MULTILINE
#define PCRE_DOTALL           PCRE2_DOTALL
#define PCRE_EXTENDED         PCRE2_EXTENDED
#define PCRE_ANCHORED         PCRE2_ANCHORED
#define PCRE_DOLLAR_ENDONLY   PCRE2_DOLLAR_ENDONLY
#define PCRE_UNGREEDY         PCRE2_UNGREEDY
#define PCRE_NOTEMPTY         PCRE2_NOTEMPTY
#define PCRE_UTF8             PCRE2_UTF
#define PCRE_NOTBOL           PCRE2_NOTBOL
#define PCRE_NOTEOL           PCRE2_NOTEOL

/* PCRE1 returned -1 for no match; PCRE2_ERROR_NOMATCH happens to also
 * be -1, so the conventional `>= 0` success check used throughout the
 * mod_*.c sources keeps working unchanged. */
#define PCRE_ERROR_NOMATCH    PCRE2_ERROR_NOMATCH

#endif /* !_CCZE_PCRE_H */
