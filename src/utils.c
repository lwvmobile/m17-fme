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

//get hhmmss timestamp no colon
char * getTime()
{
  char * curr = (char *) malloc(9);
  time_t t = time(NULL);
  struct tm * ptm = localtime(& t);
  sprintf(curr,"%02d%02d%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  return curr;
}

//get hh:mm:ss timestamp with colon (Ncurses Display)
char * getTimeC()
{
  char * curr = (char *) malloc(9);
  time_t t = time(NULL);
  struct tm * ptm = localtime(& t);
  sprintf(curr, "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  return curr;
}

//get hh:mm:ss timestamp with colon (Ncurses Call History)
char * getTimeN(time_t t)
{
  char * curr = (char *) malloc(9);
  struct tm * ptm = localtime(& t);
  sprintf(curr, "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  return curr;
}

//get YYYYMMDD without hyphen
char * getDate()
{
  char * curr = (char *) malloc(25);
  time_t t = time(NULL);
  struct tm * ptm = localtime(& t);
  sprintf(curr,"%04d%02d%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
  return curr;
}

//get YYYY-MM-DD without hyphen (Ncurses Display)
char * getDateH()
{
  char * curr = (char *) malloc(27);
  time_t t = time(NULL);
  struct tm * ptm = localtime(& t);
  sprintf(curr, "%04d-%02d-%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
  return curr;
}

//get YYYY-MM-DD with hyphen (Ncurses Call History)
char * getDateN(time_t t)
{
  char * curr = (char *) malloc(27);
  struct tm * ptm = localtime(& t);
  sprintf(curr, "%04d-%02d-%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
  return curr;
}
