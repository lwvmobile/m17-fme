/*-------------------------------------------------------------------------------
 * m17_lsf_demodulator.c
 * M17 Project - Link Setup Frame Demodulation and Debug
 *
 * LWVMOBILE
 * 2025-01 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void demod_lsf(Super * super, uint8_t * input, int debug)
{
  //quell defined but not used warnings from m17.h
  stfu ();

  int i;
  uint8_t  dbuf[184];                 //384-bit frame - 16-bit (8 symbol) sync pattern (184 dibits)
  float    sbuf[184];                 //float symbol buffer
  uint16_t soft_bit[2*SYM_PER_PLD];   //raw frame soft bits
  uint16_t d_soft_bit[2*SYM_PER_PLD]; //deinterleaved soft bits
  uint8_t  viterbi_bytes[31];         //packed viterbi return bytes
  uint32_t error = 0;                 //viterbi error

  memset(dbuf, 0, sizeof(dbuf));
  memset(sbuf, 0.0f, sizeof(sbuf));
  memset(soft_bit, 0, sizeof(soft_bit));
  memset(d_soft_bit, 0, sizeof(d_soft_bit));
  memset(viterbi_bytes, 0, sizeof(viterbi_bytes));

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

  //libm17 magic
  //slice symbols to soft dibits
  slice_symbols(soft_bit, sbuf);

  //derandomize
  randomize_soft_bits(soft_bit);

  //deinterleave
  reorder_soft_bits(d_soft_bit, soft_bit);

  //viterbi
  error = viterbi_decode_punctured(viterbi_bytes, d_soft_bit, p1, 2*SYM_PER_PLD, 61);

  //unpack into the lsf bit array
  memset (super->m17d.lsf, 0, sizeof(super->m17d.lsf));
  unpack_byte_array_into_bit_array(viterbi_bytes+1, super->m17d.lsf, 30);

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
    fprintf (stderr, "\n      (CRC CHK) E: %04X; C: %04X; Ve: %1.1f;", crc_ext, crc_cmp, (float)error/(float)0xFFFF);
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