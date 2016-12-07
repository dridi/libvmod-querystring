================
vmod-querystring
================

.. image:: https://travis-ci.org/Dridi/libvmod-querystring.svg
   :alt: Travis CI badge
   :target: https://travis-ci.org/Dridi/libvmod-querystring/
.. image:: https://codecov.io/gh/Dridi/libvmod-querystring/branch/master/graph/badge.svg
   :alt: Codecov badge
   :target: https://codecov.io/gh/Dridi/libvmod-querystring

Description
===========

The purpose of this module is to give you a fine-grained control over a URL's
query-string in Varnish Cache. It's possible to remove the query-string, clean
it, sort its parameters or filter it to only keep a subset of them.

This can greatly improve your hit ratio and efficiency with Varnish, because
by default two URLs with the same path but different query-strings are also
different. This is what the RFCs mandate but probably not what you usually
want for your web site or application.

A query-string is just a character string starting after a question mark in a
URL. But in a web context, it is usually a structured key/values store encoded
with the ``application/x-www-form-urlencoded`` media type. This module deals
with this kind of query-strings.

Examples
========

Consider the default hashing in Varnish::

    sub vcl_hash {
        hash_data(req.url);
        if (req.http.host) {
            hash_data(req.http.host);
        } else {
            hash_data(server.ip);
        }
        return (lookup);
    }

Clients requesting ``/index.html`` and ``/index.html?`` will most likely get
the exact same response with most web servers / frameworks / stacks / wossname
but Varnish will see two different URLs and end up with two duplicate objects
in the cache.

This is a problem hard to solve with Varnish alone because it requires some
knowledge of the back-end application but it can usually be mitigated with
a couple assumptions:

- the application doesn't need query-strings
- except for POST requests that are not cached
- and for analytics/tracking purposes

In this case it can be solved like this::

    import querystring;

    sub vcl_hash {
        if (req.method == "GET" || req.method == "HEAD") {
            hash_data(querystring.remove(req.url));
        }
        else {
            hash_data(req.url);
        }
        hash_data(req.http.host);
        return (lookup);
    }

This way Varnish will get the same unique hash for both ``/index.html`` and
``/index.html?`` but the back-end application will receive the original client
request. Depending on your requirements/goals, you may also take a different
approach.

Surely enough this module can do more than what a simple regular expression
substitution (``regsub``) could do, right? First, readability is improved. It
should be obvious what the previous snippet does with no regex to decipher.

Second, it makes more complex operations easier to implement. For instance,
you may want to remove Google Analytics parameters from requests because:

- they could create cache duplicates for every campaigns
- the application does not need them, only marketing folks
- the user's browser makes AJAX calls to GA regardless
- they can be delivered to marketing via ``varnishncsa``

It could be solved like this::

    import std;
    import querystring;

    sub vcl_init {
        new ga = querystring.filter();
        ga.add_regex("^utm_.*");
    }

    sub vcl_recv {
        std.log("ga:" + ga.extract(req.url, mode = keep));
        set req.url = ga.apply(req.url);
    }

This is enough to remove all Analytics parameters you may use (``utm_source``,
``utm_medium``, ``utm_campaign`` etc) and keep the rest of the query-string
unless there are no other parameters in which case it's simply removed. The
log statement allows you to get those analytics parameters (and only them) in
``varnishncsa`` using the format string ``%{VCL_Log:ga}x``.

Sometimes you might want to whitelist query strings which you know impact the 
application and just remove all the remaining query strings that bots and others
might add to your page, this can be achieved with the below.

    sub vcl_init {
        new whitelist_querystring = querystring.filter();
        whitelist_querystring.add_string("page");
    }

    sub vcl_recv {
        # Only keep ?page=
        set req.url = whitelist_querystring.apply(req.url, mode = keep);
     }

All functions are documented in the manual page ``vmod_querystring(3)``.

Installation
============

The module requires the GNU Build System, you may follow these steps::

    ./bootstrap
    make check

Arguments to the ``bootstrap`` script are passed to the underlying execution
of the generated ``configure`` script. Once ``bootstrap`` is done, you can
later run the ``configure`` script directly if you need to reconfigure your
build tree.

When building from source, you need the following dependencies:

- autoconf
- autoconf-archive
- automake
- libtool
- rst2man
- varnish (at least 4.1.4-beta1)

If you downloaded the latest release archive, there will be no ``bootstrap``
script because releases are uploaded pre-configured. Instead you need to run
``./configure`` to check and set your environment up.

In this case your dependencies are:

- rst2man
- varnish (at least 4.1.3)

You can then proceed with the installation::

    sudo make install

If your Varnish installation did not use the default ``/usr`` prefix, you need
this in your environment before running ``./bootstrap``::

    export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
    export ACLOCAL_PATH=${PREFIX}/share/aclocal

The module is then configured for an installation inside ``${PREFIX}``, unless
the ``--prefix`` option was used in the ``configure`` execution.

RPM Packaging
=============

Instead of directly installing the package you can build an RPM::

    make rpm

The resulting packages can be found in the ``rpmbuild`` directory in your
build tree.

If you need to build an RPM for a different platform you may use ``mock(1)``
with the proper ``--root`` option. All you got to do is run ``make mockbuild``
and set the desired flags in the ``MOCK_OPTS`` variable. For instance, to
build RPMs for CentOS 7::

    make mockbuild MOCK_OPTS='--root epel-7-x86_64'

The resulting packages can be found in the ``mockbuild`` directory in your
build tree.

DPKG Packaging
==============

Experimental DPKG packaging is also available, using the ``deb`` target::

    make deb

The resulting packages can be found at the top of your build tree.

If you need to build a Deb for a specific platform you may use ``pdebuild(1)``
and ``pbuilder(8)`` to set up the base tarball and then run ``make pdebuild``
and set the desired flags in the ``PDEBUILD_OPTS`` variable. For instance to
build debs for Debian Sid, assuming your environment is properly configured
to switch between distributions::

    make pdebuild PDEBUILD_OPTS='-- --distribution sid'

The resulting packages can be found in the ``pdebuild`` directory in your
build tree.

See also
========

To learn more about query-strings and HTTP caching, you can have a look at the
relevant RFCs:

- `RFC 1866 Section 8.2.1`__: The form-urlencoded Media Type
- `RFC 3986 Section 3`__: Syntax Components
- `RFC 7234 Section 2`__: Overview of Cache Operation

__ https://tools.ietf.org/html/rfc1866#section-8.2.1
__ https://tools.ietf.org/html/rfc3986#section-3
__ https://tools.ietf.org/html/rfc7234#section-2

The test suite also shows the differences in cache hits and misses with and
without the use of this module.
