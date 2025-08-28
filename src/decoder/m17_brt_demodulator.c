/*-------------------------------------------------------------------------------
 * m17_brt_demodulator.c
 * M17 Project - BERT (Bit Error Rate Test) Frame Demodulation and Debug
 *
 * LWVMOBILE
 * 2025-08 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

static uint8_t bert_lfsr_bit_array[197];
static uint16_t bert_lfsr_seed;
static uint16_t sync_error_count;

uint16_t brt_lfsr (uint16_t seed, uint8_t * output, uint8_t len)
{
  for (int i = 0; i < len; i++)
  {
    uint8_t bit = ((seed >> 8) ^ (seed >> 4)) & 1;
    seed <<= 1;
    seed |= bit;
    output[i] = bit;
  }
  seed &= 0x1FF;
  return seed;
}

void init_brt(void)
{
  memset (bert_lfsr_bit_array, 0, sizeof(bert_lfsr_bit_array));
  bert_lfsr_seed = brt_lfsr(1, bert_lfsr_bit_array, 197);
  sync_error_count = 0;
}

uint32_t prepare_brt(float * sbuf, uint8_t * output_bits)
{

  uint16_t soft_bit[2*SYM_PER_PLD];   //raw frame soft bits
  uint16_t d_soft_bit[2*SYM_PER_PLD]; //deinterleaved soft bits
  uint8_t  viterbi_bytes[31];         //packed viterbi return bytes
  uint32_t error = 0;                 //viterbi error

  memset(soft_bit, 0, sizeof(soft_bit));
  memset(d_soft_bit, 0, sizeof(d_soft_bit));
  memset(viterbi_bytes, 0, sizeof(viterbi_bytes));

  //libm17 magic
  //slice symbols to soft dibits
  slice_symbols(soft_bit, sbuf);

  //derandomize
  randomize_soft_bits(soft_bit);

  //deinterleave
  reorder_soft_bits(d_soft_bit, soft_bit);

  //viterbi
  error = viterbi_decode_punctured(viterbi_bytes, d_soft_bit, p2, 368, 12);

  //debug
  // fprintf (stderr, "\n VB: ");
  // for (int i = 1; i < 26; i++)
  //   fprintf (stderr, "%02X ", viterbi_bytes[i]);

  //load viterbi_bytes into bits for either data packets or voice packets
  unpack_byte_array_into_bit_array(viterbi_bytes, output_bits, 26);

  return error;

}

void demod_brt(Super * super, uint8_t * input, int debug)
{
  //quell defined but not used warnings from m17.h
  stfu ();

  int i;
  
  uint8_t  dbuf[184];                 //384-bit frame - 16-bit (8 symbol) sync pattern (184 dibits)
  float    sbuf[184];                 //float symbol buffer

  memset (dbuf, 0, sizeof(dbuf));
  memset (sbuf, 0, sizeof(sbuf));

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
  else //we are debugging, and copy input to m17_rnd_bits, and convert input to symbols
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

  uint8_t bert_bits[216]; //little extra, needed for the offset from viterbi
  memset (bert_bits, 0, sizeof(bert_bits));

  uint32_t ve = prepare_brt(sbuf, bert_bits);

  uint8_t sync_error_bit_count = 0;
  uint8_t full_bit_error_count = 0;

  //first 18 for sync period test (twice len of PRBS9)
  for (i = 0; i < 18; i++)
  {
    if (bert_bits[i+7] != bert_lfsr_bit_array[i]) //+7 offset
      sync_error_bit_count++;
  }

  //do full bit rx error as well
  for (i = 0; i < 197; i++)
  {
    if (bert_bits[i+7] != bert_lfsr_bit_array[i]) //+7 offset
      full_bit_error_count++;
  }

  fprintf (stderr, "\n Sync Bit Error: %02d / 18; Full Bit Error: %03d / 197; Ve: %1.1f; LFSR: %03X;", sync_error_bit_count, full_bit_error_count, (float)ve/(float)0xFFFF , bert_lfsr_seed);

  if (sync_error_bit_count != 0)
  {
    memset (bert_lfsr_bit_array, 0, sizeof(bert_lfsr_bit_array));
    bert_lfsr_seed = brt_lfsr(1, bert_lfsr_bit_array, 197);
    sync_error_count++;
    fprintf (stderr, " Sync Error; %02d / 512;", sync_error_count);

    if (sync_error_count > 512) //9-bit LFSR should reset after 512 attempts, if it doesn't, then heavy reception error
      fprintf (stderr, " Sync Failure!");
  }
  else
  {
    bert_lfsr_seed = brt_lfsr(bert_lfsr_seed, bert_lfsr_bit_array, 197); //unsure if this advances by only 1, or by next 197
    sync_error_count = 0;
    fprintf (stderr, " Sync Okay;");
  }

  if (super->opts.payload_verbosity > 0)
  {
    fprintf (stderr, "\n LFSR Bits: ");
    for (i = 0; i < 197; i++)
      fprintf (stderr, "%d", bert_lfsr_bit_array[i]);

    fprintf (stderr, "\n   RX Bits: ");
    for (i = 0; i < 197; i++)
      fprintf (stderr, "%d", bert_bits[i]);
  }

  super->demod.sync_time = super->demod.current_time = time(NULL);

}
