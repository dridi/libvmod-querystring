================
vmod-querystring
================

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
with the MIME type ``application/x-www-form-urlencoded``. This module deals
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
- except for POST forms that are not cached
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

This way Varnish will see get the same unique hash for both ``/index.html``
and ``/index.html?`` but the back-end application will receive the original
client request. Depending on your requirements/goals, you may also take a
different approach.

Surely enough this module can do more than what a simple regular expression
substitution (``regsub``) could do, right? First, readability is improved. It
should be obvious what the previous snippet does with no regex to decipher.

Second, it makes more complex operations easier to implement. For instance,
you may want to remove Google Analytics parameters from requests because:

- they could create cache duplicates for every campaigns
- the application does not need them, only marketing folks
- they can be delivered to business people via ``varnishncsa``

It can be solved like this::

    import querystring;

    sub vcl_recv {
        set req.url = querystring.regfilter(req.url, "utm_.*");
    }

This is enough to remove all Analytics parameters you may use (``utm_source``,
``utm_medium``, ``utm_campaign`` etc) and keep the rest of the query-string
unless there are no other parameters in which case it's simply removed.

All functions are documented in the manual page ``vmod_querystring(3)``.

Installation
============

The module requires the GNU Build System, you may follow these steps::

    ./autogen.sh
    ./configure
    make check

You can then proceed with the installation::

    sudo make install

Instead of directly installing the package you can build an RPM instead::

    make dist
    rpmbuild -tb *.tar.gz

If you need to build an RPM for a different platform you may use ``mock(1)``::

    make dist
    mock --buildsrpm --resultdir . --sources . --spec vmod-querystring.spec
    mock --rebuild   --resultdir . *.src.rpm

If your Varnish installation did not use the default ``/usr`` prefix, you need
this in your environment before running ``./autogen.sh``::

    export PKG_CONFIG_PATH=/path/to/lib/pkgconfig

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
