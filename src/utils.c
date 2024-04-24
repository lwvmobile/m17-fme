/*-------------------------------------------------------------------------------
 * utils.c
 * Project M17 - Misc Utility Functions
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void open_stdout_pipe(Super * super)
{
  super->opts.stdout_pipe = fileno(stdout);
}

void write_stdout_pipe(Super * super, short * out, size_t nsam)
{
  write (super->opts.stdout_pipe, out, nsam*2);
}

uint64_t ConvertBitIntoBytes(uint8_t * BufferIn, uint32_t BitLength)
{
  uint64_t Output = 0;
  uint32_t i;

  for(i = 0; i < BitLength; i++)
  {
    Output <<= 1;
    Output |= (uint64_t)(BufferIn[i] & 1);
  }

  return Output;
}

char * getTime() //get hhmmss timestamp (Windows files can't have colons in them)
{
  char * curr = (char *) malloc(9);
  time_t t = time(NULL);
  struct tm * ptm = localtime(& t);
  sprintf(
    curr,
    "%02d%02d%02d",
    ptm->tm_hour,
    ptm->tm_min,
    ptm->tm_sec
  );
  return curr;
}

char * getTimeC() //get hh:mm:ss timestamp (Ncurses Display)
{
  char * curr = (char *) malloc(9);
  time_t t = time(NULL);
  struct tm * ptm = localtime(& t);
  sprintf(
    curr,
    "%02d:%02d:%02d",
    ptm->tm_hour,
    ptm->tm_min,
    ptm->tm_sec
  );
  return curr;
}

char * getDate()
{
  char * datename = (char *) malloc(9);
  char * curr;
  struct tm * to;
  time_t t;
  t = time(NULL);
  to = localtime( & t);
  strftime(datename, sizeof(datename), "%Y%m%d", to);
  curr = strtok(datename, " ");
  return curr;
}