%define version	%(cat %{_topdir}/version.txt)

Name:		libthmap
Version:	%{version}
Release:	1%{?dist}
Summary:	Concurrent trie-hash map library
Group:		System Environment/Libraries
License:	BSD
URL:		https://github.com/rmind/thmap
Source0:	libthmap.tar.gz

BuildRequires:	make
BuildRequires:	libtool

%description

Concurrent trie-hash map library -- a general purpose hash map, combining
the elements of hashing and radix trie.  The implementation is written in
C11 and distributed under the 2-clause BSD license.

%prep
%setup -q -n src

%check
make tests

%build
make %{?_smp_mflags} lib \
    LIBDIR=%{_libdir}

%install
make install \
    DESTDIR=%{buildroot} \
    LIBDIR=%{_libdir} \
    INCDIR=%{_includedir} \
    MANDIR=%{_mandir}

%files
%{_libdir}/*
%{_includedir}/*
%{_mandir}/*

%changelog
