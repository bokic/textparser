%global git_version %(git describe --tags --abbrev=0 2>/dev/null | sed 's/^v//' || echo 1.0.12)

Name:           textparser
Version:        %{?pkg_version}%{!?pkg_version:%{git_version}}
Release:        1%{?dist}
Summary:        High-performance, extensible text parsing CLI

License:        MIT
URL:            https://github.com/bokic/textparser
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.15
BuildRequires:  ninja-build
BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  python3
BuildRequires:  pcre2-devel >= 10.42
BuildRequires:  json-c-devel
BuildRequires:  gtest-devel

Requires:       libtextparser%{?_isa} = %{version}-%{release}
Requires:       libtextparser-json%{?_isa} = %{version}-%{release}

%description
TextParser is a high-performance, extensible text parsing CLI and library
written in C. It uses regular expressions and native matchers to parse
language grammars and produce hierarchical Abstract Syntax Trees (ASTs).

This package provides the `textparser` CLI tool.

%package -n ccat
Summary:        Colorizing cat utility using textparser
Requires:       libtextparser%{?_isa} = %{version}-%{release}

%description -n ccat
ccat is a syntax-highlighting colorizing alternative to cat powered by
the textparser library.

%package -n libtextparser
Summary:        Core textparser shared library

%description -n libtextparser
Shared library for the TextParser high-performance text parsing engine.

%package -n libtextparser-json
Summary:        JSON definition runtime loader for textparser
Requires:       libtextparser%{?_isa} = %{version}-%{release}

%description -n libtextparser-json
Shared library providing dynamic JSON grammar definition loading for textparser.

%package -n libtextparser-devel
Summary:        Development files for textparser
Requires:       libtextparser%{?_isa} = %{version}-%{release}
Requires:       libtextparser-json%{?_isa} = %{version}-%{release}

%description -n libtextparser-devel
Header files, pkg-config files, and CMake configuration for developing
applications with libtextparser.

%prep
%autosetup

%build
%cmake -G Ninja \
    -DTEXTPARSER_VERSION_TAG="%{version}" \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=OFF
%cmake_build

%install
%cmake_install

%check
%ctest

%files
%{_bindir}/textparser

%files -n ccat
%{_bindir}/ccat

%files -n libtextparser
%license LICENSE
%doc README.md
%{_libdir}/libtextparser.so.*

%files -n libtextparser-json
%{_libdir}/libtextparser-json.so.*

%files -n libtextparser-devel
%{_includedir}/textparser.h
%{_includedir}/textparser.hpp
%{_includedir}/textparser-json.h
%{_libdir}/libtextparser.so
%{_libdir}/libtextparser-json.so
%{_libdir}/pkgconfig/textparser.pc
%{_libdir}/pkgconfig/textparser-json.pc
%{_libdir}/cmake/textparser/

%changelog
* Sun Aug 30 2026 Boris <boris@example.com> - 1.0.12-1
- Initial Fedora package release with subpackages
