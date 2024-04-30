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

  //the last_symbol buffer is now super->demod.sync_symbols so sync
  //can be obtained when exiting and re-entering this function if needed

  //look for frame synchronization
  for (int i = 0; i < 192*10; i++)
  {

    //check exitflag
    if (exitflag == 1) break;

    //return a sample, symbol and a dibit, place them into arrays for buffer storage
    float float_symbol = demodulate_and_return_float_symbol(super);
    convert_float_symbol_to_dibit_and_store(super, float_symbol);

    //push the float symbol to the buffer and check for sync patterns
    push_float_buffer(super->demod.sync_symbols, float_symbol);
    type = dist_and_sync(super->demod.sync_symbols);

    //print sync information and update
    if (type != -1)
    {
      print_frame_sync_pattern(super, type);
      super->demod.in_sync = 1;
    } 

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

//Euclidean Norm Distance on Multiple Frame Sync Types,
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
  float float_symbol = 0.0f;

  //If reading from float symbol in file
  if (super->opts.float_symbol_in != NULL)
  {
    //read in one float symbol
    fread (&float_symbol, sizeof(float), 1, super->opts.float_symbol_in); //sizeof(float) is 4 (usually)
    if ( feof(super->opts.float_symbol_in) ) exitflag = 1; //end of file, exit
  }

  //DSD-FME based dibit input file (capture.bin)
  else if (super->opts.dibit_in != NULL)
  {
    //read one dibit
    uint8_t dibit = fgetc (super->opts.dibit_in);

    //convert dibit to float (I know its redundant...but easier to shoe it in here)
    if      (dibit == 0) float_symbol = +1.0f;
    else if (dibit == 1) float_symbol = +3.0f;
    else if (dibit == 2) float_symbol = -1.0f;
    else if (dibit == 3) float_symbol = -3.0f;
    else                 float_symbol = +0.0f;

    if ( feof(super->opts.dibit_in) ) exitflag = 1; //end of file, exit
  }

  //If using RF Audio Based Input (Pulse, OSS, SNDFile or other)
  else
  {
    //gather number of samples_per_symbol, and then store the nth sample
    for (i = 0; i < super->demod.fsk4_samples_per_symbol; i++)
    {

      sample = get_short_audio_input_sample(super);

      //evaluate jitter here
      //TODO: Make a symbol_timing_recovery function

      //collect post sample for evaluation for timing recovery purposes
      super->demod.last_sample = sample;

    }

    buffer_refresh_min_max_center(super); //buffer based on last 192 symbols
    // simple_refresh_min_max_center(super, sample); //based on current only
    float_symbol = float_symbol_slicer(super, sample);
  }

  //store float symbol
  super->demod.float_symbol_buffer[super->demod.float_symbol_buffer_ptr++] = float_symbol;

  //store sample used
  super->demod.sample_buffer[super->demod.sample_buffer_ptr++] = sample;

  //ptr safety truncate (shouldn't be needed due to cast type as uint16_t 
  //into an array larger, but nevertheless, good practice on array indexing)
  super->demod.float_symbol_buffer_ptr &= 0xFFFF;
  super->demod.sample_buffer_ptr       &= 0xFFFF;
  if (super->demod.float_symbol_buffer_ptr == 0) super->demod.float_symbol_buffer_ptr = 192;
  if (super->demod.sample_buffer_ptr == 0) super->demod.sample_buffer_ptr = 192;

  //debug
  // fprintf (stderr, "\n FSPTR: %05d; FS: %1.0f; SAPTR: %05d; SAMP: %06d;", super->demod.float_symbol_buffer_ptr, super->demod.float_symbol_buffer[super->demod.float_symbol_buffer_ptr-1], super->demod.sample_buffer_ptr, super->demod.sample_buffer[super->demod.sample_buffer_ptr-1]);

  //return dibit value
  return float_symbol;
}

float float_symbol_slicer(Super * super, short sample)
{
  float float_symbol = 0.0f;
  if (super->opts.inverted_signal == 0)
  {
    if (sample < super->demod.fsk4_lmid)
      float_symbol = -3.0f;
    else if (sample > super->demod.fsk4_lmid   && sample < super->demod.fsk4_center)
      float_symbol = -1.0f;
    else if (sample > super->demod.fsk4_center && sample < super->demod.fsk4_umid)
      float_symbol = +1.0f;
    else if (sample > super->demod.fsk4_umid)
      float_symbol = +3.0f;
  }
  else if (super->opts.inverted_signal == 1)
  {
    if (sample < super->demod.fsk4_lmid)
      float_symbol = +3.0f;
    else if (sample > super->demod.fsk4_lmid   && sample < super->demod.fsk4_center)
      float_symbol = +1.0f;
    else if (sample > super->demod.fsk4_center && sample < super->demod.fsk4_umid)
      float_symbol = -1.0f;
    else if (sample > super->demod.fsk4_umid)
      float_symbol = -3.0f;
  }

  return float_symbol;
}

//reset values when no carrier
void no_carrier_sync (Super * super)
{
  //reset some demodulator states
  print_frame_sync_pattern(super, -1);
  super->demod.fsk4_min    = 0.0f;
  super->demod.fsk4_max    = 0.0f;
  super->demod.fsk4_lmid   = 0.0f;
  super->demod.fsk4_umid   = 0.0f;
  super->demod.fsk4_center = 0.0f;
  super->demod.in_sync     = 0;

  //timing
  super->demod.jitter = -1;

  //reset buffers here
  memset (super->demod.float_symbol_buffer, 0.0f, 65540*sizeof(float));
  super->demod.float_symbol_buffer_ptr = 192;
  
  memset (super->demod.sample_buffer, 0, 65540*sizeof(short));
  super->demod.sample_buffer_ptr = 192;

  memset (super->demod.dibit_buffer, 0, 65540*sizeof(uint8_t));
  super->demod.dibit_buffer_ptr = 192;

  //reset some decoder states
  super->m17d.src = 0;
  super->m17d.dst = 0;
  super->m17d.can = -1;

  memset(super->m17d.lsf, 0, sizeof(super->m17d.lsf));
  memset(super->m17d.meta, 0, sizeof(super->m17d.meta));
  super->m17d.dt = 15;
  super->m17d.enc_et = 0;
  super->m17d.enc_st = 0;
  sprintf (super->m17d.dst_csd_str, "%s", "");
  sprintf (super->m17d.src_csd_str, "%s", "");

}

void buffer_refresh_min_max_center (Super * super)
{

  uint16_t i   = 0;
  uint16_t ptr = 0;

  //calculate center, max, and min (apparently, max and min weren't initialized as 0.0f)
  float buffer_max = 0.0f; float buffer_min = 0.0f; float buffer_value = 0.0f;

  for (i = 0; i < 192; i++)
  {

    ptr = super->demod.sample_buffer_ptr-i;
    buffer_value = super->demod.sample_buffer[ptr];

    //debug
    // fprintf (stderr, "\n PTR: %d; BUF: %6.0f;", ptr, buffer_value);

    //clipping and sanity check on buffer_value
    if (buffer_value > +32760.0f) buffer_value = +32760.0f;
    if (buffer_value < -32760.0f) buffer_value = -32760.0f;

    if      (buffer_value > buffer_max) buffer_max = buffer_value;
    else if (buffer_value < buffer_min) buffer_min = buffer_value;
  }

  super->demod.fsk4_max = buffer_max;
  super->demod.fsk4_min = buffer_min;

  super->demod.fsk4_lmid = buffer_min / 2.0f;
  super->demod.fsk4_umid = buffer_max / 2.0f;
  super->demod.fsk4_center = (fabs(buffer_max) - fabs(buffer_min)) / 2.0f;

  //in level value, absolute value of the greater of min or max / greatest value multiplied by 100 for percent
  if (fabs(super->demod.fsk4_min) > fabs(super->demod.fsk4_max))
    super->demod.input_level    = ( (fabs(super->demod.fsk4_min)) / 32767.0f) * 100.0f;
  else super->demod.input_level = ( (fabs(super->demod.fsk4_max)) / 32767.0f) * 100.0f;

}

void simple_refresh_min_max_center (Super * super, float sample)
{

  //NOTE: This will only work if signal levels are consistent, which is okay
  //for testing, but in real world application, this will probably fail

  //clipping and sanity check on sample
  if (sample > +32760.0f) sample = +32760.0f;
  if (sample < -32760.0f) sample = -32760.0f;
  
  //simple straight-forward approach
  if      (sample > super->demod.fsk4_max) super->demod.fsk4_max = sample;
  else if (sample < super->demod.fsk4_min) super->demod.fsk4_min = sample;

  //calculate center, lower middle and upper middle values based on the min and max
  super->demod.fsk4_lmid = super->demod.fsk4_min / 2.0f;
  super->demod.fsk4_umid = super->demod.fsk4_max / 2.0f;

  //disable this if issues arise, not sure if this is okay or not (I think this is okay)
  super->demod.fsk4_center = (fabs(super->demod.fsk4_max) - fabs(super->demod.fsk4_min)) / 2.0f;

  //in level value, absolute value of the greater of min or max / greatest value multiplied by 100 for percent
  if (fabs(super->demod.fsk4_min) > fabs(super->demod.fsk4_max))
    super->demod.input_level    = ( (fabs(super->demod.fsk4_min)) / 32767.0f) * 100.0f;
  else super->demod.input_level = ( (fabs(super->demod.fsk4_max)) / 32767.0f) * 100.0f;
}

uint8_t convert_float_symbol_to_dibit_and_store(Super * super, float float_symbol)
{
  uint8_t dibit = 0;

  //digitize the float symbol into a dibit
  dibit = digitize_symbol_to_dibit(float_symbol);

  //store dibit
  super->demod.dibit_buffer[super->demod.dibit_buffer_ptr++] = dibit;

  //ptr safety truncate (shouldn't be needed due to cast type as uint16_t 
  //into an array larger, but nevertheless, good practice on array indexing)
  super->demod.dibit_buffer_ptr &= 0xFFFF;
  if (super->demod.dibit_buffer_ptr == 0) super->demod.dibit_buffer_ptr = 192;

  //debug
  // fprintf (stderr, "\n DIBIT PTR: %05d; DIBIT: %d", super->demod.dibit_buffer_ptr, super->demod.dibit_buffer[super->demod.dibit_buffer_ptr-1]);

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

//Euclidean Norm will return a distance value based on the 
//normalized value of the sync pattern vs what was received
//in perfect reception, the distance will be 0.0f on a sync pattern
//but we allow a threshold value of 2.0f, which is perfectly fine
//and has no issues with false sync pattern occurrance.
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
    return " No";
}

//high level debug infomation dumps
void print_debug_information(Super * super)
{
  if (super->opts.payload_verbosity >= 3)
  {
    fprintf (stderr, "\n MIN: %f; MAX: %f; LMID: %f; UMID: %f; Center: %f; ", 
      super->demod.fsk4_min, super->demod.fsk4_max, super->demod.fsk4_lmid, 
      super->demod.fsk4_umid, super->demod.fsk4_center);
  }
}

void print_frame_sync_pattern(Super * super, int type)
{
  char * timestr = getTimeN(super->demod.current_time);
  char * syncstr = get_sync_type_string(type);
  fprintf (stderr, "\n");
  if (super->opts.payload_verbosity > 1)
    fprintf (stderr, "INLVL: %2.1f; ", super->demod.input_level);
  fprintf (stderr, "M17 %s Frame Sync (%s): ", syncstr, timestr);
  free (timestr); timestr = NULL;
}

//sometimes you just want it to shut up
void stfu ()
{
  //quell defined but not used warnings from m17.h
  UNUSED(b40); UNUSED(m17_scramble); UNUSED(p1); UNUSED(p3); UNUSED(symbol_map); UNUSED(m17_rrc);
  UNUSED(lsf_sync_symbols); UNUSED(str_sync_symbols); UNUSED(pkt_sync_symbols); UNUSED(symbol_levels);
}