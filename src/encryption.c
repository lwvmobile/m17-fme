/*-------------------------------------------------------------------------------
 * encryption.c
 * Project M17 - Encryption Keystream Generation
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//scrambler pn sequence generation
void pn_sequence_generator (Super * super)
{
  int i = 0;
  uint32_t lfsr, bit;
  uint8_t subtype = 0;
  lfsr = super->enc.scrambler_key;

  if      (lfsr > 0 && lfsr <= 0xFF)          subtype = 0; // 8-bit key
  else if (lfsr > 0xFF && lfsr <= 0xFFFF)     subtype = 1; //16-bit key
  else if (lfsr > 0xFFFF && lfsr <= 0xFFFFFF) subtype = 2; //24-bit key
  else                                        subtype = 0; // 8-bit key (default)

  //set type and subtype, mirror to m17e
  super->enc.enc_type = 1;
  super->enc.enc_subtype = subtype;

  super->m17e.enc_et = 1;
  super->m17e.enc_st = subtype;

  fprintf (stderr, "Scrambler Key: %X; Subtype: %d;", lfsr, subtype);
  fprintf (stderr, "\npN Sequence: ");
  //run pN sequence with taps specified
  for (i = 0; i < 128*6; i++)
  {
    //get feedback bit with specidifed taps, depending on the subtype
    if (subtype == 0)
      bit = (lfsr >> 7) ^ (lfsr >> 5) ^ (lfsr >> 4) ^ (lfsr >> 3);
    else if (subtype == 1)
      bit = (lfsr >> 15) ^ (lfsr >> 14) ^ (lfsr >> 12) ^ (lfsr >> 3);
    else if (subtype == 2)
      bit = (lfsr >> 23) ^ (lfsr >> 22) ^ (lfsr >> 21) ^ (lfsr >> 16);
    else bit = 0; //should never get here, but just in case
    
    bit &= 1; //truncate bit to 1 bit (required since I didn't do it above)
    lfsr = (lfsr << 1) | bit; //shift LFSR left once and OR bit onto LFSR's LSB
    lfsr &= 0xFFFFFF; //trancate lfsr to 24-bit (really doesn't matter)
    super->enc.scrambler_pn[i] = lfsr & 1;

    //debug
    fprintf (stderr, "%d", super->enc.scrambler_pn[i]);

  }

  fprintf (stderr, "\n");
}