/*-------------------------------------------------------------------------------
 * framesync.c
 * Project M17 - Frame Sync and Float Symbol / uint8_t Dibit Functions
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void fsk4_framesync (Super * super)
{

  //TODO: Working, but debug why this doesn't always work when started, may be a jitter or timing issue

  //quell defined but not used warnings from m17.h
  UNUSED(b40); UNUSED(m17_scramble); UNUSED(p1); UNUSED(p3); UNUSED(symbol_map); UNUSED(m17_rrc);
  UNUSED(lsf_sync_symbols); UNUSED(str_sync_symbols); UNUSED(pkt_sync_symbols); UNUSED(symbol_levels);

  //safety init on the ptrs
  super->demod.sample_buffer_ptr = 0;
  super->demod.symbol_buffer_ptr = 0;
  super->demod.dibit_buffer_ptr  = 0;
  super->demod.float_symbol_buffer_ptr = 0;

  super->demod.fsk4_samples_per_symbol = 10; //set to input rate / system's bps rate (48000/4800 on M17 is 10)
  super->demod.fsk4_symbol_center = 4;

  //this is the buffer to check for frame sync using the euclidean distance mumbo jumbo
  float last_symbols[8]; memset (last_symbols, 0.0f, sizeof(last_symbols));

  //look for frame synchronization
  while (!exitflag)
  {

    float dist = 99.0f; //euclidean distance
    float float_symbol = demodulate_and_return_float_symbol(super);
    convert_float_symbol_to_dibit_and_store(super, float_symbol);

    //push the last symbol and look for a frame sync pattern
    dist = push_and_dist (last_symbols, float_symbol);
    if (dist < 2.0f) //DIST_THRESH
    {
      //TODO: Make nicer frame sync print with timestamp and sthit
      fprintf (stderr, "\n M17 Stream Frame Sync: ");
      //Launch into decoder with appropriate type and setup the deocder for get_dibit function
      demod_str(super, NULL, 0);
    }

  }
}

float eucl_norm(float * in1, int8_t * in2, uint8_t n)
{
  float tmp = 0.0f;
  for(uint8_t i = 0; i < n; i++)
    tmp += (in1[i]-(float)in2[i])*(in1[i]-(float)in2[i]);
  return sqrtf(tmp);
}

float push_and_dist (float * last, float symbol)
{
  float dist = 0.0f;
  for(uint8_t i = 0; i < 7; i++)
    last[i]=last[i+1];
  last[7]=symbol;
  dist = eucl_norm(last, str_sync_symbols, 8);
  return dist;
}

uint8_t digitize_symbol_to_dibit (float symbol)
{
  uint8_t dibit = 0;
  //positive polarity TODO: Do inverted as well based on user value
  if (symbol == +3.0) dibit = 1;
  if (symbol == +1.0) dibit = 0;
  if (symbol == -1.0) dibit = 2;
  if (symbol == -3.0) dibit = 3;
  return dibit;
}

float demodulate_and_return_float_symbol(Super * super)
{
  int i;
  short sample = 0;
  short center_sample = 0;
  // float float_sum = 0.0f;
  float float_symbol = 0.0f;

  for (i = 0; i < super->demod.fsk4_samples_per_symbol; i++)
  {
    sample = get_short_audio_input_sample(super);
    // float_sum += (float)sample;
    //store sample that is at the approximate center value
    if (i == super->demod.fsk4_symbol_center)
      center_sample = sample;
  }

  //calculate max and min based on lastest values of the float buffer (WIP)
  // float buffer_max, buffer_min, buffer_value = 0.0f;
  // for (i = 0; i < 192; i++)
  // {
  //   buffer_value = super->demod.sample_buffer[(super->demod.sample_buffer_ptr-i)%65535];
  //   if      (buffer_value > buffer_max) buffer_max = buffer_value;
  //   else if (buffer_value < buffer_min) buffer_min = buffer_value;
  // }

  // super->demod.fsk4_max = buffer_max;
  // super->demod.fsk4_min = buffer_min;
  //end max and min float buffer calculation

  //simple straight-forward approach
  if      (center_sample > super->demod.fsk4_max) super->demod.fsk4_max = center_sample;
  else if (center_sample < super->demod.fsk4_min) super->demod.fsk4_min = center_sample;

  //calculate lower middle and upper middle values based on the min and max TODO: go by actual deviation?
  super->demod.fsk4_lmid = super->demod.fsk4_min / 1.73f; //root 3, was 2.0f;
  super->demod.fsk4_umid = super->demod.fsk4_max / 1.73f; //root 3, was 2.0f;

  if (center_sample < super->demod.fsk4_lmid)
    float_symbol = -3.0f;
  else if (center_sample > super->demod.fsk4_lmid && center_sample < super->demod.fsk4_center)
    float_symbol = -1.0f;
  else if (center_sample > super->demod.fsk4_center && center_sample < super->demod.fsk4_umid)
    float_symbol = +1.0f;
  else if (center_sample > super->demod.fsk4_umid)
    float_symbol = +3.0f;

  //store float symbol
  super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr++%65535)] = float_symbol;

  //store sample used
  super->demod.sample_buffer[(super->demod.sample_buffer_ptr++%65535)] = center_sample;

  //return dibit value
  return float_symbol;
}

uint8_t convert_float_symbol_to_dibit_and_store(Super * super, float float_symbol)
{
  uint8_t dibit = 0;

  //digitize the float symbol into a dibit
  dibit = digitize_symbol_to_dibit(float_symbol);

  //store dibit
  super->demod.dibit_buffer[(super->demod.dibit_buffer_ptr++%65535)] = dibit;

  return dibit;
}

//convenience wrapper function for easy transition
uint8_t get_dibit (Super * super)
{
  uint8_t dibit = 0;
  float float_symbol = 0.0f;
  float_symbol = demodulate_and_return_float_symbol(super);
  dibit = convert_float_symbol_to_dibit_and_store(super, float_symbol);
  return dibit;
}

