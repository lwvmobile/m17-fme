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

//input bit array, return output as up to a 64-bit value
uint64_t convert_bits_into_output(uint8_t * input, int len)
{
  uint64_t output = 0;
  int i;
  for(i = 0; i < len; i++)
  {
    output <<= 1;
    output |= (uint64_t)(input[i] & 1);
  }
  return output;
}

//take x amount of bits and pack into len amount of bytes
void pack_bit_array_into_byte_array (uint8_t * input, uint8_t * output, int len)
{
  int i;
  for (i = 0; i < len; i++)
    output[i] = (uint8_t)convert_bits_into_output(&input[i*8], 8);
}

//take x amount of bytes and unpack back into a bit array
void unpack_byte_array_into_bit_array (uint8_t * input, uint8_t * output, int len)
{
  int i = 0, k = 0;
  for (i = 0; i < len; i++)
  {
    output[k++] = (input[i] >> 7) & 1;
    output[k++] = (input[i] >> 6) & 1;
    output[k++] = (input[i] >> 5) & 1;
    output[k++] = (input[i] >> 4) & 1;
    output[k++] = (input[i] >> 3) & 1;
    output[k++] = (input[i] >> 2) & 1;
    output[k++] = (input[i] >> 1) & 1;
    output[k++] = (input[i] >> 0) & 1;
  }
}