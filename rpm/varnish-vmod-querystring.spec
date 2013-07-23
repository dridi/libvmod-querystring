%define VMODNAME   libvmod-querystring
%define VMODSRC    %{_builddir}/%{VMODNAME}-%{version}
%define VARNISHSRC %{_builddir}/varnish-%{VARNISHVER}


Summary: QueryString VMOD for Varnish
Name:    varnish-vmod-querystring
Version: 0.2
Release: 1.varnish%{VARNISHVER}%{?dist}
Group:   System Environment/Libraries
URL:     https://www.varnish-cache.org/vmod/querystring
License: BSD

# VMODs need a varnish build from the source, this is by design
Source0: http://github.com/Dridi/%{VMODNAME}/archive/v%{version}.tar.gz
Source1: http://repo.varnish-cache.org/source/varnish-%{VARNISHVER}.tar.gz

# fedora patches for varnish 3.0.3
Patch1:  varnish.no_pcre_jit.patch
Patch2:  varnish.fix_ppc64_upstream_bug_1194.patch

BuildRoot:     %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires:      varnish = %{VARNISHVER}
BuildRequires: ncurses-devel libxslt groff pcre-devel pkgconfig jemalloc-devel
BuildRequires: autoconf automake libtool python-docutils


%description
Varnish multipurpose module for URI query-string manipulation. Can be used to
normalize for instance request URLs or Location response headers in various
ways. It is recommended to at least clean incoming request URLs (removing empty
query-strings), all other functions do the cleaning.


%prep
%setup -q -b 0 -n %{VMODNAME}-%{version}
%setup -q -b 1 -n varnish-%{VARNISHVER}

cd %{VARNISHSRC}

%if 0%{?fedora} != 0 && "%{VARNISHVER}" == "3.0.3"
%ifarch i386 i686 ppc
%patch1
%endif

%patch2
%endif


%build
cd %{VARNISHSRC}
%{configure}
make %{?_smp_mflags}

cd %{VMODSRC}
VMODDIR="$(PKG_CONFIG_PATH=%{VARNISHSRC} pkg-config --variable=vmoddir varnishapi)"
./autogen.sh
%{configure} VARNISHSRC=%{VARNISHSRC} VMODDIR="$VMODDIR" --docdir=%{_docdir}/%{name}-%{version}
make %{?_smp_mflags}


%install
cd %{VMODSRC}
make install DESTDIR=%{buildroot}
rm %{buildroot}/%{_libdir}/varnish/vmods/libvmod_querystring.la


%check
cd %{VMODSRC}
make %{?_smp_mflags} check


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%{_libdir}/varnish/vmods/libvmod_querystring.so
%dir %{_docdir}/%{name}-%{version}
%doc %{_docdir}/%{name}-%{version}/*
%{_mandir}/man?/*


%changelog
* Mon Apr 15 2013 Dridi Boukelmoune <dridi.boukelmoune@gmail.com> - 0.2-1
- Initial version.
