#ifndef MAIN_H
#define MAIN_H

//defined by CMakeLists.txt -- Enable by using cmake -DCOLORS=ON ..
#ifdef PRETTY_COLORS
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#else
#define KNRM  ""
#define KRED  ""
#define KGRN  ""
#define KYEL  ""
#define KBLU  ""
#define KMAG  ""
#define KCYN  ""
#define KWHT  ""
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <mbelib.h>
#include <sndfile.h>

//OSS support
#include <sys/soundcard.h>

//Pulse Audio Support
#include <pulse/simple.h>
#include <pulse/error.h>

//Useful Warning Shoosh
#define UNUSED(x)                       ((void)x)

//only include things if found by cmake
#ifdef USE_CODEC2
#include <codec2/codec2.h>
#endif

//RTLSDR support
#ifdef USE_RTLSDR
#include <rtl-sdr.h>
#endif

//Ncurses Support
#ifdef USE_CURSES
#include <locale.h>
#include <ncurses.h>
#endif

//signal handling exitflag
extern volatile uint8_t exitflag;

//General Options
typedef struct
{
  uint8_t a;
  char b[1024];
  char c[1024];
  uint8_t d[100];
} config_opts;

//M17 Encoder and Decoder States
typedef struct
{
  uint8_t a;
  char b[1024];
  char c[1024];
  uint8_t d[100];
} m17_state;

//Pulse Audio Options and States
typedef struct
{
  uint8_t a;
  char b[1024];
  char c[1024];
  uint8_t d[100];
} pa_state;

//c function prototypes
void framesync (config_opts * opts);

//if using cpp code, then put functions in below
#ifdef __cplusplus
extern "C" {
#endif

void sample_cpp_func(uint8_t * input);

#ifdef __cplusplus
}
#endif

#endif // MAIN_H
