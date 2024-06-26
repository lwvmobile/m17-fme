/*-------------------------------------------------------------------------------
 * time.c
 * M17 Project - Time and Date Functions
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//WARNING: time(NULL) is an expensive lag causing call when we need low latency (unknown reason)

//get HHmmss timestamp no colon (file operations)
char * get_time()
{
  char * curr = calloc(7, sizeof(char));
  time_t t = time(NULL);
  struct tm * ptm = localtime(& t);
  sprintf(curr,"%02d%02d%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  return curr;
}

//get HH:mm:ss timestamp with colon (display / console output)
char * get_time_n(time_t t)
{
  char * curr = calloc(9, sizeof(char));
  struct tm * ptm = localtime(& t);
  sprintf(curr, "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  return curr;
}

//get YYYYMMDD without hyphen (file operations)
char * get_date()
{
  char * curr = calloc(25, sizeof(char));
  time_t t = time(NULL);
  struct tm * ptm = localtime(& t);
  sprintf(curr,"%04d%02d%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
  return curr;
}

//get YYYY-MM-DD with hyphen (display / console output)
char * get_date_n(time_t t)
{
  char * curr = calloc(27, sizeof(char));
  struct tm * ptm = localtime(& t);
  sprintf(curr, "%04d-%02d-%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
  return curr;
}
