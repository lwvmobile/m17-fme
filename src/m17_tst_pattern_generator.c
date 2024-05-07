/*-------------------------------------------------------------------------------
 * m17_test_pattern_generator.c
 * Project M17 - Test Pattern Sine Wave Generator
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void test_pattern_generator (Super * super)
{
  //initialize RRC memory buffer
  float mem[81]; memset (mem, 0, 81*sizeof(float));
  uint8_t nil[368]; memset (nil, 0, sizeof(nil));

  for (int i = 0; i < 25; i++)
    encode_rfa (super, nil, mem, 99);

  while (!exitflag)
  {
    //generate a test pattern on loop indefinitely until exitflag
    encode_rfa (super, nil, mem, 88);
  }

}