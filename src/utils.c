/*-------------------------------------------------------------------------------
 * utils.c
 * Project M17 - Misc Utility Functions
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
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