/*-------------------------------------------------------------------------------
 * encryption.c
 * M17 Project - Encryption Keystream Generation
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//scrambler seed calculation (only run this IF fn != 0 AND local fn != scrambler_fn_d)
uint32_t scrambler_seed_calculation(int8_t subtype, uint32_t key, int fn)
{
  int i;
  uint32_t lfsr, bit;

  lfsr = key; bit = 0;
  for (i = 0; i < 128*fn; i++)
  {
    //get feedback bit with specified taps, depending on the subtype
    if (subtype == 0)
      bit = (lfsr >> 7) ^ (lfsr >> 5) ^ (lfsr >> 4) ^ (lfsr >> 3);
    else if (subtype == 1)
      bit = (lfsr >> 15) ^ (lfsr >> 14) ^ (lfsr >> 12) ^ (lfsr >> 3);
    else if (subtype == 2)
      bit = (lfsr >> 23) ^ (lfsr >> 22) ^ (lfsr >> 21) ^ (lfsr >> 16);
    else bit = 0; //should never get here, but just in case
    
    bit &= 1; //truncate bit to 1 bit
    lfsr = (lfsr << 1) | bit; //shift LFSR left once and OR bit onto LFSR's LSB
    lfsr &= 0xFFFFFF; //truncate lfsr to 24-bit

  }

  //debug
  fprintf (stderr, "\nScrambler Key: 0x%06X; Seed: 0x%06X; Subtype: %02d; FN: %05d; ", key, lfsr, subtype, fn);

  return lfsr;
}

//scrambler key init
void scrambler_key_init (Super * super, int de)
{
  int8_t scrambler_subtype = 0;
  uint32_t key = 0;
  if (de)
  {
    scrambler_subtype = super->enc.scrambler_subtype_e;
    key = super->enc.scrambler_key;
    super->enc.scrambler_seed_e = key;
  } 
  else 
  {
    scrambler_subtype = super->enc.scrambler_subtype_d;
    key = super->enc.scrambler_key;
    super->enc.scrambler_seed_d = key;
  }

  if (scrambler_subtype == -1)
  {
    if      (key > 0      && key <= 0xFF)     scrambler_subtype = 0; // 8-bit key
    else if (key > 0xFF   && key <= 0xFFFF)   scrambler_subtype = 1; //16-bit key
    else if (key > 0xFFFF && key <= 0xFFFFFF) scrambler_subtype = 2; //24-bit key
    else                                      scrambler_subtype = 0; // 8-bit key (default)
  }

  if (de)
    super->enc.scrambler_subtype_e = scrambler_subtype;
  else super->enc.scrambler_subtype_d = scrambler_subtype;

  //set type and subtype, mirror to m17e -- move to a key loading type function instead
  super->enc.enc_type = 1;
  super->enc.enc_subtype = scrambler_subtype;

  super->m17e.enc_et = 1;
  super->m17e.enc_st = scrambler_subtype;

  fprintf (stderr, "Scrambler Key: 0x%06X; Subtype: %02d;", key, scrambler_subtype);

}

//scrambler pn sequence generation
uint32_t scrambler_sequence_generator (Super * super, int de)
{
  int i = 0;
  uint32_t lfsr, bit;
  uint8_t subtype;

  //set seed and subtype based on if we are encoding, or decoding right now
  if (de)
  {
    lfsr = super->enc.scrambler_seed_e;
    subtype = super->m17e.enc_st;
  }
    
  else
  {
    lfsr = super->enc.scrambler_seed_d;
    subtype = super->m17d.enc_st;
  }

  //debug
  // fprintf (stderr, "\nScrambler Key: 0x%06X; Seed: 0x%06X; Subtype: %02d;", super->enc.scrambler_key, lfsr, subtype);

  if (super->opts.demod_verbosity > 2)
    fprintf (stderr, "\n pN: ");
  
  //run pN sequence with taps specified
  for (i = 0; i < 128; i++)
  {
    //get feedback bit with specified taps, depending on the subtype
    if (subtype == 0)
      bit = (lfsr >> 7) ^ (lfsr >> 5) ^ (lfsr >> 4) ^ (lfsr >> 3);
    else if (subtype == 1)
      bit = (lfsr >> 15) ^ (lfsr >> 14) ^ (lfsr >> 12) ^ (lfsr >> 3);
    else if (subtype == 2)
      bit = (lfsr >> 23) ^ (lfsr >> 22) ^ (lfsr >> 21) ^ (lfsr >> 16);
    else bit = 0; //should never get here, but just in case
    
    bit &= 1; //truncate bit to 1 bit (required since I didn't do it above)
    lfsr = (lfsr << 1) | bit; //shift LFSR left once and OR bit onto LFSR's LSB
    lfsr &= 0xFFFFFF; //truncate lfsr to 24-bit (really doesn't matter)
    super->enc.scrambler_pn[i] = bit;

    //debug
    if (super->opts.demod_verbosity > 2)
    {
      if ((i != 0) && (i%64 == 0) ) fprintf (stderr, " \n     ");
      fprintf (stderr, "%d", super->enc.scrambler_pn[i]);
    }

  }

  if (super->opts.demod_verbosity > 2)
    fprintf (stderr, "\n");

  //debug vs m17-coder-sym
  // uint8_t scr_bytes[16]; memset (scr_bytes, 0, 16*sizeof(uint8_t));
  // pack_bit_array_into_byte_array(super->enc.scrambler_pn, scr_bytes, 16);
  // fprintf (stderr, "\n pN: ");
  // for (i = 0; i < 16; i++)
  //     fprintf (stderr, " %02X", scr_bytes[i]);
  // fprintf (stderr, "\n");

  return lfsr;
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
      super->enc.enc_subtype = 0;
      super->m17e.enc_et = 2;
      super->m17e.enc_st = 0;
      break;
    }
  }

  //NOTE: Maintain 'compatibility' where-as m17-tools is using 128-bit AES,
  //and signals enc_st as 0, so enc_st 0 will be AES 128 (unless Spec changes that)
  int len = 32;

  //evaluate len of inserted AES key to see what the subtype will be
  if (super->enc.aes_key_is_loaded == 1)
  {
    uint64_t sum = 0; uint64_t tmp = 0;
    for (i = 0; i < 16; i++)
      tmp += super->enc.aes_key[i];
    if (tmp != sum) //128
    {
      len = 16;
      sum = tmp;
      super->enc.enc_subtype = 0;
      super->m17e.enc_st = 0;
    }
    for (i = 16; i < 24; i++)
      tmp += super->enc.aes_key[i];
    if (tmp != sum) //192
    {
      len = 24;
      sum = tmp;
      super->enc.enc_subtype = 1;
      super->m17e.enc_st = 1;
    }
    for (i = 24; i < 32; i++)
      tmp += super->enc.aes_key[i];
    if (tmp != sum) //256
    {
      len = 32;
      sum = tmp;
      super->enc.enc_subtype = 2;
      super->m17e.enc_st = 2;
    }
  }


  //print the loaded key for user confirmation
  if (super->enc.aes_key_is_loaded)
  {
    fprintf (stderr, "AES ");
    if (super->m17e.enc_st == 0)
      fprintf (stderr, "128 ");
    if (super->m17e.enc_st == 1)
      fprintf (stderr, "192 ");
    if (super->m17e.enc_st == 2)
      fprintf (stderr, "256 ");
    fprintf (stderr, "Key: ");
    for (i = 0; i < len; i++)
    {
      // if (i == 16) fprintf (stderr, "\n             ");
      fprintf (stderr, "%02X", super->enc.aes_key[i]);
    }
    // fprintf (stderr, "\n");
  }

}