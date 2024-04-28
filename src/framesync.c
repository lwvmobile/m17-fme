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
  //sync type
  int type = -1;

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

  //this is the buffer to check for frame sync using the euclidean distance norm
  float last_symbols[8]; memset (last_symbols, 0.0f, sizeof(last_symbols));

  //look for frame synchronization
  while (!exitflag)
  {

    float float_symbol = demodulate_and_return_float_symbol(super);
    convert_float_symbol_to_dibit_and_store(super, float_symbol);

    //push the float symbol to the buffer and check for sync patterns
    push_float_buffer(last_symbols, float_symbol);
    type = dist_and_sync(last_symbols);

    if (super->opts.payload_verbosity)
      print_debug_information(super);

    //update sync time
    if (type != -1)
      super->demod.sync_time = time(NULL);

    //print framesync pattern
    if (type != -1)
      print_frame_sync_pattern(type);

    //execute on sync types
    if (type == 1)
      demod_lsf(super, NULL, 0);

    else if (type == 2)
      demod_str(super, NULL, 0);

    else if (type == 3)
      demod_pkt(super, NULL, 0);

    // else if (type == 4)
    //   demod_brt(super, NULL, 0);

    //reset type
    type = -1;

  }
}

//push samples through float buffer that is used to test for sync patterns,
void push_float_buffer (float * last, float symbol)
{
  for(uint8_t i = 0; i < 7; i++)
    last[i]=last[i+1];
  last[7]=symbol;
}

//test Euclidean Distance on Multiple Frame Sync Types,
//Return Value is Frame Sync Type (LSF, STR, PKT, BRT)
int dist_and_sync(float * last)
{

  float dist = 99.0f; //euclidean distance

  //LSF
  dist = eucl_norm(last, lsf_sync_symbols, 8);
  if (dist < 2.0f) return 1;
  dist = 99.0f; //reset

  //STR
  dist = eucl_norm(last, str_sync_symbols, 8);
  if (dist < 2.0f) return 2;
  dist = 99.0f; //reset

  //PKT
  dist = eucl_norm(last, pkt_sync_symbols, 8);
  if (dist < 2.0f) return 3;
  dist = 99.0f; //reset

  //BRT
  // dist = eucl_norm(last, brt_sync_symbols, 8); //need to add syncword for brt
  // if (dist < 2.0f) return 4;
  // dist = 99.0f; //reset

  return -1;
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
  float float_symbol = 0.0f;

  //gather number of samples_per_symbol, and then store the center_sample
  for (i = 0; i < super->demod.fsk4_samples_per_symbol; i++)
  {
    sample = get_short_audio_input_sample(super);
    if (i == super->demod.fsk4_symbol_center)
      center_sample = sample;
  }

  // simple_refresh_min_max_center(super, center_sample);
  complex_refresh_min_max_center (super);
  float_symbol = float_symbol_slicer(super, sample);

  //store float symbol
  super->demod.float_symbol_buffer[(super->demod.float_symbol_buffer_ptr++%65535)] = float_symbol;

  //store sample used
  super->demod.sample_buffer[(super->demod.sample_buffer_ptr++%65535)] = center_sample;

  //return dibit value
  return float_symbol;
}

float float_symbol_slicer(Super * super, short sample)
{
  float float_symbol = 0.0f;

  if (sample < super->demod.fsk4_lmid)
    float_symbol = -3.0f;
  else if (sample > super->demod.fsk4_lmid   && sample < super->demod.fsk4_center)
    float_symbol = -1.0f;
  else if (sample > super->demod.fsk4_center && sample < super->demod.fsk4_umid)
    float_symbol = +1.0f;
  else if (sample > super->demod.fsk4_umid)
    float_symbol = +3.0f;

  return float_symbol;
}

void complex_refresh_min_max_center (Super * super)
{

  //TODO: Only make buffers as large as they need to be, tweak for loop len?

  int i = 0;

  //calculate center, max, and min based on lastest values of the sample buffer (WIP)
  float buffer_max, buffer_min, buffer_value = 0.0f;
  for (i = 0; i < 192*10; i++) //tweak this value? or something else?
  {
    buffer_value = super->demod.sample_buffer[(super->demod.sample_buffer_ptr-i)%65535];
    if      (buffer_value > buffer_max) buffer_max = buffer_value;
    else if (buffer_value < buffer_min) buffer_min = buffer_value;
  }

  super->demod.fsk4_max = buffer_max;
  super->demod.fsk4_min = buffer_min;

  super->demod.fsk4_lmid = buffer_min / 2.0f;
  super->demod.fsk4_umid = buffer_max / 2.0f;
  super->demod.fsk4_center = (fabs(buffer_max) - fabs(buffer_min)) / 2.0f;
  //end max and min float buffer calculation
}

void simple_refresh_min_max_center (Super * super, float sample)
{

  //NOTE: This will only work if signal levels are consistent, which is okay
  //for testing, but in real world application, this will easily fail
  
  //simple straight-forward approach
  if      (sample > super->demod.fsk4_max) super->demod.fsk4_max = sample;
  else if (sample < super->demod.fsk4_min) super->demod.fsk4_min = sample;

  //TODO: go by actual deviation for system type, but allow 'wiggle' room
  //calculate center, lower middle and upper middle values based on the min and max
  super->demod.fsk4_lmid = super->demod.fsk4_min / 2.0f;
  super->demod.fsk4_umid = super->demod.fsk4_max / 2.0f;

  //disable this if issues arise, not sure if this is okay or not (I think this is okay)
  super->demod.fsk4_center = (fabs(super->demod.fsk4_max) - fabs(super->demod.fsk4_min)) / 2.0f;
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

//convenience wrapper function for easy transition from old code to new code
uint8_t get_dibit (Super * super)
{
  uint8_t dibit = 0;
  float float_symbol = 0.0f;
  float_symbol = demodulate_and_return_float_symbol(super);
  dibit = convert_float_symbol_to_dibit_and_store(super, float_symbol);
  return dibit;
}


//organize some more later, and put credits for things of Woj and L and whoever else
//based off of lib17 math https://github.com/M17-Project/libm17
float eucl_norm(float * in1, int8_t * in2, uint8_t n)
{
  float tmp = 0.0f;
  for(uint8_t i = 0; i < n; i++)
    tmp += (in1[i]-(float)in2[i])*(in1[i]-(float)in2[i]);
  return sqrtf(tmp);
}

char * get_sync_type_string(int type)
{
  if (type == 1)
    return "LSF";
  else if (type == 2)
    return "STR";
  else if (type == 3)
    return "PKT";
  else if (type == 4)
    return "BRT";
  else
    return "";
}

//high level debug infomation dumps
void print_debug_information(Super * super)
{
  if (super->opts.payload_verbosity > 3)
  {
    fprintf (stderr, "\n MIN: %f; MAX: %f; LMID: %f; UMID: %f; Center: %f; ", 
      super->demod.fsk4_min, super->demod.fsk4_max, super->demod.fsk4_lmid, 
      super->demod.fsk4_umid, super->demod.fsk4_center);
  }

}

//TODO: Fix datestr bs, or cleanup later
void print_frame_sync_pattern(int type)
{
  // char * datestr = getDateH(); //THIS causes issues, like an overflow, or slows down the function too much
  char * timestr = getTimeC();
  char * syncstr = get_sync_type_string(type);

  fprintf (stderr, "\n");
  // fprintf (stderr, "(%s.%s) ", datestr, timestr);
  // fprintf (stderr, "%s. ", datestr);
  // fprintf (stderr, "%s ", timestr);

  // fprintf (stderr, "M17 %s Frame Sync: ", syncstr);
  fprintf (stderr, "M17 %s Frame Sync (%s): ", syncstr, timestr);

  //free allocated memory and NULL the PTR
  // if (datestr != NULL)
  // {
  //   free (datestr);
  //   datestr = NULL;
  // }

  if (timestr != NULL)
  {
    free (timestr);
    timestr = NULL;
  }

}

