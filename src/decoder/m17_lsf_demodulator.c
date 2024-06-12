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

  int i, x;
  uint8_t dbuf[384];           //384-bit frame - 16-bit (8 symbol) sync pattern (184 dibits)
  uint8_t m17_int_bits[368];  //368 bits that are still interleaved
  uint8_t m17_rnd_bits[368]; //368 bits that are still scrambled (randomized)
  uint8_t m17_bits[368];    //368 bits that have been de-interleaved and de-scramble
  uint8_t m17_depunc[500]; //488 bits after depuncturing

  memset (dbuf, 0, sizeof(dbuf));
  memset (m17_int_bits, 0, sizeof(m17_int_bits));
  memset (m17_rnd_bits, 0, sizeof(m17_rnd_bits));
  memset (m17_bits, 0, sizeof(m17_bits));
  memset (m17_depunc, 0, sizeof(m17_depunc));

  //if not running in debug / encoder mode, then perform dibit collection
  if (debug == 0)
  {
    //load dibits into dibit buffer
    for (i = 0; i < 184; i++)
      dbuf[i] = get_dibit(super);

    //convert dbuf into a bit array
    for (i = 0; i < 184; i++)
    {
      m17_rnd_bits[i*2+0] = (dbuf[i] >> 1) & 1;
      m17_rnd_bits[i*2+1] = (dbuf[i] >> 0) & 1;
    }
  }
  else //we are debugging, and copy input to m17_rnd_bits
    memcpy (m17_rnd_bits, input, 368);

  //descramble the frame
  for (i = 0; i < 368; i++)
    m17_int_bits[i] = (m17_rnd_bits[i] ^ m17_scramble[i]) & 1;

  //deinterleave the bit array using Quadratic Permutation Polynomial
  //function π(x) = (45x + 92x^2 ) mod 368
  for (i = 0; i < 368; i++)
  {
    x = ((45*i)+(92*i*i)) % 368;
    m17_bits[i] = m17_int_bits[x];
  }

  //p1 depuncture
  p1_predictive_depuncture (super, m17_bits, m17_depunc);

  //setup the convolutional decoder
  uint8_t temp[500];
  uint8_t s0;
  uint8_t s1;
  uint8_t m_data[32];
  uint8_t trellis_buf[260]; //30*8 = 240
  memset (trellis_buf, 0, sizeof(trellis_buf));
  memset (temp, 0, sizeof (temp));
  memset (m_data, 0, sizeof (m_data));
  uint16_t metric = 0; UNUSED(metric);

  for (i = 0; i < 488; i++)
    temp[i] = m17_depunc[i] << 1; 

  convolution_start();
  for (i = 0; i < 244; i++)
  {
    s0 = temp[(2*i)];
    s1 = temp[(2*i)+1];

    metric += convolution_decode(s0, s1);
  }

  convolution_chainback(m_data, 240);

  unpack_byte_array_into_bit_array(m_data, trellis_buf, 30);

  //test running other viterbi / trellis decoder
  // memset (trellis_buf, 0, sizeof(trellis_buf));
  // memset (m_data, 0, sizeof (m_data));
  // trellis_decode(trellis_buf, m17_depunc, 240);

  //debug view metric out of convolutional decoder
  // fprintf (stderr, " lsf metric: %05d; ", metric); //144 and 17616

  memset (super->m17d.lsf, 0, sizeof(super->m17d.lsf));
  memcpy (super->m17d.lsf, trellis_buf, 240);

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
    fprintf (stderr, "\n      (CRC CHK) E: %04X; C: %04X;", crc_ext, crc_cmp);
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
    time_t ts = super->demod.current_time - epoch - 1L;  //timestamp since epoch
    srand(ts); //randomizer seed based on timestamp

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

    //The last two octets are the CTR_HIGH value (upper 16 bits of the frame number),
    //but you would need to talk non-stop for over 20 minutes to roll it, so just using rnd
    //also, using zeroes seems like it may be a security issue, so using rnd as a base
    nonce[12] = rand() & 0xFF;
    nonce[13] = rand() & 0xFF;

    k = 0;
    for (j = 0; j < 14; j++)
    {
      for (i = 0; i < 8; i++)
        iv[k++] = (nonce[j] >> (7-i))&1;
    }

    for (i = 0; i < 112; i++) //may consider only loading the first 32-bit and not full value
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
    else if ( (i > 224) && (i < 456) && has_meta)
    {
      if (p1[k++] == 1)
        output[x++] = input[j++];
      else output[x++] = fake_con[i];
    }

    //observation of last bit (Old Method)
    else
    {
      if (p1[k++] == 1) output[x++] = input[j++];
      else if (output[x-2] == 1) output[x++] = 1;
      else output[x++] = 0;
    }

    if (k == 61)
      k = 0; //reset 8 times

  }

}