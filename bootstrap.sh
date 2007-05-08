#!/bin/sh

aclocal && automake --gnu --copy --add-missing && autoconf
