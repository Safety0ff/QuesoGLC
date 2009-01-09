#!/bin/sh

set -x
aclocal -I build/m4
libtoolize --force --copy
autoheader
automake --foreign --add-missing --copy
autoconf
