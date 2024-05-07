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
  int i;
  uint64_t output = 0;
  for(i = 0; i < len; i++)
  {
    output <<= 1;
    output |= (uint64_t)(input[i] & 1);
  }
  return output;
}

//take x amount of bits and pack into len amount of bytes (symmetrical)
void pack_bit_array_into_byte_array (uint8_t * input, uint8_t * output, int len)
{
  int i;
  for (i = 0; i < len; i++)
    output[i] = (uint8_t)convert_bits_into_output(&input[i*8], 8);
}

//take len amount of bits and pack into x amount of bytes (asymmetrical)
void pack_bit_array_into_byte_array_asym (uint8_t * input, uint8_t * output, int len)
{
  int i = 0; int k = len % 8;
  for (i = 0; i < len; i++)
  {
    output[i/8] <<= 1;
    output[i/8] |= input[i];
  }
  //if any leftover bits that don't flush the last byte fully packed, shift them over left
  if (k)
    output[i/8] <<= 8-k;
}

//take len amount of bytes and unpack back into a bit array
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

//convenience function to convert a dibit buffer array into 
//a binary buffer array, len is length of input buffer
void convert_dibit_array_into_binary_array (uint8_t * input, uint8_t * output, int len)
{
  int i;
  for (i = 0; i < len; i++)
  {
    output[(i*2)+0] = (input[i] >> 0) & 1;
    output[(i*2)+1] = (input[i] >> 1) & 1;
  }
}