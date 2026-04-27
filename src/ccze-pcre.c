/* -*- mode: c; c-file-style: "gnu" -*-
 * ccze-pcre.c -- PCRE1-compatible API implemented on top of PCRE2.
 *
 * The wrapper structs all start with a `kind` tag so that the
 * polymorphic ccze_pcre_free() can tell a compiled pattern from a
 * (no-op) study result without an extra free entry point.
 */

#ifdef HAVE_CONFIG_H
# include "system.h"
#endif

#include "ccze-pcre.h"

#include <stdlib.h>
#include <string.h>

#define CCZE_PCRE_KIND_CODE  1
#define CCZE_PCRE_KIND_EXTRA 2

struct ccze_pcre_code
{
  int kind;
  pcre2_code *re;
};

struct ccze_pcre_extra
{
  int kind;
};

#if defined(__GNUC__) || defined(__clang__)
# define CCZE_THREAD_LOCAL __thread
#else
# define CCZE_THREAD_LOCAL
#endif

static CCZE_THREAD_LOCAL char ccze_pcre_errbuf[256];

pcre *
ccze_pcre_compile (const char *pattern, int options,
                   const char **errptr, int *erroffset,
                   const unsigned char *tableptr)
{
  int errcode;
  PCRE2_SIZE erroffset_pcre2 = 0;
  pcre2_code *real;
  struct ccze_pcre_code *w;

  (void) tableptr;  /* PCRE2 uses compile contexts; ccze never sets tables. */

  real = pcre2_compile ((PCRE2_SPTR) pattern, PCRE2_ZERO_TERMINATED,
                        (uint32_t) options, &errcode, &erroffset_pcre2, NULL);
  if (real == NULL)
    {
      if (errptr != NULL)
        {
          pcre2_get_error_message (errcode,
                                   (PCRE2_UCHAR *) ccze_pcre_errbuf,
                                   sizeof ccze_pcre_errbuf);
          *errptr = ccze_pcre_errbuf;
        }
      if (erroffset != NULL)
        *erroffset = (int) erroffset_pcre2;
      return NULL;
    }

  w = malloc (sizeof *w);
  if (w == NULL)
    {
      pcre2_code_free (real);
      if (errptr != NULL)
        *errptr = "out of memory";
      if (erroffset != NULL)
        *erroffset = 0;
      return NULL;
    }
  w->kind = CCZE_PCRE_KIND_CODE;
  w->re = real;
  return (pcre *) w;
}

pcre_extra *
ccze_pcre_study (const pcre *code, int options, const char **errptr)
{
  struct ccze_pcre_extra *e;

  (void) code;
  (void) options;

  /* PCRE2 has no separate study step; JIT compilation is opt-in and
   * left as a no-op here for simplicity.  We still allocate a wrapper
   * so callers can pcre_free() it symmetrically with the compiled
   * pattern. */
  e = malloc (sizeof *e);
  if (e == NULL)
    {
      if (errptr != NULL)
        *errptr = "out of memory";
      return NULL;
    }
  e->kind = CCZE_PCRE_KIND_EXTRA;
  if (errptr != NULL)
    *errptr = NULL;
  return (pcre_extra *) e;
}

int
ccze_pcre_exec (const pcre *code, const pcre_extra *extra,
                const char *subject, int length, int startoffset,
                int options, int *ovector, int ovecsize)
{
  const struct ccze_pcre_code *w = (const struct ccze_pcre_code *) code;
  pcre2_match_data *md;
  PCRE2_SIZE subj_len;
  int rc;
  PCRE2_SIZE *ov;
  int pairs, max_pairs, i;

  (void) extra;  /* study result is a no-op wrapper; nothing to apply. */

  if (w == NULL || w->re == NULL)
    return PCRE2_ERROR_NULL;

  md = pcre2_match_data_create_from_pattern (w->re, NULL);
  if (md == NULL)
    return PCRE2_ERROR_NOMEMORY;

  subj_len = (length < 0) ? PCRE2_ZERO_TERMINATED : (PCRE2_SIZE) length;

  rc = pcre2_match (w->re, (PCRE2_SPTR) subject, subj_len,
                    (PCRE2_SIZE) startoffset, (uint32_t) options, md, NULL);

  if (rc < 0)
    {
      pcre2_match_data_free (md);
      return rc;
    }

  /* PCRE1 reserved the last third of ovector as workspace; PCRE2
   * does not, but we honor that convention so callers passing 99
   * still see only 33 group slots used. */
  ov = pcre2_get_ovector_pointer (md);
  pairs = rc * 2;
  max_pairs = (ovecsize / 3) * 2;
  if (pairs > max_pairs)
    pairs = max_pairs;

  for (i = 0; i < pairs; i++)
    ovector[i] = (ov[i] == PCRE2_UNSET) ? -1 : (int) ov[i];

  pcre2_match_data_free (md);
  return rc;
}

int
ccze_pcre_get_substring (const char *subject, int *ovector,
                         int stringcount, int stringnumber,
                         const char **stringptr)
{
  int s, e;
  size_t len;
  char *out;

  (void) stringcount;  /* Not needed: ovector already holds the data. */

  if (subject == NULL || ovector == NULL || stringptr == NULL
      || stringnumber < 0)
    return -1;

  s = ovector[stringnumber * 2];
  e = ovector[stringnumber * 2 + 1];

  if (s < 0 || e < 0 || e < s)
    {
      out = malloc (1);
      if (out == NULL)
        return -1;
      out[0] = '\0';
      *stringptr = out;
      return 0;
    }

  len = (size_t) (e - s);
  out = malloc (len + 1);
  if (out == NULL)
    return -1;
  memcpy (out, subject + s, len);
  out[len] = '\0';
  *stringptr = out;
  return (int) len;
}

void
ccze_pcre_free (void *ptr)
{
  int kind;

  if (ptr == NULL)
    return;

  kind = *(int *) ptr;
  if (kind == CCZE_PCRE_KIND_CODE)
    {
      struct ccze_pcre_code *w = ptr;
      if (w->re != NULL)
        pcre2_code_free (w->re);
    }
  /* CCZE_PCRE_KIND_EXTRA has no inner resource to release. */
  free (ptr);
}

void
ccze_pcre_free_substring (const char *stringptr)
{
  free ((void *) stringptr);
}
