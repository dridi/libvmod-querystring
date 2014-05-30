.. libvmod-querystring - querystring manipulation module for Varnish

   libvmod-querystring - querystring manipulation module for Varnish
   
   Copyright (C) 2012-2014, Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
   All rights reserved.
   
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   1. Redistributions of source code must retain the above
      copyright notice, this list of conditions and the following
      disclaimer.
   2. Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials
      provided with the distribution.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.

================
vmod_querystring
================

--------------------------
Varnish QueryString Module
--------------------------

:Author: Dridi Boukelmoune
:Date: 2012-06-18
:Version: 0.2
:Manual section: 3

SYNOPSIS
========

import querystring;

DESCRIPTION
===========

Varnish multipurpose vmod for URI query-string manipulation. Can be used to
normalize for instance request URLs or Location response headers in various
ways. It is recommended to at least clean incoming request URLs (removing empty
parameters or query-strings), all other functions do the cleaning.

FUNCTIONS
=========

clean
------

Prototype
   STRING clean(STRING url)
Description
   Returns the given URI without empty parameters. The query-string is removed
   if empty (either before or after the removal of empty parameters).
Example
   .. sourcecode::

      set req.url = querystring.clean(req.url);

remove
------

Prototype
   STRING remove(STRING url)
Description
   Returns the given URI with its query-string removed
Example
   .. sourcecode::

      set req.url = querystring.remove(req.url);

sort
----

Prototype
   STRING sort(STRING url)
Description
   Returns the given URI with its query-string sorted
Example
   .. sourcecode::

      set req.url = querystring.sort(req.url);

filtersep
---------

Prototype
   STRING filtersep()
Description
   Returns the separator needed by the filter and filter_except functions

filter
------

Prototype
   STRING filter(STRING url, STRING_LIST parameter_names)
Description
   Returns the given URI without the listed parameters
Example
   .. sourcecode::

      set req.url = querystring.filter(req.url,
        "utm_source" + querystring.filtersep() +
        "utm_medium" + querystring.filtersep() +
        "utm_campaign");

filter_except
-------------

Prototype
   STRING filter_except(STRING url, STRING_LIST parameter_names)
Description
   Returns the given URI but only keeps the listed parameters
Example
   .. sourcecode::

      set req.url = querystring.filter(req.url,
                                       "q" + querystring.filtersep() + "p");

regfilter
---------

Prototype
   STRING regfilter(STRING url, STRING parameter_names_regex)
Description
   Returns the given URI without the parameters matching a regular expression
Example
   .. sourcecode::

      set req.url = querystring.regfilter(req.url, "utm\_.*");

regfilter_except
----------------

Prototype
   STRING regfilter_except(STRING url, STRING parameter_names_regex)
Description
   Returns the given URI but only keeps the parameters matching a regular
   expression
Example
   .. sourcecode::

      set req.url = querystring.regfilter_except(req.url, "^(q|p)$");

EXAMPLES
========

In your VCL you could then use this vmod along the following lines::

   import querystring;

   sub vcl_hash {
      # sort the URL before the request hashing
      set req.url = querystring.sort(req.url);
   }

ACKNOWLEDGMENT
==============

The sort algorithm is a mix of Jason Mooberry's Skwurly and my own QuerySort
with regards for the Varnish workspace memory model of the worker threads.

COPYRIGHT
=========

This document is licensed under the same license as the
libvmod-querystring project. See LICENSE for details.

* Copyright (c) 2012-2014 Dridi Boukelmoune
