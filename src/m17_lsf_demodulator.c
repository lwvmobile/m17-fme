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
  uint16_t metric = 0; UNUSED(metric);

  //test viterbi with all zeroes and all ones
  // memset (m17_depunc, 0, sizeof(m17_depunc));
  // memset (m17_depunc, 1, sizeof(m17_depunc));
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