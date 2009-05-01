#!/bin/sh

autoheader
aclocal && automake --gnu --copy --add-missing && autoconf
