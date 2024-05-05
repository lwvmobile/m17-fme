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
  fprintf (stderr, "\npN: ");
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
    if ((i != 0) && (i%64 == 0) ) fprintf (stderr, "\n    ");
    fprintf (stderr, "%d", super->enc.scrambler_pn[i]);

  }

  fprintf (stderr, "\n");
}

//load an AES key based on user argument
void aes_key_loader (Super * super)
{
  int i;

  //load AES key from A1 A2 A3 A4 optarg chunks to array
  super->enc.aes_key[0]  = (super->enc.A1 >> 56ULL) & 0xFF;
  super->enc.aes_key[1]  = (super->enc.A1 >> 48ULL) & 0xFF;
  super->enc.aes_key[2]  = (super->enc.A1 >> 40ULL) & 0xFF;
  super->enc.aes_key[3]  = (super->enc.A1 >> 32ULL) & 0xFF;
  super->enc.aes_key[4]  = (super->enc.A1 >> 24ULL) & 0xFF;
  super->enc.aes_key[5]  = (super->enc.A1 >> 16ULL) & 0xFF;
  super->enc.aes_key[6]  = (super->enc.A1 >>  8ULL) & 0xFF;
  super->enc.aes_key[7]  = (super->enc.A1 >>  0ULL) & 0xFF;

  super->enc.aes_key[8]  = (super->enc.A2 >> 56ULL) & 0xFF;
  super->enc.aes_key[9]  = (super->enc.A2 >> 48ULL) & 0xFF;
  super->enc.aes_key[10] = (super->enc.A2 >> 40ULL) & 0xFF;
  super->enc.aes_key[11] = (super->enc.A2 >> 32ULL) & 0xFF;
  super->enc.aes_key[12] = (super->enc.A2 >> 24ULL) & 0xFF;
  super->enc.aes_key[13] = (super->enc.A2 >> 16ULL) & 0xFF;
  super->enc.aes_key[14] = (super->enc.A2 >>  8ULL) & 0xFF;
  super->enc.aes_key[15] = (super->enc.A2 >>  0ULL) & 0xFF;

  super->enc.aes_key[16] = (super->enc.A3 >> 56ULL) & 0xFF;
  super->enc.aes_key[17] = (super->enc.A3 >> 48ULL) & 0xFF;
  super->enc.aes_key[18] = (super->enc.A3 >> 40ULL) & 0xFF;
  super->enc.aes_key[19] = (super->enc.A3 >> 32ULL) & 0xFF;
  super->enc.aes_key[20] = (super->enc.A3 >> 24ULL) & 0xFF;
  super->enc.aes_key[21] = (super->enc.A3 >> 16ULL) & 0xFF;
  super->enc.aes_key[22] = (super->enc.A3 >>  8ULL) & 0xFF;
  super->enc.aes_key[23] = (super->enc.A3 >>  0ULL) & 0xFF;

  super->enc.aes_key[24] = (super->enc.A4 >> 56ULL) & 0xFF;
  super->enc.aes_key[25] = (super->enc.A4 >> 48ULL) & 0xFF;
  super->enc.aes_key[26] = (super->enc.A4 >> 40ULL) & 0xFF;
  super->enc.aes_key[27] = (super->enc.A4 >> 32ULL) & 0xFF;
  super->enc.aes_key[28] = (super->enc.A4 >> 24ULL) & 0xFF;
  super->enc.aes_key[29] = (super->enc.A4 >> 16ULL) & 0xFF;
  super->enc.aes_key[30] = (super->enc.A4 >>  8ULL) & 0xFF;
  super->enc.aes_key[31] = (super->enc.A4 >>  0ULL) & 0xFF;

  //evaluate and flag that a key is loaded if at least one byte has a non-zero value
  for (i = 0; i < 32; i++)
  {
    if (super->enc.aes_key[i] != 0)
    {
      super->enc.aes_key_is_loaded = 1;
      super->enc.enc_type = 2;
      super->m17e.enc_et = 2;
      break;
    }
  }

  //print the loaded key for user confirmation
  if (super->enc.aes_key_is_loaded)
  {
    fprintf (stderr, "AES Key:");
    for (i = 0; i < 32; i++)
    {
      if (i == 16) fprintf (stderr, "\n        ");
      fprintf (stderr, " %02X", super->enc.aes_key[i]);
    }
    fprintf (stderr, "\n");
  }

}