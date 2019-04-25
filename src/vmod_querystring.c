/*-
 * Copyright (C) 2016-2019  Dridi Boukelmoune
 * All rights reserved.
 *
 * Author: Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cache/cache.h>
#include <vrt_obj.h>

#include <vcl.h>
#include <vre.h>
#include <vsb.h>

#include "vcc_querystring_if.h"

/* Varnish < 6.2 compat */
#ifndef VPFX
  #define VPFX(a) vmod_ ## a
  #define VARGS(a) vmod_ ## a ## _arg
  #define VENUM(a) vmod_enum_ ## a
#endif

/* End Of Query Parameter */
#define EOQP(c) (c == '\0' || c == '&')

#define CHECK_VALID_URL(ctx, arg)					\
	do {								\
		if (!(arg)->valid_url) {				\
			if ((ctx)->req)					\
				(arg)->url = VRT_r_req_url(ctx);	\
			else if ((ctx)->bo)				\
				(arg)->url = VRT_r_bereq_url(ctx);	\
			else {						\
				VRT_fail(ctx, "Invalid transaction");	\
				return (NULL);				\
			}						\
		}							\
	} while (0)

#ifndef HAVE_WS_RESERVEALL
#  define WS_ReserveAll(ws) WS_Reserve(ws, 0)
#endif

/***********************************************************************
 * Type definitions
 */

struct qs_param {
	const char	*val;
	size_t		val_len;
	size_t		cmp_len;
};

struct qs_filter;

typedef int qs_match_f(VRT_CTX, const struct qs_filter *, const char *,
    unsigned);

typedef void qs_free_f(void *);

struct qs_filter {
	unsigned		magic;
#define QS_FILTER_MAGIC		0xfc750864
	union {
		void		*ptr;
		const char	*str;
	};
	qs_match_f		*match;
	qs_free_f		*free;
	VTAILQ_ENTRY(qs_filter)	list;
};

struct VPFX(querystring_filter) {
	unsigned			magic;
#define VMOD_QUERYSTRING_FILTER_MAGIC	0xbe8ecdb4
	VTAILQ_HEAD(, qs_filter)	filters;
	unsigned			sort;
	unsigned			uniq;
	unsigned			match_name;
};

/***********************************************************************
 * Static data structures
 */

static struct VPFX(querystring_filter) qs_clean_filter = {
	.magic = VMOD_QUERYSTRING_FILTER_MAGIC,
	.sort = 0,
};

static struct VPFX(querystring_filter) qs_sort_filter = {
	.magic = VMOD_QUERYSTRING_FILTER_MAGIC,
	.sort = 1,
};

static struct VPFX(querystring_filter) qs_sort_uniq_filter = {
	.magic = VMOD_QUERYSTRING_FILTER_MAGIC,
	.sort = 1,
	.uniq = 1,
};

/***********************************************************************
 * VMOD implementation
 */

int qs_cmp(const void *, const void *);

static const char *
qs_truncate(struct ws *ws, const char * const url, const char *qs)
{
	size_t len, res;
	char *str;

	CHECK_OBJ_NOTNULL(ws, WS_MAGIC);
	AN(url);
	AN(qs);
	assert(url <= qs);

	len = qs - url;
	if (len == 0)
		return ("");

	res = WS_ReserveAll(ws);
	if (res < len + 1) {
		WS_Release(ws, 0);
		return (url);
	}

	str = ws->f;
	(void)memcpy(str, url, len);
	str[len] = '\0';
	WS_Release(ws, len + 1);
	return (str);
}

static int
qs_empty(struct ws *ws, const char * const url, const char **res)
{
	const char *qs;

	CHECK_OBJ_NOTNULL(ws, WS_MAGIC);
	AN(res);

	*res = url;

	if (url == NULL)
		return (1);

	qs = strchr(url, '?');
	if (qs == NULL)
		return (1);

	if (qs[1] == '\0') {
		*res = qs_truncate(ws, url, qs);
		return (1);
	}

	*res = qs;
	return (0);
}

static int
qs_match_string(VRT_CTX, const struct qs_filter *qsf, const char *s,
    unsigned keep)
{

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(qsf, QS_FILTER_MAGIC);

	(void)keep;
	return (!strcmp(s, qsf->str));
}

static int
qs_match_regex(VRT_CTX, const struct qs_filter *qsf, const char *s,
    unsigned keep)
{

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(qsf, QS_FILTER_MAGIC);

	(void)keep;
	return (VRT_re_match(ctx, s, qsf->ptr));
}

static int
qs_match_glob(VRT_CTX, const struct qs_filter *qsf, const char *s,
    unsigned keep)
{
	int match;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(qsf, QS_FILTER_MAGIC);

	match = fnmatch(qsf->str, s, 0);

	if (match == 0)
		return (1);

	if (match == FNM_NOMATCH)
		return (0);

	/* NB: If the fnmatch failed because of a wrong pattern, the error is
	 * logged but the query-string is kept intact.
	 */
	VSLb(ctx->vsl, SLT_Error, "querystring: failed to match glob `%s'",
	    qsf->str);
	return (keep);
}

int
qs_cmp(const void *v1, const void *v2)
{
	const struct qs_param *p1, *p2;
	size_t len;
	int cmp;

	AN(v1);
	AN(v2);
	p1 = v1;
	p2 = v2;

	len = p1->cmp_len < p2->cmp_len ? p1->cmp_len : p2->cmp_len;
	cmp = strncmp(p1->val, p2->val, len);

	if (cmp || p1->cmp_len == p2->cmp_len)
		return (cmp);
	return (p1->cmp_len - p2->cmp_len);
}

static unsigned
qs_match(VRT_CTX, const struct VPFX(querystring_filter) *obj,
    const struct qs_param *param, unsigned keep)
{
	struct qs_filter *qsf;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(obj, VMOD_QUERYSTRING_FILTER_MAGIC);

	if (param->cmp_len == 0)
		return (0);

	if (VTAILQ_EMPTY(&obj->filters))
		return (1);

	VTAILQ_FOREACH(qsf, &obj->filters, list) {
		CHECK_OBJ_NOTNULL(qsf, QS_FILTER_MAGIC);
		if (qsf->match(ctx, qsf, param->val, keep))
			return (keep);
	}

	return (!keep);
}

static size_t
qs_search(struct qs_param *key, struct qs_param *src, size_t cnt)
{
	size_t i, l = 0, u = cnt;
	int cmp;

	/* bsearch a position */
	do {
		i = (l + u) / 2;
		cmp = qs_cmp(key, src + i);
		if (cmp < 0)
			u = i;
		if (cmp > 0)
			l = i + 1;
	} while (l < u && cmp);

	/* ensure a stable sort */
	while (cmp >= 0 && ++i < cnt)
		cmp = qs_cmp(key, src + i);

	return (i);
}

static ssize_t
qs_insert(struct qs_param *new, struct qs_param *params, size_t cnt,
    unsigned sort, unsigned uniq)
{
	size_t pos = cnt;

	if (sort && cnt > 0)
		pos = qs_search(new, params, cnt);

	if (uniq && pos > 0 && !qs_cmp(new, params + pos - 1))
		return (-1);

	if (pos != cnt) {
		assert(pos < cnt);
		new = params + pos;
		(void)memmove(new + 1, new, (cnt - pos) * sizeof *new);
	}

	return (pos);
}

static char *
qs_append(char *cur, size_t cnt, struct qs_param *head)
{
	char sep;

	AN(cur);
	AN(cnt);
	AN(head);

	sep = '?';
	while (cnt > 0) {
		AZ(*cur);
		*cur = sep;
		cur++;
		(void)snprintf(cur, head->val_len + 1, "%s", head->val);
		sep = '&';
		cur += head->val_len;
		head++;
		cnt--;
	}

	return (cur);
}

static const char *
qs_apply(VRT_CTX, const char * const url, const char *qs, unsigned keep,
    const struct VPFX(querystring_filter) *obj)
{
	struct qs_param *params, *p, tmp;
	const char *nm, *eq;
	char *res, *cur;
	size_t len, cnt;
	ssize_t pos;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(ctx->ws, WS_MAGIC);
	CHECK_OBJ_NOTNULL(obj, VMOD_QUERYSTRING_FILTER_MAGIC);
	AN(url);
	AN(qs);
	assert(url <= qs);
	assert(*qs == '?');

	(void)WS_ReserveAll(ctx->ws);
	res = ctx->ws->f;
	len = strlen(url) + 1;

	params = (void *)PRNDUP(res + len);
	p = params;
	if ((uintptr_t)p > (uintptr_t)ctx->ws->e) {
		WS_Release(ctx->ws, 0);
		return (url);
	}

	len = qs - url;
	(void)memcpy(res, url, len);
	cur = res + len;

	cnt = 0;
	qs++;
	AN(*qs);

	/* NB: during the matching phase we can use the preallocated space for
	 * the result's query-string in order to copy the current parameter in
	 * the loop. This saves an allocation in matchers that require a null-
	 * terminated string (e.g. glob and regex).
	 */
	tmp.val = cur;

	while (*qs != '\0') {
		nm = qs;
		eq = NULL;

		while (!EOQP(*qs)) {
			if (eq == NULL && *qs == '=')
				eq = qs;
			qs++;
		}

		if (eq == nm)
			tmp.cmp_len = 0;
		else if (obj->match_name && eq != NULL)
			tmp.cmp_len = eq - nm;
		else
			tmp.cmp_len = qs - nm;

		/* NB: reminder, tmp.val == cur */
		(void)memcpy(cur, nm, tmp.cmp_len);
		cur[tmp.cmp_len] = '\0';
		tmp.val_len = qs - nm;

		pos = -1;
		if (qs_match(ctx, obj, &tmp, keep)) {
			AN(tmp.cmp_len);
			p = params + cnt;
			if ((uintptr_t)(p + 1) > (uintptr_t)ctx->ws->e) {
				WS_Release(ctx->ws, 0);
				return (url);
			}
			pos = qs_insert(&tmp, params, cnt, obj->sort,
			    obj->uniq);
		}

		if (pos >= 0) {
			p = params + pos;
			p->val = nm;
			p->val_len = tmp.val_len;
			p->cmp_len = tmp.cmp_len;
			cnt++;
		}

		if (*qs == '&')
			qs++;
	}

	*cur = '\0';
	if (cnt > 0)
		cur = qs_append(cur, cnt, params);

	AZ(*cur);
	cur = (char *)PRNDUP(cur + 1);
	assert((uintptr_t)cur <= (uintptr_t)params);

	WS_Release(ctx->ws, cur - res);
	return (res);
}

/***********************************************************************
 * VMOD interfaces
 */

VCL_STRING
vmod_remove(VRT_CTX, struct VARGS(remove) *arg)
{
	const char *res, *qs;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(ctx->ws, WS_MAGIC);
	AN(arg);

	CHECK_VALID_URL(ctx, arg);

	res = NULL;
	if (qs_empty(ctx->ws, arg->url, &res))
		return (res);

	qs = res;
	return (qs_truncate(ctx->ws, arg->url, qs));
}

VCL_VOID
vmod_filter__init(VRT_CTX, struct VPFX(querystring_filter) **objp,
    const char *vcl_name, VCL_BOOL sort, VCL_BOOL uniq, VCL_STRING match)
{
	struct VPFX(querystring_filter) *obj;

	ASSERT_CLI();
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	AN(objp);
	AZ(*objp);
	AN(vcl_name);

	ALLOC_OBJ(obj, VMOD_QUERYSTRING_FILTER_MAGIC);
	AN(obj);

	VTAILQ_INIT(&obj->filters);
	obj->sort = sort;
	obj->uniq = uniq;

	if (match == VENUM(name))
		obj->match_name = 1;
	else if (match != VENUM(param)) {
		VRT_fail(ctx, "Unknown matching type: %s", match);
		FREE_OBJ(obj);
	}

	*objp = obj;
}

VCL_VOID
vmod_filter__fini(struct VPFX(querystring_filter) **objp)
{
	struct VPFX(querystring_filter) *obj;
	struct qs_filter *qsf, *tmp;

	ASSERT_CLI();
	TAKE_OBJ_NOTNULL(obj, objp, VMOD_QUERYSTRING_FILTER_MAGIC);

	VTAILQ_FOREACH_SAFE(qsf, &obj->filters, list, tmp) {
		CHECK_OBJ_NOTNULL(qsf, QS_FILTER_MAGIC);
		if (qsf->free != NULL)
			qsf->free(qsf->ptr);
		VTAILQ_REMOVE(&obj->filters, qsf, list);
		FREE_OBJ(qsf);
	}

	FREE_OBJ(obj);
}

VCL_VOID
vmod_filter_add_string(VRT_CTX, struct VPFX(querystring_filter) *obj,
    VCL_STRING string)
{
	struct qs_filter *qsf;

	ASSERT_CLI();
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(obj, VMOD_QUERYSTRING_FILTER_MAGIC);
	AN(string);

	ALLOC_OBJ(qsf, QS_FILTER_MAGIC);
	AN(qsf);

	qsf->str = string;
	qsf->match = qs_match_string;
	VTAILQ_INSERT_TAIL(&obj->filters, qsf, list);
}

VCL_VOID
vmod_filter_add_glob(VRT_CTX, struct VPFX(querystring_filter) *obj,
    VCL_STRING glob)
{
	struct qs_filter *qsf;

	ASSERT_CLI();
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(obj, VMOD_QUERYSTRING_FILTER_MAGIC);
	AN(glob);

	ALLOC_OBJ(qsf, QS_FILTER_MAGIC);
	AN(qsf);

	qsf->str = glob;
	qsf->match = qs_match_glob;
	VTAILQ_INSERT_TAIL(&obj->filters, qsf, list);
}

VCL_VOID
vmod_filter_add_regex(VRT_CTX, struct VPFX(querystring_filter) *obj,
    VCL_STRING regex)
{
	struct qs_filter *qsf;
	const char *error;
	int error_offset;
	ssize_t msg_len;

	ASSERT_CLI();
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(obj, VMOD_QUERYSTRING_FILTER_MAGIC);
	AN(regex);

	ALLOC_OBJ(qsf, QS_FILTER_MAGIC);
	AN(qsf);

	qsf->ptr = VRE_compile(regex, 0, &error, &error_offset);
	if (qsf->ptr == NULL) {
		AN(ctx->msg);
		FREE_OBJ(qsf);
		msg_len = VSB_len(ctx->msg);
		VRT_fail(ctx,
		    "vmod-querystring: regex error (%s): '%s' pos %d",
		    error, regex, error_offset);

		/* NB: VRT_fail may or may not pass the error message to the
		 * CLI, deal with it. */
		if (msg_len == VSB_len(ctx->msg))
			VSB_printf(ctx->msg, "vmod-querystring: "
			    "regex error (%s): '%s' pos %d\n",
			    error, regex, error_offset);

		return;
	}

	qsf->match = qs_match_regex;
	qsf->free = VRT_re_fini;
	VTAILQ_INSERT_TAIL(&obj->filters, qsf, list);
}

VCL_STRING
vmod_filter_apply(VRT_CTX, struct VPFX(querystring_filter) *obj,
    struct VARGS(filter_apply) *arg)
{
	const char *tmp, *qs;
	unsigned keep;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(obj, VMOD_QUERYSTRING_FILTER_MAGIC);
	AN(arg);
	AN(arg->mode);

	CHECK_VALID_URL(ctx, arg);

	tmp = NULL;
	if (qs_empty(ctx->ws, arg->url, &tmp))
		return (tmp);

	qs = tmp;
	keep = 0;

	if (arg->mode == VENUM(keep))
		keep = 1;
	else if (arg->mode != VENUM(drop)) {
		VRT_fail(ctx, "Unknown filtering mode: %s", arg->mode);
		return (NULL);
	}

	return (qs_apply(ctx, arg->url, qs, keep, obj));
}

VCL_STRING
vmod_filter_extract(VRT_CTX, struct VPFX(querystring_filter) *obj,
    struct VARGS(filter_extract) *arg)
{
	struct VARGS(filter_apply) apply_arg[1];
	const char *res, *qs;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(obj, VMOD_QUERYSTRING_FILTER_MAGIC);
	AN(arg);
	AN(arg->mode);

	CHECK_VALID_URL(ctx, arg);

	if (arg->url == NULL)
		return (NULL);

	qs = strchr(arg->url, '?');
	if (qs == NULL || qs[1] == '\0')
		return (NULL);

	apply_arg->valid_url = 1;
	apply_arg->url = qs;
	apply_arg->mode = arg->mode;
	res = vmod_filter_apply(ctx, obj, apply_arg);
	AN(res);
	if (*res == '?')
		res++;
	else
		AZ(*res);
	return (res);
}

VCL_STRING
vmod_clean(VRT_CTX, struct VARGS(clean) *arg)
{
	struct VARGS(filter_apply) apply_arg[1];

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	AN(arg);
	apply_arg->valid_url = arg->valid_url;
	apply_arg->url = arg->url;
	apply_arg->mode = VENUM(keep);
	return (vmod_filter_apply(ctx, &qs_clean_filter, apply_arg));
}

VCL_STRING
vmod_sort(VRT_CTX, struct VARGS(sort) *arg)
{
	struct VPFX(querystring_filter) *filter;
	struct VARGS(filter_apply) apply_arg[1];

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	AN(arg);
	filter = arg->uniq ? &qs_sort_uniq_filter : &qs_sort_filter;

	apply_arg->valid_url = arg->valid_url;
	apply_arg->url = arg->url;
	apply_arg->mode = VENUM(keep);
	return (vmod_filter_apply(ctx, filter, apply_arg));
}
