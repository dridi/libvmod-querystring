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

All functions are documented in the manual page ``vmod_querystring(3)``.

Installation
============

The module relies on the GNU Build System, also known as autotools. To install
it, start by grabbing the latest release [1]_ and follow these steps::

    # Get to the source tree
    tar -xzf vmod-querystring-${VERSION}.tar.gz
    cd vmod-querystring-${VERSION}

    # Build and install
    ./configure
    make
    make check # optional
    sudo make install

You only need to have Varnish (at least 6.0.6) and its development files
installed on your system. Instead of manually installing the module you can
build packages, see below. The ``configure`` script also needs ``pkg-config``
installed to find Varnish development files.

If your Varnish installation did not use the default ``/usr`` prefix, you
will likely need to at least set the ``pkg-config`` path to find your Varnish
installation. For example add this in your environment before running
``./configure``::

    export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig

Or the approach recommended by autoconf::

    ./configure PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig ...

The module is then configured for an installation inside ``${PREFIX}``, unless
the ``--prefix`` option was used in the ``configure`` execution. For more
information about what can be configured, run ``./configure --help``.

Alongside the release archive, you will find a PDF export of the module's
manual.

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

DPKG packaging is also available with ``dpkg-buildpackage(1)``, using the
``deb`` target::

    make deb

It is possible to either redefine the ``DPKG_BUILDPACKAGE`` command or simply
add options via ``DPKG_BUILDPACKAGE_OPTS``. For example to specify a specific
privilege escalation method::

    make deb DPKG_BUILDPACKAGE_OPTS=-rfakeroot

The resulting packages can be found in the ``dpkgbuild`` directory in your
build tree. By default sources and changes are NOT signed, in order to sign
packages the ``DPKG_BUILDPACKAGE`` variable MUST be redefined.

If you need to build a Deb for a specific platform you may use ``pdebuild(1)``
and ``pbuilder(8)`` to set up the base tarball and then run ``make pdebuild``
and set the desired flags in the ``PDEBUILD_OPTS`` variable. For instance to
build debs for Debian Sid, assuming your environment is properly configured
to switch between distributions::

    make pdebuild PDEBUILD_OPTS='-- --distribution sid'

The resulting packages can be found in the ``pdebuild`` directory in your
build tree.

As an alternative to ``pdebuild(1)`` you may prefer ``sbuild(1)`` instead.
Similarly, you may run ``make sbuild`` and set the desired flags in the
``SBUILD_OPTS`` variable. For instance to build debs for Debian Sid, assuming
your environment is properly configured to switch between distributions::

    make sbuild SBUILD_OPTS='--dist sid'

The resulting packages can be found in the ``sbuild`` directory in your
build tree.

Hacking
=======

When working on the source code, there are additional dependencies:

- autoconf
- automake
- libtool
- rst2man (python3-docutils)
- varnish (at least 6.0.6)

You will notice the lack of a ``configure`` script, it needs to be generated
with the various autotools programs. Instead, you can use the ``bootstrap``
script that takes care of both generating and running ``configure``. It also
works for VPATH_ builds.

.. _VPATH: https://www.gnu.org/software/automake/manual/html_node/VPATH-Builds.html

Arguments to the ``bootstrap`` script are passed to the underlying execution
of the generated ``configure`` script. Once ``bootstrap`` is done, you can
later run the ``configure`` script directly if you need to reconfigure your
build tree or use more than one VPATH.

If your Varnish installation did not use the default ``/usr`` prefix, you need
this in your environment before running ``./bootstrap``::

    export ACLOCAL_PATH=${PREFIX}/share/aclocal

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

.. [1] https://github.com/Dridi/libvmod-querystring/releases/latest
