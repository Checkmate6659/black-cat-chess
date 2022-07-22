#ifndef __POSIX_H__
#define __POSIX_H__

// #include <cerrno>
// #include <cstdio>

// #if defined(_WIN32) || defined(_WIN64)
// #  include <windows.h>
// #else // assume POSIX
// #  include <sys/resource.h>
// // #  include <sys/select.h>
// #  include <sys/time.h>
// #  include <sys/types.h>
// #  include <unistd.h>
// #endif

// bool input_available ();




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
