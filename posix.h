#ifndef __POSIX_H__
#define __POSIX_H__


#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#  include <conio.h> //on Windows systems, just include conio.h

#else //assume POSIX: use custom kbhit() function defined at termios level
#  include <termios.h>
#  include <sys/select.h>
#  include <sys/ioctl.h>

   int kbhit();

#endif

#endif
