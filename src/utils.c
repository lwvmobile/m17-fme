/*-------------------------------------------------------------------------------
 * utils.c
 * Project M17 - Misc Utility Functions
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//simple 6x 8K to 48K upsample
void upsample_6x(short input, short * output)
{
  int i;
  for (i = 0; i < 6; i++)
    output[i] = input;
}

void open_stdout_pipe(config_opts * opts)
{
  opts->stdout_pipe = fileno(stdout);
}

void write_stdout_pipe(config_opts * opts, short * out, size_t nsam)
{
  write (opts->stdout_pipe, out, nsam*2);
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