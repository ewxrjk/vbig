#! /bin/bash
set -e
srcdir=$(dirname $0)
here=$(pwd)
cd $srcdir
mkdir -p config.aux
if test -d $HOME/share/aclocal; then
  aclocal --acdir=$HOME/share/aclocal
else
  aclocal
fi
autoconf
autoheader
automake -a || true		# for INSTALL
automake --foreign -a
