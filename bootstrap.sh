#!/bin/sh

glibtoolize --copy --force \
&& autoheader \
&& aclocal \
&& automake --gnu --copy --add-missing \
&& autoconf
