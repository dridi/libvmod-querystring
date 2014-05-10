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
 *
 *
 * This file manages API changes in order to cross-compile against
 * different versions of Varnish:
 *
 * * Varnish 4.0.0
 * - cache.h has been moved
 * - provides vrt_ctx instead of sess
 *
 * * Varnish 3.0.3
 * - VRT_re_match needs a sess pointer
 */

#if VARNISH_MAJOR == 3

#include "cache.h"

#if defined HAVE_VARNISH_3_0_3 || defined HAVE_VARNISH_3_0_4 \
 || defined HAVE_VARNISH_3_0_5
#define QS_NEED_RE_CTX
#endif

#define QS_LOG_CALL(sp, pattern, ...) \
	WSP(sp, SLT_VCL_call, "%s(" pattern ")", __func__, __VA_ARGS__);

#define QS_LOG_RETURN(sp, value) WSP(sp, SLT_VCL_return, "\"%s\"", value);

typedef struct sess re_ctx;

#endif // VARNISH_MAJOR == 3

/* ------------------------------------------------------------------- */

#if VARNISH_MAJOR == 4

#include "cache/cache.h"

#define QS_NEED_RE_CTX

#define QS_LOG_CALL(ctx, pattern, ...) \
	VSLb(ctx->vsl, SLT_VCL_call, "%s(" pattern ")", __func__, __VA_ARGS__);

#define QS_LOG_RETURN(ctx, value) VSLb(ctx->vsl, SLT_VCL_return, "\"%s\"", value);

typedef const struct vrt_ctx re_ctx;

#endif // VARNISH_MAJOR == 4

/* ------------------------------------------------------------------- */

#ifndef QS_ENABLE_LOGGING

#undef QS_LOG_CALL
#undef QS_LOG_RETURN

#define QS_LOG_CALL
#define QS_LOG_RETURN

#endif

/* ------------------------------------------------------------------- */

struct query_param {
	const char *value;
	short length;
};

enum filter_type {clean, filter, regfilter};

struct filter_params {
	const char *params;
	va_list    ap;
};

struct regfilter_params {
	const char *regex;
	void       *re;
	re_ctx     *re_ctx;
};

struct filter_context {
	enum filter_type type;
	struct ws        *ws;
	const char       *uri;
	const char       *query_string;
	union {
		struct filter_params    filter;
		struct regfilter_params regfilter;
	} params;
	bool (*is_filtered) (const char*, int, struct filter_context*);
	bool   is_kept;
};

