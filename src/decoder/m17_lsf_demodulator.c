/*-------------------------------------------------------------------------------
 * m17_lsf_demodulator.c
 * M17 Project - Link Setup Frame Demodulation and Debug
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void demod_lsf(Super * super, uint8_t * input, int debug)
{
  //quell defined but not used warnings from m17.h
  stfu ();

  int i;
  uint8_t dbuf[184]; //384-bit frame - 16-bit (8 symbol) sync pattern (184 dibits)
  float   sbuf[184]; //float symbol buffer

  memset(dbuf, 0, sizeof(dbuf));
  memset(sbuf, 0.0f, sizeof(sbuf));

  //if not running in debug / encoder mode, then perform dibit collection
  if (debug == 0)
  {
    //load dibits into dibit buffer
    for (i = 0; i < 184; i++)
      dbuf[i] = get_dibit(super);

    //convert dbuf into a symbol array
    for (i = 0; i < 184; i++)
    {
      if      (dbuf[i] == 0) sbuf[i] = +1.0f;
      else if (dbuf[i] == 1) sbuf[i] = +3.0f;
      else if (dbuf[i] == 2) sbuf[i] = -1.0f;
      else if (dbuf[i] == 3) sbuf[i] = -3.0f;
      else                   sbuf[i] = +0.0f;
    }
    
  }
  else //we are debugging, and convert input to symbols
  {
    //load dibits into dibit buffer from input bits
    for (i = 0; i < 184; i++)
      dbuf[i] = (input[(i*2)+0] << 1) | input[(i*2)+1];

    //convert dbuf into a symbol array
    for (i = 0; i < 184; i++)
    {
      if      (dbuf[i] == 0) sbuf[i] = +1.0f;
      else if (dbuf[i] == 1) sbuf[i] = +3.0f;
      else if (dbuf[i] == 2) sbuf[i] = -1.0f;
      else if (dbuf[i] == 3) sbuf[i] = -3.0f;
      else                   sbuf[i] = +0.0f;
    }
  }

  //
  uint32_t error = 0;                 //viterbi error
  uint16_t soft_bit[2*SYM_PER_PLD];   //raw frame soft bits
  uint16_t d_soft_bit[2*SYM_PER_PLD]; //deinterleaved soft bits
  uint8_t  lsf_vit[31];               //packed LSF frame

  memset(soft_bit, 0, sizeof(soft_bit));
  memset(d_soft_bit, 0, sizeof(d_soft_bit));
  memset(lsf_vit, 0, sizeof(lsf_vit));

  //libm17 magic
  //slice symbols to soft dibits
  slice_symbols(soft_bit, sbuf);

  //derandomize
  randomize_soft_bits(soft_bit);

  //deinterleave
  reorder_soft_bits(d_soft_bit, soft_bit);

  //viterbi
  error = viterbi_decode_punctured(lsf_vit, d_soft_bit, puncture_pattern_1, 2*SYM_PER_PLD, 61);

  //unpack into the lsf bit array
  memset (super->m17d.lsf, 0, sizeof(super->m17d.lsf));
  unpack_byte_array_into_bit_array(lsf_vit+1, super->m17d.lsf, 30);

  uint8_t lsf_packed[30];
  memset (lsf_packed, 0, sizeof(lsf_packed));

  //need to pack bytes for the sw5wwp variant of the crc (might as well, may be useful in the future)
  for (i = 0; i < 30; i++)
    lsf_packed[i] = (uint8_t)convert_bits_into_output(&super->m17d.lsf[i*8], 8);

  uint16_t crc_cmp = crc16(lsf_packed, 28);
  uint16_t crc_ext = (uint16_t)convert_bits_into_output(&super->m17d.lsf[224], 16);
  int crc_err = 0;

  if (crc_cmp != crc_ext) crc_err = 1;

  if (crc_err == 0)
    decode_lsf_contents(super);
  else if (super->opts.allow_crc_failure == 1)
    decode_lsf_contents(super);

  if (super->opts.payload_verbosity >= 1)
  {
    fprintf (stderr, "\n LSF:");
    for (i = 0; i < 30; i++)
    {
      if (i == 15) fprintf (stderr, "\n     ");
      fprintf (stderr, " %02X", lsf_packed[i]);
    }
    fprintf (stderr, "\n      (CRC CHK) E: %04X; C: %04X; V-Error: %d;", crc_ext, crc_cmp, error);
  }

  if (crc_err == 1) fprintf (stderr, " CRC ERR");

  //track errors
  if (crc_err == 1) super->error.lsf_hdr_crc_err++;

  //get rid of this if it costs too much CPU / skips / lags
  super->demod.sync_time = super->demod.current_time = time(NULL);

  //refresh ncurses printer, if enabled
  #ifdef USE_CURSES
  if (super->opts.use_ncurses_terminal == 1)
    print_ncurses_terminal(super);
  #endif

}


void p1_predictive_depuncture(Super * super, uint8_t * input, uint8_t * output)
{

  //NOTES: Even after a convolutional encode and puncture, we can still look at the raw data
  //and more specifically, look at certain areas to determine if an encryption IV or META 
  //is present, or if things like the DST field might be 0xFFFFFFFFFFFF

  /* 
    //SRC and DST 0xFFFFFFFFFFFF
    M17 LSF Frame Sync (19:37:09): (approximate breaks in DST, SRC, and META)
    Input: DE4924924925B6DB6DB6DB 6924924924925B0636 32A8 0000000000000000000000000000000000000000006B1077
    DST: BROADCAST SRC:  UNKNOWN FFFFFFFFFFFF CAN: 7; Data Packet
    LSF: FF FF FF FF FF FF FF FF FF FF FF FF 03 82 00
          00 00 00 00 00 00 00 00 00 00 00 00 00 05 35
          (CRC CHK) E: 0535; C: 0535;

    //DST 0xFFFFFFFFFFFF
    M17 LSF Frame Sync (19:43:02): 
    Input: DE4924924925B6DB6D4C 00003B8796DE856E5836 32A8 00000000000000000000000000000000000000002A83696F
    DST: BROADCAST SRC: N0CALL    CAN: 7; Data Packet
    LSF: FF FF FF FF FF FF 00 00 4B 13 D1 06 03 82 00
          00 00 00 00 00 00 00 00 00 00 00 00 00 80 E9
          (CRC CHK) E: 80E9; C: 80E9;

    //DST 0xFFFFFFFFFFFF and IV present
        M17 LSF Frame Sync (19:46:43): 
    Input:DE4924924925B6DB6D4C 00003B8796DE856E5836 5AD7 86B20A8204AC57A6D34B0ECC8A448D11F6BC50C2 470EB80F
    LSF: FF FF FF FF FF FF 00 00 4B 13 D1 06 03 95 66
          46 9A F8 31 CA 39 C3 6E B9 37 6E D1 89 9B 0D
          (CRC CHK) E: 9B0D; C: 5108; CRC ERR


  */

  uint32_t broadcast = 0;
  broadcast = (uint32_t)convert_bits_into_output(&input[0], 32);

  //TODO: bit diff count comparison on the XOR of this field and 0xDE492492
  if (broadcast != 0xDE492492) broadcast = 0;

  uint32_t has_meta  = 0;
  has_meta = (uint32_t)convert_bits_into_output(&input[240], 32);

  //debug
  // fprintf (stderr, " BC: %08X; META: %08X; ", broadcast, has_meta);

  int i, j, k, x;

  //debug, inspect input vs known good LSF frame values
  // fprintf (stderr, "\n Input:");
  // for (i = 0; i < 368/8; i++)
  //   fprintf (stderr, "%02X", (uint8_t)convert_bits_into_output(&input[i*8], 8));

  //predictive depuncture by fabricating a convolved bit array (or just leaving it as zeroes)
  uint8_t fake_lsf[244]; //244 bits of a fabricated LSF frame
  uint8_t fake_con[488]; //488 bits of a fabricated LSF frame after convolutional encode

  memset (fake_lsf, 0, sizeof(fake_lsf));
  memset (fake_con, 0, sizeof(fake_con));

  //craft a partially filled fake_lsf array

  //DST == BROADCAST (48-bits)
  if (broadcast)
  {
    for (i = 0; i < 48; i++)
      fake_lsf[i] = 1;
  }

  //Meta or IV field present
  if (has_meta)
  {
    //if this is an IV, then we will mimic an IV by duplicating 
    //one by the same standard as in the M17 Specifications and
    //same code used by the m17_str_encoder

    time_t epoch = 1577836800L;                           //Jan 1, 2020, 00:00:00 UTC
    time_t ts = super->demod.current_time - epoch;        //timestamp since epoch
    srand((unsigned int)ts&0xFFFFFFFE); //randomizer seed based on timestamp

    //SID (run 2x rand to account for it)
    rand(); rand();

    //initialize a nonce
    uint8_t nonce[14]; memset (nonce, 0, sizeof(nonce));
    uint8_t   iv[112]; memset(iv, 0, sizeof(iv));

    //32-bit LSB of the timestamp
    nonce[0]  = (ts >> 24) & 0xFF;
    nonce[1]  = (ts >> 16) & 0xFF;
    nonce[2]  = (ts >> 8)  & 0xFF;
    nonce[3]  = (ts >> 0)  & 0xFF;

    //64-bit of rnd data
    nonce[4]  = rand() & 0xFF;
    nonce[5]  = rand() & 0xFF;
    nonce[6]  = rand() & 0xFF;
    nonce[7]  = rand() & 0xFF;
    nonce[8]  = rand() & 0xFF;
    nonce[9]  = rand() & 0xFF;
    nonce[10] = rand() & 0xFF;
    nonce[11] = rand() & 0xFF;
    //Spec updated and removes the CTR_HIGH value
    nonce[12] = rand() & 0xFF;
    nonce[13] = rand() & 0xFF;

    k = 0;
    for (j = 0; j < 14; j++)
    {
      for (i = 0; i < 8; i++)
        iv[k++] = (nonce[j] >> (7-i))&1;
    }

    for (i = 0; i < 112; i++)
      fake_lsf[i+112] = iv[i];

  }
  
  //Use the convolutional encoder to encode the fake LSF Frame
  simple_conv_encoder (fake_lsf, fake_con, 244);

  //reset counters
  j = 0; k = 0; x = 0;

  // P1 Depuncture
  for (i = 0; i < 488; i++)
  {

    //DST == Broadcast (predictive)
    if ( (i < 96) && broadcast)
    {
      if (p1[k++] == 1)
        output[x++] = input[j++];
      else output[x++] = fake_con[i];
    }

    //Meta / IV Field (predictive)
    else if ( (i > 224) && (i < 448) && has_meta) //224+112+112
    {
      if (p1[k++] == 1)
        output[x++] = input[j++];
      else output[x++] = fake_con[i];
    }

    //zero fill any other portions
    else
    {
      if (p1[k++] == 1) output[x++] = input[j++];
      else output[x++] = 0;
    }

    if (k == 61)
      k = 0; //reset 8 times

  }

}