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

Varnish multi-purpose vmod for URI query-string manipulation.

FUNCTIONS
=========

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

INSTALLATION
============

The source tree is based on autotools to configure the building, and
does also have the necessary bits in place to do functional unit tests
using the varnishtest tool.

You need to download Varnish source code and build it locally. It is
needed for varnishtest invocation. Download the appropriate Varnish 3
source tarball (http://repo.varnish-cache.org/source/) and extract it
somewhere on your disk (eg. /usr/src/).

Build Varnish::

 cd /usr/src/varnish-3.0.x/
 ./autogen.sh
 ./configure
 make

You also need QuerySort development files. You can either install the
devel package or install it using make. See installation instuctions
(https://github.com/dridi/querysort).

For manual installation, download the appropriate QuerySort source
tarball (https://github.com/dridi/querysort/downloads) and extract it
somewhere on your disk (eg. /usr/src/).

Build QuerySort::

 cd /usr/src/querysort-1.0.x/
 make
 sudo make install
 sudo ldconfig

The libvmod-querystring module can then be built.

Usage::

 ./autogen.sh
 ./configure VARNISHSRC=/usr/src/varnish-3.0.x/ [VMODDIR=`DIR`]
 make
 sudo make install

`VARNISHSRC` is the directory of the Varnish source tree for which to
compile your vmod. Both the `VARNISHSRC` and `VARNISHSRC/include`
will be added to the include search paths for your module.

Optionally you can also set the vmod install directory by adding
`VMODDIR=DIR` (defaults to the pkg-config discovered directory from your
Varnish installation).

Make targets:

* make - builds the vmod
* make install - installs your vmod in `VMODDIR`
* make check - runs the unit tests in ``src/tests/*.vtc``

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

