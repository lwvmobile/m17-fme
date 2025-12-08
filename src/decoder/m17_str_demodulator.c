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
  
  uint8_t  dbuf[184];                 //384-bit frame - 16-bit (8 symbol) sync pattern (184 dibits)
  float    sbuf[184];                 //float symbol buffer
  uint8_t m17_rnd_bits[368]; //368 bits that are still scrambled (randomized)
  uint8_t m17_int_bits[368]; //368 bits that are still interleaved
  uint8_t m17_bits[368]; //368 bits that have been de-interleaved and de-scramble
  uint8_t lich_bits[96];
  int lich_err = -1;

  memset (dbuf, 0, sizeof(dbuf));
  memset (sbuf, 0, sizeof(sbuf));
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
  else //we are debugging, and copy input to m17_rnd_bits, and convert input to symbols
  {
    memcpy (m17_rnd_bits, input, 368);

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
    prepare_str(super, sbuf);

  //get rid of this if it costs too much CPU / skips / lags
  super->demod.sync_time = super->demod.current_time = time(NULL);

  //refresh ncurses printer, if enabled
  #ifdef USE_CURSES
  if (super->opts.use_ncurses_terminal == 1)
    print_ncurses_terminal(super);
  #endif

}

void prepare_str(Super * super, float * sbuf)
{
  int i;
  uint16_t soft_bit[2*SYM_PER_PLD];   //raw frame soft bits
  uint16_t d_soft_bit[2*SYM_PER_PLD]; //deinterleaved soft bits
  uint8_t  viterbi_bytes[31];         //packed viterbi return bytes
  uint32_t error = 0;                 //viterbi error

  uint8_t stream_bits[144]; //128+16
  uint8_t payload[128];
  uint8_t end = 9;
  uint16_t fn = 0;

  memset(soft_bit, 0, sizeof(soft_bit));
  memset(d_soft_bit, 0, sizeof(d_soft_bit));
  memset(viterbi_bytes, 0, sizeof(viterbi_bytes));

  memset (stream_bits, 0, sizeof(stream_bits));
  memset (payload, 0, sizeof(payload));

  //libm17 magic
  //slice symbols to soft dibits
  slice_symbols(soft_bit, sbuf);

  //derandomize
  randomize_soft_bits(soft_bit);

  //deinterleave
  reorder_soft_bits(d_soft_bit, soft_bit);

  //viterbi
  error = viterbi_decode_punctured(viterbi_bytes, d_soft_bit+96, p2, 272, 12);

  //track viterbi error / cost metric
  super->error.viterbi_err = (float)error/(float)0xFFFF;

  //TODO: BER Estimate
  // state->error.ber_estimate;

  //load viterbi_bytes into bits for either data packets or voice packets
  unpack_byte_array_into_bit_array(viterbi_bytes+1, stream_bits, 18); //18*8 = 144

  end = stream_bits[0];
  fn = (uint16_t)convert_bits_into_output(&stream_bits[1], 15);

  //for scrambler seed calculation, if required (late entry)
  super->enc.scrambler_fn_d = fn;
  if (fn == 0)
    super->enc.scrambler_seed_d = super->enc.scrambler_key;

  //insert fn bits into aes_iv 14 and meta 15 for Initialization Vector
  super->m17d.lsf3.aes_iv[14] = (uint8_t)convert_bits_into_output(&stream_bits[1], 7);
  super->m17d.lsf3.aes_iv[15] = (uint8_t)convert_bits_into_output(&stream_bits[8], 8);

  if (super->opts.payload_verbosity >= 1)
  {
    fprintf (stderr, " FN: %04X", fn);
    //viterbi error
    fprintf (stderr, " Ve: %1.1f; ", (float)error/(float)0xFFFF);
  }

  if (end == 1)
    fprintf (stderr, " END;");

  for (i = 0; i < 128; i++)
    payload[i] = stream_bits[i+16];

  if ((super->m17d.skip_call == 0) && (super->m17d.dt == 2 || super->m17d.dt == 3))
    decode_str_payload(super, payload, super->m17d.dt, fn%6);
  else super->m17d.dt = 15;

  //reset skip call
  if (super->m17d.skip_call == 1 && end == 1)
    super->m17d.skip_call = 0;

  //failsafe to still get ECDSA digest if bad initial LSF (better than not attempting it)
  //note: dt of 15 will now be rejected by payload decoder after digest
  if (super->m17d.ecdsa.keys_loaded == 1 && super->m17d.dt == 15)
    decode_str_payload(super, payload, super->m17d.dt, fn%6);

  if (super->opts.payload_verbosity >= 1 && super->m17d.dt == 15)
  {
    fprintf (stderr, "\n STREAM: ");
    for (i = 0; i < 18; i++) 
      fprintf (stderr, "%02X", (uint8_t)convert_bits_into_output(&stream_bits[i*8], 8));
  }
}