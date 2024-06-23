/*-------------------------------------------------------------------------------
 * m17_str_demodulator.c
 * M17 Project - Stream Frame Demodulation and Debug
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void demod_str(Super * super, uint8_t * input, int debug)
{
  //quell defined but not used warnings from m17.h
  stfu ();

  int i, x;
  
  uint8_t dbuf[384]; //384-bit frame - 16-bit (8 symbol) sync pattern (184 dibits)
  uint8_t m17_rnd_bits[368]; //368 bits that are still scrambled (randomized)
  uint8_t m17_int_bits[368]; //368 bits that are still interleaved
  uint8_t m17_bits[368]; //368 bits that have been de-interleaved and de-scramble
  uint8_t lich_bits[96];
  int lich_err = -1;

  memset (dbuf, 0, sizeof(dbuf));
  memset (m17_rnd_bits, 0, sizeof(m17_rnd_bits));
  memset (m17_int_bits, 0, sizeof(m17_int_bits));
  memset (m17_bits, 0, sizeof(m17_bits));
  memset (lich_bits, 0, sizeof(lich_bits));

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

  for (i = 0; i < 96; i++)
    lich_bits[i] = m17_bits[i];

  //check lich first, and handle LSF chunk and completed LSF
  lich_err = decode_lich_contents(super, lich_bits);

  if (lich_err == 0)
    prepare_str(super, m17_bits);

  //get rid of this if it costs too much CPU / skips / lags
  super->demod.sync_time = super->demod.current_time = time(NULL);

  //refresh ncurses printer, if enabled
  #ifdef USE_CURSES
  if (super->opts.use_ncurses_terminal == 1)
    print_ncurses_terminal(super);
  #endif

}

void prepare_str(Super * super, uint8_t * input)
{
  int i, k, x; 
  uint8_t m17_punc[275]; //25 * 11 = 275
  memset (m17_punc, 0, sizeof(m17_punc));
  for (i = 0; i < 272; i++)
    m17_punc[i] = input[i+96];

  //depuncture the bits
  uint8_t m17_depunc[300]; //25 * 12 = 300
  memset (m17_depunc, 0, sizeof(m17_depunc));
  k = 0; x = 0;
  for (i = 0; i < 25; i++)
  {
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = m17_punc[x++];
    m17_depunc[k++] = 0; 
  }

  //setup the convolutional decoder
  uint8_t temp[300];
  uint8_t s0;
  uint8_t s1;
  uint8_t m_data[28];
  uint8_t trellis_buf[144];
  memset (trellis_buf, 0, sizeof(trellis_buf));
  memset (temp, 0, sizeof (temp));
  memset (m_data, 0, sizeof (m_data));
  uint16_t metric = 0; UNUSED(metric);

  //test viterbi with all zeroes and all ones
  // memset (m17_depunc, 0, sizeof(m17_depunc));
  // memset (m17_depunc, 1, sizeof(m17_depunc));

  for (i = 0; i < 296; i++)
    temp[i] = m17_depunc[i] << 1; 

  convolution_start();
  for (i = 0; i < 148; i++)
  {
    s0 = temp[(2*i)];
    s1 = temp[(2*i)+1];

    metric += convolution_decode(s0, s1);
  }

  convolution_chainback(m_data, 144);

  unpack_byte_array_into_bit_array(m_data, trellis_buf, 18);

  //test running other viterbi / trellis decoder
  // memset (trellis_buf, 0, sizeof(trellis_buf));
  // memset (m_data, 0, sizeof (m_data));
  // trellis_decode(trellis_buf, m17_depunc, 144);

  //load m_data into bits for either data packets or voice packets
  uint8_t payload[128];
  uint8_t end = 9;
  uint16_t fn = 0;
  memset (payload, 0, sizeof(payload));

  end = trellis_buf[0];
  fn = (uint16_t)convert_bits_into_output(&trellis_buf[1], 15);

  //for scrambler seed calculation, if required (late entry)
  super->enc.scrambler_fn_d = fn;
  if (fn == 0)
    super->enc.scrambler_seed_d = super->enc.scrambler_key;

  //insert fn bits into meta 14 and meta 15 for Initialization Vector
  super->m17d.meta[14] = (uint8_t)convert_bits_into_output(&trellis_buf[1], 7);
  super->m17d.meta[15] = (uint8_t)convert_bits_into_output(&trellis_buf[8], 8);

  if (super->opts.payload_verbosity >= 1)
    fprintf (stderr, " FSN: %05d", fn);

  if (end == 1)
    fprintf (stderr, " END;");

  //place code here, but may be better to leave it disabled if 
  //bit errors occur (no parity or checksum on these bits)
  // if (end == 1)
  // {
  //   //close per call wav file, if opened
  //   if (super->wav.wav_out_pc)
  //     close_wav_out_pc (super);
  // }

  //debug view metric out of convolutional decoder
  // fprintf (stderr, " str metric: %05d; ", metric); //88 and 6422

  for (i = 0; i < 128; i++)
    payload[i] = trellis_buf[i+16];

  if (super->m17d.dt == 2 || super->m17d.dt == 3)
    decode_str_payload(super, payload, super->m17d.dt, fn%6);
  else if (super->m17d.dt == 1)  fprintf (stderr, " DATA;");
  else if (super->m17d.dt == 0)  fprintf (stderr, "  RES;");
  else if (super->m17d.dt == 15) fprintf (stderr, " UNSET");
  // else                             fprintf (stderr, "  UNK;");

  //failsafe to still get ECDSA digest if bad initial LSF (better than not attempting it)
  //note: dt of 15 will now be rejected by payload decoder after digest
  if (super->m17d.ecdsa.keys_loaded == 1 && super->m17d.dt == 15)
    decode_str_payload(super, payload, super->m17d.dt, fn%6);

    

  if (super->opts.payload_verbosity >= 1 && super->m17d.dt < 2)
  {
    fprintf (stderr, "\n STREAM:");
    for (i = 0; i < 18; i++) 
      fprintf (stderr, " %02X", (uint8_t)convert_bits_into_output(&trellis_buf[i*8], 8));
  }
}