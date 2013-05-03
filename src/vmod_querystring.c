/*
 * libvmod-querystring - querystring manipulation module for Varnish
 *
 * Copyright (C) 2012, Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 * All rights reserved.
 *
 * Redistribution  and use in source and binary forms, with or without
 * modification,  are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions   of  source   code   must   retain  the   above
 *    copyright  notice, this  list of  conditions  and the  following
 *    disclaimer.
 * 2. Redistributions   in  binary  form  must  reproduce  the   above
 *    copyright  notice, this  list of  conditions and  the  following
 *    disclaimer   in  the   documentation   and/or  other   materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT  NOT
 * LIMITED  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS
 * FOR  A  PARTICULAR  PURPOSE ARE DISCLAIMED. IN NO EVENT  SHALL  THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL,    SPECIAL,   EXEMPLARY,   OR   CONSEQUENTIAL   DAMAGES
 * (INCLUDING,  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES;  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT  LIABILITY,  OR  TORT (INCLUDING  NEGLIGENCE  OR  OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "vrt.h"
#include "vre.h"
#include "vmod_querystring.h"

#include "vcc_if.h"

/* End Of Query Parameter */
#define EOQP(c) (*c == '\0' || *c == '&')

/***********************************************************************/

#ifndef HAVE_MEMPCPY

void *mempcpy(void *dst, const void *src, size_t len)
{
	return (void*)(((char*)memcpy(dst, src, len)) + len);
}

#endif

/***********************************************************************
 * The static functions below contain the actual implementation of the
 * module with the least possible coupling to Varnish. This helps keep a
 * single code base for all Varnish versions.
 */

static const char *
truncate_querystring(struct ws *ws, const char *uri, const char *query_string)
{
	int query_string_position;
	char *clean_uri;

	query_string_position = query_string - uri;
	clean_uri = WS_Alloc(ws, query_string_position);

	if (clean_uri == NULL) {
		return uri;
	}

	memcpy(clean_uri, uri, query_string_position);
	clean_uri[query_string_position] = '\0';

	return clean_uri;
}

static const char *
clean_uri(struct ws *ws, const char *uri)
{
	if (uri == NULL) {
		return NULL;
	}

	char *query_string = strchr(uri, '?');
	if (query_string == NULL || query_string[1] != '\0') {
		return uri;
	}

	// TODO clean empty params

	return truncate_querystring(ws, uri, query_string);
}

static const char *
remove_querystring(struct ws *ws, const char *uri)
{
	if (uri == NULL) {
		return NULL;
	}

	char *query_string = strchr(uri, '?');
	if (query_string == NULL) {
		return uri;
	}

	return truncate_querystring(ws, uri, query_string);
}

struct query_param {
	const char *value;
	short length;
};

static int
compare_params(const char* a, const char* b)
{
	while (*a == *b) {
		if (EOQP(a) || EOQP(b)) {
			return 0;
		}
		a++;
		b++;
	}
	return *a - *b;
}

static const char *
sort_querystring(struct ws *ws, const char *uri)
{
	if (uri == NULL) {
		return NULL;
	}

	char *query_string = strchr(uri, '?');
	if (query_string == NULL) {
		return uri;
	}

	if (query_string[1] == '\0') {
		return truncate_querystring(ws, uri, query_string);
	}

	/* reserve some memory */
	char *snapshot = WS_Snapshot(ws);
	char *sorted_uri = WS_Alloc(ws, strlen(uri) + 1);

	WS_Assert(ws);

	if (sorted_uri == NULL) {
		WS_Reset(ws, snapshot);
		return uri;
	}

	unsigned available = WS_Reserve(ws, 0);
	struct query_param *params = (struct query_param*) ws->f;
	struct query_param *end = params + available;

	/* initialize the params array */
	int head = 10;

	if (&params[head + 1] > end) {
		head = 0;
	}

	if (&params[head + 1] > end) {
		WS_Release(ws, 0);
		WS_Reset(ws, snapshot);
		return uri;
	}

	int tail = head;
	int last_param = head;

	/* search and sort params */
	bool sorted = true;
	char *c = query_string + 1;
	params[head].value = c;

	for (; *c != '\0' && &params[tail+1] < end; c++) {
		if (*c != '&') {
			continue;
		}

		const char *current_param = c+1;
		params[last_param].length = c - params[last_param].value;

		if (head > 0 && compare_params(params[head].value, current_param) > -1) {
			sorted = false;
			params[--head].value = current_param;
			last_param = head;
			continue;
		}

		if (compare_params(params[tail].value, current_param) < 1) {
			params[++tail].value = current_param;
			last_param = tail;
			continue;
		}

		sorted = false;

		int i = tail++;
		params[tail] = params[i];

		int previous = i-1;
		while (i > head && compare_params(params[previous].value, current_param) > -1) {
			params[i--] = params[previous--];
		}

		params[i].value = current_param;
		last_param = i;
	}

	if (sorted == true || &params[tail+1] >= end || tail - head < 1) {
		WS_Release(ws, 0);
		WS_Reset(ws, snapshot);
		return uri;
	}

	params[last_param].length = c - params[last_param].value;

	/* copy the url parts */
	char *position = mempcpy(sorted_uri, uri, query_string - uri + 1);
	int count = tail-head;

	for (;count > 0; count--, ++head) {
		if (params[head].length > 0) {
			position = mempcpy(position, params[head].value, params[head].length);
			*position++ = '&';
		}
	}

	if (params[head].length > 0) {
		position = mempcpy(position, params[head].value, params[head].length);
	}
	else {
		position--;
	}

	*position = '\0';

	WS_Release(ws, 0);
	return sorted_uri;
}

static bool
is_param_filtered(const char *param, int length, const char *params, va_list ap)
{
	if (length == 0) {
		return true;
	}

	const char *p = params;

	while (p != vrt_magic_string_end) {
		if (p != NULL && strlen(p) == length && strncmp(param, p, length) == 0) {
			return true;
		}
		p = va_arg(ap, const char *);
	}

	return false;
}

static void
append_string(char **begin, const char *end, const char *string, int length)
{
	if (*begin + length < end) {
		memcpy(*begin, string, length);
	}
	*begin += length;
}

static const char *
filter_querystring(struct ws *ws, const char *uri, const char *params, va_list ap)
{
	if (uri == NULL) {
		return NULL;
	}

	char *query_string = strchr(uri, '?');
	if (query_string == NULL) {
		return uri;
	}

	if (query_string[1] == '\0') {
		return truncate_querystring(ws, uri, query_string);
	}

	unsigned available = WS_Reserve(ws, 0);
	char *begin = ws->f;
	char *end = &begin[available];

	append_string(&begin, end, uri, query_string - uri + 1);

	char *current_position = query_string;
	while (*current_position != '\0' && begin < end) {
		const char *param_position = ++current_position;
		const char *equal_position = NULL;

		while (*current_position != '\0' && *current_position != '&') {
			if (equal_position == NULL && *current_position == '=') {
				equal_position = current_position;
			}
			current_position++;
		}

		int param_name_length =
			(equal_position ? equal_position : current_position) - param_position;

		if ( ! is_param_filtered(param_position, param_name_length, params, ap) ) {
			append_string(&begin, end, param_position, current_position - param_position);
			if (*current_position == '&') {
				*begin = '&';
				begin++;
			}
		}
	}

	if (begin < end) {
		begin -= (begin[-1] == '&');
		*begin = '\0';
	}

	begin++;

	if (begin > end) {
		WS_Release(ws, 0);
		return uri;
	}

	end = begin;
	begin = ws->f;
	WS_Release(ws, end - begin);
	return begin;
}



static bool
#ifdef QS_NEED_RE_CTX
is_param_regfiltered(const char *param, int length, void *re, re_ctx *re_ctx)
#else
is_param_regfiltered(const char *param, int length, void *re)
#endif
{
	if (length == 0) {
		return true;
	}

	char p[length + 1];

	memcpy(p, param, length);
	p[length + 1] = '\0';

#ifdef QS_NEED_RE_CTX
	return (bool) VRT_re_match(re_ctx, p, re);
#else
	return (bool) VRT_re_match(p, re);
#endif
}

static void *
compile_regex(const char *regex)
{
	void *re;
	const char *error;
	int error_offset;

	re = VRE_compile(regex, 0, &error, &error_offset);
	return re;
}

static const char *
#ifdef QS_NEED_RE_CTX
regfilter_querystring(struct ws *ws, const char *uri, const char *regex, re_ctx *re_ctx)
#else
regfilter_querystring(struct ws *ws, const char *uri, const char *regex)
#endif
{
	if (uri == NULL) {
		return NULL;
	}

	char *query_string = strchr(uri, '?');
	if (query_string == NULL) {
		return uri;
	}

	if (query_string[1] == '\0') {
		return truncate_querystring(ws, uri, query_string);
	}

	void *re = compile_regex(regex);
	if (re == NULL) {
		return uri;
	}

	unsigned available = WS_Reserve(ws, 0);
	char *begin = ws->f;
	char *end = &begin[available];

	append_string(&begin, end, uri, query_string - uri + 1);

	char *current_position = query_string;
	while (*current_position != '\0' && begin < end) {
		const char *param_position = ++current_position;
		const char *equal_position = NULL;

		while (*current_position != '\0' && *current_position != '&') {
			if (equal_position == NULL && *current_position == '=') {
				equal_position = current_position;
			}
			current_position++;
		}

		int param_name_length =
			(equal_position ? equal_position : current_position) - param_position;

#ifdef QS_NEED_RE_CTX
		if ( ! is_param_regfiltered(param_position, param_name_length, re, re_ctx) ) {
#else
		if ( ! is_param_regfiltered(param_position, param_name_length, re) ) {
#endif
			append_string(&begin, end, param_position, current_position - param_position);
			if (*current_position == '&') {
				*begin = '&';
				begin++;
			}
		}
	}

	VRT_re_fini(re);

	if (begin < end) {
		begin -= (begin[-1] == '&');
		*begin = '\0';
	}

	begin++;

	if (begin > end) {
		WS_Release(ws, 0);
		return uri;
	}

	end = begin;
	begin = ws->f;
	WS_Release(ws, end - begin);
	return begin;
}

/***********************************************************************
 * Below are the functions that will actually be linked by Varnish.
 */

#if VARNISH_MAJOR == 3

const char *
vmod_clean(struct sess *sp, const char *uri)
{
	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	return clean_uri(sp->ws, uri);
}

const char *
vmod_remove(struct sess *sp, const char *uri)
{
	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	return remove_querystring(sp->ws, uri);
}

const char *
vmod_sort(struct sess *sp, const char *uri)
{
	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	return sort_querystring(sp->ws, uri);
}

const char *
vmod_filter(struct sess *sp, const char *uri, const char *params, ...)
{
	va_list ap;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);

	va_start(ap, params);
	filtered_uri = filter_querystring(sp->ws, uri, params, ap);
	va_end(ap);

	return filtered_uri;
}

const char *
vmod_filtersep(struct sess *sp)
{
	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	return NULL;
}

const char *
vmod_regfilter(struct sess *sp, const char *uri, const char *regex)
{
	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
#ifdef QS_NEED_RE_CTX
	return regfilter_querystring(sp->ws, uri, regex, sp);
#else
	return regfilter_querystring(sp->ws, uri, regex);
#endif
}

#endif

/***********************************************************************/

#if VARNISH_MAJOR == 4

const char *
vmod_clean(const struct vrt_ctx *ctx, const char *uri)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	return clean_uri(ctx->ws, uri);
}

const char *
vmod_remove(const struct vrt_ctx *ctx, const char *uri)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	return remove_querystring(ctx->ws, uri);
}

const char *
vmod_sort(const struct vrt_ctx *ctx, const char *uri)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	return sort_querystring(ctx->ws, uri);
}

const char *
vmod_filter(const struct vrt_ctx *ctx, const char *uri, const char *params, ...)
{
	va_list ap;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	va_start(ap, params);
	filtered_uri = filter_querystring(ctx->ws, uri, params, ap);
	va_end(ap);

	return filtered_uri;
}

const char *
vmod_filtersep(const struct vrt_ctx *ctx)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	return NULL;
}

const char *
vmod_regfilter(const struct vrt_ctx *ctx, const char *uri, const char *regex)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	return regfilter_querystring(ctx->ws, uri, regex, ctx);
}

#endif

