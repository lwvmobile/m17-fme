/*-------------------------------------------------------------------------------
 * m17_pkt_demodulator.c
 * Project M17 - Packet Frame Demodulation and Debug
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void demod_pkt(Super * super, uint8_t * input, int debug)
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

  //P3 Depuncture
  x = 0;
  for (i = 0; i < 420; i++)
  {
    if (p3[i%8] == 1)
      m17_depunc[i] = m17_bits[x++];
    else m17_depunc[i] = 0;
  }

  //setup the convolutional decoder
  uint8_t temp[500];
  uint8_t s0;
  uint8_t s1;
  uint8_t m_data[27];
  uint8_t trellis_buf[260]; //30*8 = 240
  memset (trellis_buf, 0, sizeof(trellis_buf));
  memset (temp, 0, sizeof (temp));
  memset (m_data, 0, sizeof (m_data));

  for (i = 0; i < 420; i++) //double check and test value here
    temp[i] = m17_depunc[i] << 1; 

  convolution_start();
  for (i = 0; i < 210; i++) //double check and test value here
  {
    s0 = temp[(2*i)];
    s1 = temp[(2*i)+1];

    convolution_decode(s0, s1);
  }

  convolution_chainback(m_data, 206); //double check and test value here

  //244/8 = 30, last 4 (244-248) are trailing zeroes
  for(i = 0; i < 26; i++)
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

  uint8_t pkt_packed[26];
  memset (pkt_packed, 0, sizeof(pkt_packed));

  //pack to local
  memcpy (pkt_packed, m_data, 26);

  //local variables
  uint8_t counter = (pkt_packed[25] >> 2) & 0x1F;
  uint8_t eot = (pkt_packed[25] >> 7) & 1;

  int ptr = super->m17d.pbc_ptr*25;
  int total = ptr + counter - 1;
  int end = ptr + 25;

  //debug counter and eot value
  if (!eot) fprintf (stderr, " CNT: %02d; PBC: %02d; EOT: %d;", super->m17d.pbc_ptr, counter, eot);
  else fprintf (stderr, " CNT: %02d; LST: %02d; EOT: %d;", super->m17d.pbc_ptr, counter, eot);

  //put packet into storage
  memcpy (super->m17d.pkt+ptr, pkt_packed, 25);

  //individual frame packet
  if (super->opts.payload_verbosity >= 1)
  {
    fprintf (stderr, "\n pkt: ");
    for (i = 0; i < 26; i++)
      fprintf (stderr, "%02X", pkt_packed[i]);
  }

  //evaluate completed packet if eot bit is signalled in current packet
  if (eot)
  {
    //do a CRC check
    uint16_t crc_cmp = crc16(super->m17d.pkt, total+1);
    uint16_t crc_ext = (super->m17d.pkt[total+1] << 8) + super->m17d.pkt[total+2];

    // optimal location?
    if (crc_cmp != crc_ext)
      fprintf (stderr, " (CRC ERR) ");

    //error tracking
    if (crc_cmp != crc_ext) super->error.pkt_crc_err++;

    //decode completed packet
    if (crc_cmp == crc_ext)
      decode_pkt_contents(super, super->m17d.pkt, total);
    else if (super->opts.allow_crc_failure == 1)
      decode_pkt_contents(super, super->m17d.pkt, total);

    // if (crc_cmp != crc_ext)
    //   fprintf (stderr, " (CRC ERR) ");

    if (super->opts.payload_verbosity == 1)
    {
      fprintf (stderr, "\n PKT:");
      for (i = 0; i < end; i++)
      {
        if ( (i%25) == 0 && i != 0)
          fprintf (stderr, "\n     ");
        fprintf (stderr, " %02X", super->m17d.pkt[i]);
      }
      fprintf (stderr, "\n      (CRC CHK) E: %04X; C: %04X;", crc_ext, crc_cmp);
    }

    //reset after processing
    memset (super->m17d.pkt, 0, sizeof(super->m17d.pkt));
    super->m17d.pbc_ptr = 0;
    // super->m17d.dt = 15; //reset here, on OTAKD frames, if the subsequent LSF has an error, then it leaves this as DATA
  }

  //increment pbc counter last
  if (!eot) super->m17d.pbc_ptr++;

  //get rid of this if it costs too much CPU / skips / lags
  super->demod.sync_time = super->demod.current_time = time(NULL);

  //refresh ncurses printer, if enabled
  #ifdef USE_CURSES
  if (super->opts.use_ncurses_terminal == 1)
    print_ncurses_terminal(super);
  #endif

}