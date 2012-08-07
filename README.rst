================
vmod_querystring
================

--------------------------
Varnish QueryString Module
--------------------------

:Author: Dridi Boukelmoune
:Date: 2012-06-18
:Version: 1.0
:Manual section: 3

SYNOPSIS
========

import querystring;

DESCRIPTION
===========

Varnish multi-purpose vmod for URI query-string manipulation. Can be used to
normalize for instance request URLs or Location response headers in various
ways. It is recommended to at least clean incoming request URLs (removing empty
query-strings), all other functions do the cleaning.

FUNCTIONS
=========

clean
------

Prototype
        ::
                clean(STRING URI)
Return value
	STRING
Description
	Returns The given URI with its query-string removed if empty
Example
        ::
                set req.url = querystring.clean(req.url);

remove
------

Prototype
        ::
                remove(STRING URI)
Return value
	STRING
Description
	Returns The given URI with its query-string removed
Example
        ::
                set req.url = querystring.remove(req.url);

sort
----

Prototype
        ::
                sort(STRING URI)
Return value
	STRING
Description
	Returns The given URI with its query-string sorted
Example
        ::
                set req.url = querystring.sort(req.url);

EXAMPLE
=======

In your VCL you could then use this vmod along the following lines::
        
        import querystring;

        sub vcl_hash {
                # sort the URL before the request hashing
                set req.url = querystring.sort(req.url);
        }

COPYRIGHT
=========

This document is licensed under the same license as the
libvmod-querystring project. See LICENSE for details.

* Copyright (c) 2012 Dridi Boukelmoune

SEE ALSO
========

querysort(3)

