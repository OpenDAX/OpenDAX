#!/bin/sh

#Uncomment this line for Max OSX
glibtoolize --copy --force \
#Uncomment this line for Linux, BSD and others
#libtoolize --copy --force \

&& autoheader \
&& aclocal \
&& automake --gnu --copy --add-missing \
&& autoconf
