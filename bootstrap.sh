#!/bin/sh

libtoolize --copy --force \
&& autoheader \
&& aclocal \
&& automake --gnu --copy --add-missing \
&& autoconf
