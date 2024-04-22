/*-------------------------------------------------------------------------------
 * m17_lsf_demodulator.c
 * Project M17 - Link Setup Frame Demodulation and Debug
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

 #include "main.h"
// #include "m17.h"

void demod_lsf(m17_decoder_state * m17d, uint8_t * input, int debug)
{

  //NOTE: Same as demod_lsf, sans dibit collection
  uint8_t m17_scramble[369] = { 
  1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1,
  1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0,
  1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0,
  1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0,
  1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1,
  0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0,
  0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1,
  1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1,
  1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1,
  0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0,
  0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0,
  1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0,
  0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1,
  1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1,
  1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1,
  0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0,
  0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1 
  };

  uint8_t p1[62] = {
  1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1,
  1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1,
  1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1,
  1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1
  };

  int i, j, k, x;

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
    // for (i = 0; i < 184; i++)
    //   dbuf[i] = (uint8_t) getDibit(opts, state);

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

  j = 0; k = 0; x = 0;

  // P1 Depuncture
  for (i = 0; i < 488; i++)
  {
    //assign any puncture as a 0
    // if (p1[k++] == 1) m17_depunc[x++] = m17_bits[j++];
    // else m17_depunc[x++] = 0;


    //seems to be better if we use the last bit as an educated guess on what the next bit should be
    //this pseudo logic is based purely on 0xFFFFFFFFFF as Broadcast, and all zeroes as the Meta(IV)

    //DST, or META field
    if (i < 48 || i > 96)
    {
      if (p1[k++] == 1) m17_depunc[x++] = m17_bits[j++];
      else if (m17_depunc[x-2] == 1) m17_depunc[x++] = 1;
      else m17_depunc[x++] = 0;
    }
    else //any other field
    {
      if (p1[k++] == 1) m17_depunc[x++] = m17_bits[j++];
      else m17_depunc[x++] = 0;
    }

    

    if (k == 61) k = 0; //61 -- should reset 8 times againt the array

  }

  //setup the convolutional decoder
  uint8_t temp[500];
  uint8_t s0;
  uint8_t s1;
  uint8_t m_data[32];
  uint8_t trellis_buf[260]; //30*8 = 240
  memset (trellis_buf, 0, sizeof(trellis_buf));
  memset (temp, 0, sizeof (temp));
  memset (m_data, 0, sizeof (m_data));

  for (i = 0; i < 488; i++)
    temp[i] = m17_depunc[i] << 1; 

  convolution_start();
  for (i = 0; i < 244; i++)
  {
    s0 = temp[(2*i)];
    s1 = temp[(2*i)+1];

    convolution_decode(s0, s1);
  }

  convolution_chainback(m_data, 240);

  //244/8 = 30, last 4 (244-248) are trailing zeroes
  for(i = 0; i < 30; i++)
  {
    trellis_buf[(i*8)+0] = (m_data[i] >> 7) & 1;
    trellis_buf[(i*8)+1] = (m_data[i] >> 6) & 1;
    trellis_buf[(i*8)+2] = (m_data[i] >> 5) & 1;
    trellis_buf[(i*8)+3] = (m_data[i] >> 4) & 1;
    trellis_buf[(i*8)+4] = (m_data[i] >> 3) & 1;
    trellis_buf[(i*8)+5] = (m_data[i] >> 2) & 1;
    trellis_buf[(i*8)+6] = (m_data[i] >> 1) & 1;
    trellis_buf[(i*8)+7] = (m_data[i] >> 0) & 1;
  }

  memset (m17d->lsf, 0, sizeof(m17d->lsf));
  memcpy (m17d->lsf, trellis_buf, 240);

  uint8_t lsf_packed[30];
  memset (lsf_packed, 0, sizeof(lsf_packed));

  //need to pack bytes for the sw5wwp variant of the crc (might as well, may be useful in the future)
  for (i = 0; i < 30; i++)
    lsf_packed[i] = (uint8_t)ConvertBitIntoBytes(&m17d->lsf[i*8], 8);

  uint16_t crc_cmp = crc16(lsf_packed, 28);
  uint16_t crc_ext = (uint16_t)ConvertBitIntoBytes(&m17d->lsf[224], 16);
  int crc_err = 0;

  if (crc_cmp != crc_ext) crc_err = 1;

  if (crc_err == 0)
    decode_lsf_contents(m17d);
  // else if (opts->skip_crc_fail == 0)
  //   M17decodeLSF(state);

  // if (opts->payload == 1)
  {
    fprintf (stderr, "\n LSF: ");
    for (i = 0; i < 30; i++)
    {
      if (i == 15) fprintf (stderr, "\n      ");
      fprintf (stderr, "[%02X]", lsf_packed[i]);
    }
    fprintf (stderr, " (CRC CHK) E: %04X; C: %04X;", crc_ext, crc_cmp);
  }

  if (crc_err == 1) fprintf (stderr, " CRC ERR");

}