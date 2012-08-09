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
#include <stdarg.h>
#include <stdbool.h>
#include <querysort.h>

#include "vrt.h"
#include "bin/varnishd/cache.h"

#include "vcc_if.h"

int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
	return 0;
}

static const char *
clean_uri_querystring(struct sess *sp, const char *uri, const char *query_string)
{
	struct ws *ws = sp->wrk->ws;
	int query_string_position = query_string - uri;
	char *clean_uri = WS_Alloc(ws, query_string_position);

	WS_Assert(ws);

	if (clean_uri == NULL) {
		return uri;
	}

	memcpy(clean_uri, uri, query_string_position);
	clean_uri[query_string_position] = '\0';

	return clean_uri;
}

const char *
vmod_clean(struct sess *sp, const char *uri)
{
	if (uri == NULL) {
		return NULL;
	}

	char *query_string = strchr(uri, '?');
	if (query_string == NULL || query_string[1] != '\0') {
		return uri;
	}

	return clean_uri_querystring(sp, uri, query_string);
}

const char *
vmod_remove(struct sess *sp, const char *uri)
{
	if (uri == NULL) {
		return NULL;
	}

	char *query_string = strchr(uri, '?');
	if (query_string == NULL) {
		return uri;
	}

	return clean_uri_querystring(sp, uri, query_string);
}

const char *
vmod_sort(struct sess *sp, const char *uri)
{
	if (uri == NULL) {
		return NULL;
	}

	char *query_string = strchr(uri, '?');
	if (query_string == NULL) {
		return uri;
	}

	if (query_string[1] == '\0') {
		return clean_uri_querystring(sp, uri, query_string);
	}
	
	struct ws *ws = sp->wrk->ws;
	char *sorted_uri = WS_Alloc(ws, strlen(uri) + 1);
	
	WS_Assert(ws);
	
	if (sorted_uri == NULL) {
		return NULL;
	}
	
	if (qs_sort(uri, sorted_uri) != QS_OK) {
		return NULL;
	}
	
	return sorted_uri;
}

static bool
is_param_filtered(const char *param, int length, const char *params, va_list ap)
{
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

const char *
vmod_filter(struct sess *sp, const char *uri, const char *params, ...)
{
	if (uri == NULL) {
		return NULL;
	}

	char *query_string = strchr(uri, '?');
	if (query_string == NULL) {
		return uri;
	}

	if (query_string[1] == '\0') {
		return clean_uri_querystring(sp, uri, query_string);
	}
	
	unsigned available = WS_Reserve(sp->wrk->ws, 0);
	char *begin = sp->wrk->ws->f;
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

		va_list ap;
		va_start(ap, params);
		if ( ! is_param_filtered(param_position, param_name_length, params, ap) ) {
			append_string(&begin, end, param_position, current_position - param_position);
			if (*current_position == '&') {
				*begin = '&';
				begin++;
			}
		}
		va_end(ap);
	}

	if (begin < end) {
		begin -= (begin[-1] == '&');
		*begin = '\0';
	}
	
	begin++;

	if (begin > end) {
		WS_Release(sp->wrk->ws, 0);
		return uri;
	}

	end = begin;
	begin = sp->wrk->ws->f;
	WS_Release(sp->wrk->ws, end - begin);
	return begin;
}

const char *
vmod_filtersep(struct sess *sp)
{
	return NULL;
}

static bool
is_param_regfiltered(const char *param, int length, void *re)
{
	char p[length + 1];

	memcpy(p, param, length);
	p[length + 1] = '\0';

	return (bool) VRT_re_match(p, re);
}

void *
compile_regex(const char *regex)
{
	void *re;
	const char *error;
	int error_offset;

	re = VRE_compile(regex, 0, &error, &error_offset);
	return re;
}

const char *
vmod_regfilter(struct sess *sp, const char *uri, const char *regex)
{
	if (uri == NULL) {
		return NULL;
	}

	char *query_string = strchr(uri, '?');
	if (query_string == NULL) {
		return uri;
	}

	if (query_string[1] == '\0') {
		return clean_uri_querystring(sp, uri, query_string);
	}

	void *re = compile_regex(regex);
	if (re == NULL) {
		return uri;
	}

	unsigned available = WS_Reserve(sp->wrk->ws, 0);
	char *begin = sp->wrk->ws->f;
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

		if ( ! is_param_regfiltered(param_position, param_name_length, re) ) {
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
		WS_Release(sp->wrk->ws, 0);
		return uri;
	}

	end = begin;
	begin = sp->wrk->ws->f;
	WS_Release(sp->wrk->ws, end - begin);
	return begin;
}

