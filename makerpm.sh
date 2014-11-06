#!/bin/bash -e
BUILD_ARCH=x86_64
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SCRIPTDIR

PACKAGENAME="jsonMerger"

echo "removing old build area"
rm -rf /tmp/jsonMerger-build-tmp
echo "creating new build area"
mkdir  /tmp/jsonMerger-build-tmp
ls
cd     /tmp/jsonMerger-build-tmp
mkdir BUILD
mkdir RPMS
TOPDIR=$PWD
echo "working in $PWD"

cd $TOPDIR
# we are done here, write the specs and make the fu***** rpm
cat > jsonMerger.spec <<EOF
Name: $PACKAGENAME
Version: 0.1
Release: 2
Summary: jsonMerger tool
License: gpl
Group: DAQ
Packager: smorovic
Source: none
%define _topdir $TOPDIR
BuildArch: $BUILD_ARCH
AutoReqProv: no

Provides:/usr/bin/jsonMerger

%description
jsonMerger tool

%prep
%build
#todo

%install
rm -rf \$RPM_BUILD_ROOT
mkdir -p \$RPM_BUILD_ROOT
%__install -d "%{buildroot}/usr/bin"

mkdir -p usr/bin
cp $SCRIPTDIR/jsonMerger %{buildroot}/usr/bin/jsonMerger

%files
%defattr(-, root, root, -)
%attr( 755 ,root, root) /usr/bin/jsonMerger

EOF

rpmbuild --target noarch --define "_topdir `pwd`/RPMBUILD" -bb jsonMerger.spec

