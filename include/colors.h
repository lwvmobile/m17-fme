/*-------------------------------------------------------------------------------
 * colors.h
 * Project M17 - Collection of ANSI Color Codes for Terminal
 *
 * defined by CMakeLists.txt -- Enable by using cmake -DCOLORS=ON ..
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#ifdef PRETTY_COLORS
#define KNRM  "\x1B[0m"
#define KBLK  "\x1B[30m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define BBLK  "\x1B[40m"
#define BWHT  "\x1B[47m"
#define BNRM  "\x1B[49m"
#else
#define KNRM  ""
#define KBLK  ""
#define KRED  ""
#define KGRN  ""
#define KYEL  ""
#define KBLU  ""
#define KMAG  ""
#define KCYN  ""
#define KWHT  ""
#define BBLK  ""
#define BWHT  ""
#define BNRM  ""
#endif