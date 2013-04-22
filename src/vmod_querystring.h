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

/*
 * This file manages API changes in order to cross-compile against
 * different versions of Varnish:
 *
 * * Varnish 4.0.0
 * - cache.h has been moved
 * - struct sess renamed to req
 * - worker.ws renamed worker.aws
 *
 * * Varnish 3.0.3
 * - IS_PARAM_REGFILTERED(sp, param, length, re)
 * - VRT_RE_MATCH(sp, p, re)
 */

#ifdef HAVE_VARNISH_4_0_0

#include "cache/cache.h"

typedef struct req vmod_request;

#define WS aws

/* VRT_re_match needs a pointer to the session */
#define IS_PARAM_REGFILTERED(sp, param, length, re) is_param_regfiltered(sp, param, length, re)
#define VRT_RE_MATCH(sp, p, re) VRT_re_match(sp, p, re)

#endif

/* ------------------------------------------------------------------- */

#ifdef HAVE_VARNISH_3_0_3

#include "cache.h"

typedef struct sess vmod_request;

#define WS ws

/* VRT_re_match needs a pointer to the session */
#define IS_PARAM_REGFILTERED(sp, param, length, re) is_param_regfiltered(sp, param, length, re)
#define VRT_RE_MATCH(sp, p, re) VRT_re_match(sp, p, re)

#endif

/* ------------------------------------------------------------------- */

#ifdef HAVE_VARNISH_3_0_0

#include "cache.h"

typedef struct sess vmod_request;

#define WS ws

/* Tested with every versions of Varnish from 3.0.0 to 3.0.2 */
#define IS_PARAM_REGFILTERED(sp, param, length, re) is_param_regfiltered(param, length, re)
#define VRT_RE_MATCH(sp, p, re) VRT_re_match(p, re)

#endif

