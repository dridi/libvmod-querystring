/*
 * libvmod-querystring - querystring manipulation module for Varnish
 *
 * Copyright (C) 2012-2016, Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <vrt.h>
#include <vre.h>
#include <cache/cache.h>

#include "vcc_if.h"
#include "vmod_querystring.h"

/* End Of Query Parameter */
#define EOQP(c) (c == '\0' || c == '&')

/***********************************************************************/

#ifndef HAVE_MEMPCPY

void *
mempcpy(void *dst, const void *src, size_t len)
{

	return ((void*)(((char*)memcpy(dst, src, len)) + len));
}

#endif

/***********************************************************************
 * The static functions below contain the actual implementation of the
 * module with the least possible coupling to Varnish. This helps keep a
 * single code base for all Varnish versions.
 */

static const char *
qs_truncate(struct ws *ws, const char *uri, const char *qs)
{
	size_t qs_pos;
	char *trunc;

	AN(qs);
	AN(uri);
	assert(qs > uri);

	qs_pos = qs - uri;
	trunc = WS_Alloc(ws, qs_pos + 1);

	if (trunc == NULL)
		return (uri);

	memcpy(trunc, uri, qs_pos);
	trunc[qs_pos] = '\0';

	return (trunc);
}

static const char *
qs_remove(struct ws *ws, const char *uri)
{
	char *qs;

	if (uri == NULL)
		return (NULL);

	qs = strchr(uri, '?');
	if (qs == NULL)
		return (uri);

	return (qs_truncate(ws, uri, qs));
}

static int
qs_cmp(const char *a, const char *b)
{

	while (*a == *b) {
		if (EOQP(*a) || EOQP(*b))
			return (0);
		a++;
		b++;
	}
	return (*a - *b);
}

static const char *
qs_sort(struct ws *ws, const char *uri)
{
	struct query_param *end, *params;
	int count, head, i, last_param, previous, sorted, tail;
	char *c, *position, *qs, *snapshot, *sorted_uri;
	const char *current_param;
	unsigned available;

	if (uri == NULL)
		return (NULL);

	qs = strchr(uri, '?');
	if (qs == NULL)
		return (uri);

	if (qs[1] == '\0')
		return (qs_truncate(ws, uri, qs));

	/* reserve some memory */
	snapshot = WS_Snapshot(ws);
	sorted_uri = WS_Alloc(ws, strlen(uri) + 1);

	WS_Assert(ws);

	if (sorted_uri == NULL) {
		WS_Reset(ws, snapshot);
		return (uri);
	}

	available = WS_Reserve(ws, 0);
	params = (struct query_param *) ws->f;
	end = params + available;

	/* initialize the params array */
	head = 10;

	if (&params[head + 1] > end)
		head = 0;

	if (&params[head + 1] > end) {
		WS_Release(ws, 0);
		WS_Reset(ws, snapshot);
		return (uri);
	}

	tail = head;
	last_param = head;

	/* search and sort params */
	sorted = 1;
	c = qs + 1;
	params[head].value = c;

	for (; *c != '\0' && &params[tail+1] < end; c++) {
		if (*c != '&')
			continue;

		current_param = c+1;
		params[last_param].len = c - params[last_param].value;

		if (head > 0 &&
		    qs_cmp(params[head].value, current_param) > -1) {
			sorted = 0;
			params[--head].value = current_param;
			last_param = head;
			continue;
		}

		if (qs_cmp(params[tail].value, current_param) < 1) {
			params[++tail].value = current_param;
			last_param = tail;
			continue;
		}

		sorted = 0;

		i = tail++;
		params[tail] = params[i];

		previous = i-1;
		while (i > head && qs_cmp(params[previous].value,
		    current_param) > -1)
			params[i--] = params[previous--];

		params[i].value = current_param;
		last_param = i;
	}

	if (sorted || &params[tail+1] >= end || tail - head < 1) {
		WS_Release(ws, 0);
		WS_Reset(ws, snapshot);
		return (uri);
	}

	params[last_param].len = c - params[last_param].value;

	/* copy the url parts */
	position = mempcpy(sorted_uri, uri, qs - uri + 1);
	count = tail-head;

	for (;count > 0; count--, ++head)
		if (params[head].len > 0) {
			position = mempcpy(position, params[head].value,
			    params[head].len);
			*position++ = '&';
		}

	if (params[head].len > 0)
		position = mempcpy(position, params[head].value,
		    params[head].len);
	else
		position--;

	*position = '\0';

	WS_Release(ws, 0);
	return (sorted_uri);
}

static void
append_string(char **begin, const char *end, const char *string, size_t len)
{

	if (*begin + len < end)
		memcpy(*begin, string, len);
	*begin += len;
}

static int
is_param_cleaned(const char *param, size_t len, struct filter_context *context)
{

	(void)param;
	(void)context;
	return (len == 0);
}

static int
is_param_filtered(const char *param, size_t len, struct filter_context *context)
{
	const char *p;
	va_list aq;

	if (len == 0)
		return (1);

	p = context->params.filter.params;

	va_copy(aq, context->params.filter.ap);
	while (p != vrt_magic_string_end) {
		if (p != NULL && strlen(p) == len &&
		    strncmp(param, p, len) == 0)
			return (!context->is_kept);
		p = va_arg(aq, const char*);
	}
	va_end(aq);

	return (context->is_kept);
}

static int
is_param_regfiltered(const char *param, size_t len, struct filter_context *context)
{
	int match;

	if (len == 0)
		return (1);

	/* XXX: allocate from workspace? */
	char p[len + 1];

	memcpy(p, param, len);
	p[len] = '\0';

	match = VRT_re_match(context->params.regfilter.re_ctx, p,
	    context->params.regfilter.re);
	return (match ^ context->is_kept);
}

static void *
compile_regex(const char *regex)
{
	void *re;
	const char *error;
	int error_offset;

	re = VRE_compile(regex, 0, &error, &error_offset);
	return (re);
}

static const char*
qs_apply(struct filter_context *context)
{
	const char *cursor, *param_position, *equal_position;
	char *begin, *end;
	unsigned available;
	int param_name_length;

	available = WS_Reserve(context->ws, 0);
	begin = context->ws->f;
	end = &begin[available];
	cursor = context->qs;

	append_string(&begin, end, context->uri, cursor - context->uri + 1);

	while (*cursor != '\0' && begin < end) {
		param_position = ++cursor;
		equal_position = NULL;

		while (*cursor != '\0' && *cursor != '&') {
			if (equal_position == NULL && *cursor == '=')
				equal_position = cursor;
			cursor++;
		}

		param_name_length =
			(equal_position ? equal_position : cursor) - param_position;

		if ( ! context->is_filtered(param_position, param_name_length, context) ) {
			append_string(&begin, end, param_position, cursor - param_position);
			if (*cursor == '&') {
				*begin = '&';
				begin++;
			}
		}
	}

	if (begin < end) {
		begin -= (begin[-1] == '&');
		begin -= (begin[-1] == '?');
		*begin = '\0';
	}

	begin++;

	if (begin > end) {
		WS_Release(context->ws, 0);
		return (context->uri);
	}

	end = begin;
	begin = context->ws->f;
	WS_Release(context->ws, end - begin);
	return (begin);
}

static const char *
qs_filter(struct filter_context *context)
{
	const char *uri, *qs, *filtered_uri;
	void *re;

	uri = context->uri;

	if (uri == NULL)
		return (NULL);

	qs = strchr(uri, '?');

	if (qs == NULL)
		return (uri);

	if (qs[1] == '\0')
		return (qs_truncate(context->ws, uri, qs));

	if (context->type == QS_REGFILTER) {
		re = compile_regex(context->params.regfilter.regex);
		if (re == NULL)
			return (uri);
		context->params.regfilter.re = re;
	}

	context->qs = qs;
	filtered_uri = qs_apply(context);

	if (context->type == QS_REGFILTER)
		VRT_re_fini(context->params.regfilter.re);

	return (filtered_uri);
}

/***********************************************************************
 * Below are the functions that will actually be linked by Varnish.
 */

const char *
vmod_clean(const struct vrt_ctx *ctx, const char *uri)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\"", uri);

	context.type = QS_CLEAN;
	context.ws = ctx->ws;
	context.uri = uri;
	context.is_filtered = &is_param_cleaned;
	context.is_kept = 0;

	filtered_uri = qs_filter(&context);

	QS_LOG_RETURN(ctx, filtered_uri);
	return (filtered_uri);
}

const char *
vmod_remove(const struct vrt_ctx *ctx, const char *uri)
{
	const char *cleaned_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\"", uri);

	cleaned_uri = qs_remove(ctx->ws, uri);

	QS_LOG_RETURN(ctx, cleaned_uri);
	return (cleaned_uri);
}

const char *
vmod_sort(const struct vrt_ctx *ctx, const char *uri)
{
	const char *sorted_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\"", uri);

	sorted_uri = qs_sort(ctx->ws, uri);

	QS_LOG_RETURN(ctx, sorted_uri);
	return (sorted_uri);
}

const char *
vmod_filtersep(const struct vrt_ctx *ctx)
{

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	return (NULL);
}

const char *
vmod_filter(const struct vrt_ctx *ctx, const char *uri, const char *params, ...)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\", \"%s\", ...", uri, params);

	context.type = QS_FILTER;
	context.ws = ctx->ws;
	context.uri = uri;
	context.params.filter.params = params;
	context.is_filtered = &is_param_filtered;
	context.is_kept = 0;

	va_start(context.params.filter.ap, params);
	filtered_uri = qs_filter(&context);
	va_end(context.params.filter.ap);

	QS_LOG_RETURN(ctx, filtered_uri);
	return (filtered_uri);
}

const char *
vmod_filter_except(const struct vrt_ctx *ctx, const char *uri, const char *params, ...)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\", \"%s\", ...", uri, params);

	context.type = QS_FILTER;
	context.ws = ctx->ws;
	context.uri = uri;
	context.params.filter.params = params;
	context.is_filtered = &is_param_filtered;
	context.is_kept = 1;

	va_start(context.params.filter.ap, params);
	filtered_uri = qs_filter(&context);
	va_end(context.params.filter.ap);

	QS_LOG_RETURN(ctx, filtered_uri);
	return (filtered_uri);
}

const char *
vmod_regfilter(const struct vrt_ctx *ctx, const char *uri, const char *regex)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\", \"%s\"", uri, regex);

	context.type = QS_REGFILTER;
	context.ws = ctx->ws;
	context.uri = uri;
	context.params.regfilter.regex = regex;
	context.params.regfilter.re_ctx = ctx;
	context.is_filtered = &is_param_regfiltered;
	context.is_kept = 0;

	filtered_uri = qs_filter(&context);

	QS_LOG_RETURN(ctx, filtered_uri);
	return (filtered_uri);
}

const char *
vmod_regfilter_except(const struct vrt_ctx *ctx, const char *uri, const char *regex)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\", \"%s\"", uri, regex);

	context.type = QS_REGFILTER;
	context.ws = ctx->ws;
	context.uri = uri;
	context.params.regfilter.regex = regex;
	context.params.regfilter.re_ctx = ctx;
	context.is_filtered = &is_param_regfiltered;
	context.is_kept = 1;

	filtered_uri = qs_filter(&context);

	QS_LOG_RETURN(ctx, filtered_uri);
	return (filtered_uri);
}
