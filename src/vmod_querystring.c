/*
 * libvmod-querystring - querystring manipulation module for Varnish
 * 
 * Copyright (C) 2012-2013, Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
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
	char *truncated_uri;

	query_string_position = query_string - uri;
	truncated_uri = WS_Alloc(ws, query_string_position);

	if (truncated_uri == NULL) {
		return uri;
	}

	memcpy(truncated_uri, uri, query_string_position);
	truncated_uri[query_string_position] = '\0';

	return truncated_uri;
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

static void
append_string(char **begin, const char *end, const char *string, int length)
{
	if (*begin + length < end) {
		memcpy(*begin, string, length);
	}
	*begin += length;
}

static bool
is_param_cleaned(const char *param, int length, struct filter_context *context)
{
	return length == 0;
}

static bool
is_param_filtered(const char *param, int length, struct filter_context *context)
{
	va_list aq;
	if (length == 0) {
		return true;
	}

	const char *p = context->params.filter.params;

	va_copy(aq, context->params.filter.ap);
	while (p != vrt_magic_string_end) {
		if (p != NULL && strlen(p) == length && strncmp(param, p, length) == 0) {
			return true ^ context->is_kept;
		}
		p = va_arg(aq, const char*);
	}
	va_end(aq);

	return false ^ context->is_kept;
}

static bool
is_param_regfiltered(const char *param, int length, struct filter_context *context)
{
	if (length == 0) {
		return true;
	}

	char p[length + 1];

	memcpy(p, param, length);
	p[length] = '\0';

	bool match;
#ifdef QS_NEED_RE_CTX
	match = (bool) VRT_re_match(context->params.regfilter.re_ctx, p,
	                            context->params.regfilter.re);
#else
	match = (bool) VRT_re_match(p, context->params.regfilter.re);
#endif
	return match ^ context->is_kept;
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

static const char*
apply_filter(struct filter_context *context)
{
	unsigned available = WS_Reserve(context->ws, 0);
	char *begin = context->ws->f;
	char *end = &begin[available];
	const char *cursor = context->query_string;

	append_string(&begin, end, context->uri, cursor - context->uri + 1);

	while (*cursor != '\0' && begin < end) {
		const char *param_position = ++cursor;
		const char *equal_position = NULL;

		while (*cursor != '\0' && *cursor != '&') {
			if (equal_position == NULL && *cursor == '=') {
				equal_position = cursor;
			}
			cursor++;
		}

		int param_name_length =
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
		return context->uri;
	}

	end = begin;
	begin = context->ws->f;
	WS_Release(context->ws, end - begin);
	return begin;
}

static const char *
filter_querystring(struct filter_context *context)
{
	const char *uri = context->uri;
	const char *query_string;
	const char *filtered_uri;

	if (uri == NULL) {
		return NULL;
	}

	query_string = strchr(uri, '?');

	if (query_string == NULL) {
		return uri;
	}

	if (query_string[1] == '\0') {
		return truncate_querystring(context->ws, uri, query_string);
	}

	if (context->type == regfilter) {
		void *re = compile_regex(context->params.regfilter.regex);
		if (re == NULL) {
			return uri;
		}
		context->params.regfilter.re = re;
	}

	context->query_string = query_string;
	filtered_uri = apply_filter(context);

	if (context->type == regfilter) {
		VRT_re_fini(context->params.regfilter.re);
	}

	return filtered_uri;
}

/***********************************************************************
 * Below are the functions that will actually be linked by Varnish.
 */

#if VARNISH_MAJOR == 3

const char *
vmod_clean(struct sess *sp, const char *uri)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	QS_LOG_CALL(sp, "\"%s\"", uri);

	context.type = clean;
	context.ws = sp->ws;
	context.uri = uri;
	context.is_filtered = &is_param_cleaned;
	context.is_kept = false;

	filtered_uri = filter_querystring(&context);

	QS_LOG_RETURN(sp, filtered_uri);
	return filtered_uri;
}

const char *
vmod_remove(struct sess *sp, const char *uri)
{
	const char *cleaned_uri;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	QS_LOG_CALL(sp, "\"%s\"", uri);

	cleaned_uri = remove_querystring(sp->ws, uri);

	QS_LOG_RETURN(sp, cleaned_uri);
	return cleaned_uri;
}

const char *
vmod_sort(struct sess *sp, const char *uri)
{
	const char *sorted_uri;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	QS_LOG_CALL(sp, "\"%s\"", uri);

	sorted_uri = sort_querystring(sp->ws, uri);

	QS_LOG_RETURN(sp, sorted_uri);
	return sorted_uri;
}

const char *
vmod_filtersep(struct sess *sp)
{
	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	return NULL;
}

const char *
vmod_filter(struct sess *sp, const char *uri, const char *params, ...)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	QS_LOG_CALL(sp, "\"%s\", \"%s\", ...", uri, params);

	context.type = filter;
	context.ws = sp->ws;
	context.uri = uri;
	context.params.filter.params = params;
	context.is_filtered = &is_param_filtered;
	context.is_kept = false;

	va_start(context.params.filter.ap, params);
	filtered_uri = filter_querystring(&context);
	va_end(context.params.filter.ap);

	QS_LOG_RETURN(sp, filtered_uri);
	return filtered_uri;
}

const char *
vmod_filter_except(struct sess *sp, const char *uri, const char *params, ...)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	QS_LOG_CALL(sp, "\"%s\", \"%s\", ...", uri, params);

	context.type = filter;
	context.ws = sp->ws;
	context.uri = uri;
	context.params.filter.params = params;
	context.is_filtered = &is_param_filtered;
	context.is_kept = true;

	va_start(context.params.filter.ap, params);
	filtered_uri = filter_querystring(&context);
	va_end(context.params.filter.ap);

	QS_LOG_RETURN(sp, filtered_uri);
	return filtered_uri;
}

const char *
vmod_regfilter(struct sess *sp, const char *uri, const char *regex)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	QS_LOG_CALL(sp, "\"%s\", \"%s\"", uri, regex);

	context.type = regfilter;
	context.ws = sp->ws;
	context.uri = uri;
	context.params.regfilter.regex = regex;
	context.params.regfilter.re_ctx = sp;
	context.is_filtered = &is_param_regfiltered;
	context.is_kept = false;

	filtered_uri = filter_querystring(&context);

	QS_LOG_RETURN(sp, filtered_uri);
	return filtered_uri;
}

const char *
vmod_regfilter_except(struct sess *sp, const char *uri, const char *regex)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	QS_LOG_CALL(sp, "\"%s\", \"%s\"", uri, regex);

	context.type = regfilter;
	context.ws = sp->ws;
	context.uri = uri;
	context.params.regfilter.regex = regex;
	context.params.regfilter.re_ctx = sp;
	context.is_filtered = &is_param_regfiltered;
	context.is_kept = true;

	filtered_uri = filter_querystring(&context);

	QS_LOG_RETURN(sp, filtered_uri);
	return filtered_uri;
}

#endif

/***********************************************************************/

#if VARNISH_MAJOR == 4

const char *
vmod_clean(const struct vrt_ctx *ctx, const char *uri)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\"", uri);

	context.type = clean;
	context.ws = ctx->ws;
	context.uri = uri;
	context.is_filtered = &is_param_cleaned;
	context.is_kept = false;

	filtered_uri = filter_querystring(&context);

	QS_LOG_RETURN(ctx, filtered_uri);
	return filtered_uri;
}

const char *
vmod_remove(const struct vrt_ctx *ctx, const char *uri)
{
	const char *cleaned_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\"", uri);

	cleaned_uri = remove_querystring(ctx->ws, uri);

	QS_LOG_RETURN(ctx, cleaned_uri);
	return cleaned_uri;
}

const char *
vmod_sort(const struct vrt_ctx *ctx, const char *uri)
{
	const char *sorted_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\"", uri);

	sorted_uri = sort_querystring(ctx->ws, uri);

	QS_LOG_RETURN(ctx, sorted_uri);
	return sorted_uri;
}

const char *
vmod_filtersep(const struct vrt_ctx *ctx)
{
	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	return NULL;
}

const char *
vmod_filter(const struct vrt_ctx *ctx, const char *uri, const char *params, ...)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\", \"%s\", ...", uri, params);

	context.type = filter;
	context.ws = ctx->ws;
	context.uri = uri;
	context.params.filter.params = params;
	context.is_filtered = &is_param_filtered;
	context.is_kept = false;

	va_start(context.params.filter.ap, params);
	filtered_uri = filter_querystring(&context);
	va_end(context.params.filter.ap);

	QS_LOG_RETURN(ctx, filtered_uri);
	return filtered_uri;
}

const char *
vmod_filter_except(const struct vrt_ctx *ctx, const char *uri, const char *params, ...)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\", \"%s\", ...", uri, params);

	context.type = filter;
	context.ws = ctx->ws;
	context.uri = uri;
	context.params.filter.params = params;
	context.is_filtered = &is_param_filtered;
	context.is_kept = true;

	va_start(context.params.filter.ap, params);
	filtered_uri = filter_querystring(&context);
	va_end(context.params.filter.ap);

	QS_LOG_RETURN(ctx, filtered_uri);
	return filtered_uri;
}

const char *
vmod_regfilter(const struct vrt_ctx *ctx, const char *uri, const char *regex)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\", \"%s\"", uri, regex);

	context.type = regfilter;
	context.ws = ctx->ws;
	context.uri = uri;
	context.params.regfilter.regex = regex;
	context.params.regfilter.re_ctx = ctx;
	context.is_filtered = &is_param_regfiltered;
	context.is_kept = false;

	filtered_uri = filter_querystring(&context);

	QS_LOG_RETURN(ctx, filtered_uri);
	return filtered_uri;
}

const char *
vmod_regfilter_except(const struct vrt_ctx *ctx, const char *uri, const char *regex)
{
	struct filter_context context;
	const char *filtered_uri;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	QS_LOG_CALL(ctx, "\"%s\", \"%s\"", uri, regex);

	context.type = regfilter;
	context.ws = ctx->ws;
	context.uri = uri;
	context.params.regfilter.regex = regex;
	context.params.regfilter.re_ctx = ctx;
	context.is_filtered = &is_param_regfiltered;
	context.is_kept = true;

	filtered_uri = filter_querystring(&context);

	QS_LOG_RETURN(ctx, filtered_uri);
	return filtered_uri;
}

#endif
