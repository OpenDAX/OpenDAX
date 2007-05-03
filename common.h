#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#ifndef PID_FILE_PATH
  #define PID_FILE_PATH "/var/run"
#endif /* !PID_FILE_PATH */
