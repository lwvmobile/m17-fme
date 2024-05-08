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
  for (int i = 0; i < 192*1; i++)
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

    else if (type == 4)
      demod_brt(super, NULL, 0);

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
  dist = eucl_norm(last, brt_sync_symbols, 8);
  if (dist < 2.0f) return 4;
  dist = 99.0f; //reset

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

    short samples[10]; memset (samples, 0, 10*sizeof(short));

    //gather number of samples_per_symbol, and store them locally for inspection
    for (i = 0; i < super->demod.fsk4_samples_per_symbol-super->demod.fsk4_offset_correction; i++)
    {
      //retrieve sample from audio input handler
      sample = get_short_audio_input_sample(super);

      //Apply Gain to Input
      input_gain_rf (super, &sample, 1);

      //RRC input filtering on sample
      if (!super->opts.disable_rrc_filter)
        sample = rrc_input_filter(super->demod.rrc_input_mem, sample);

      //store locally for clock recover / transition inspection
      samples[i] = sample;

      //if offset, then duplicate samples and reset
      if (super->demod.fsk4_offset_correction)
      {
        for (int j = super->demod.fsk4_offset_correction; j < super->demod.fsk4_samples_per_symbol; j++)
          samples[j] = sample; ///assign any skipped samples as the last received one instead
        super->demod.fsk4_offset_correction = 0;
      }

    }

    //calculate min/lmid/center/umid/max vs buffer of last 192 samples
    buffer_refresh_min_max_center(super);

    //vote for the best sample based on procedural criteria
    sample = vote_for_sample(super, samples);

    //test for clock recovery (offset value)
    // clock_recovery(super, samples);

    //slice float_symbol from provided sample
    float_symbol = float_symbol_slicer(super, sample);

  }

  //store float symbol
  super->demod.float_symbol_buffer[super->demod.float_symbol_buffer_ptr++] = float_symbol;

  //store sample used
  super->demod.sample_buffer[super->demod.sample_buffer_ptr++] = sample;

  //ptr safety truncate (shouldn't be needed due to cast type as uint16_t 
  //into an array larger, but nevertheless, good practice on array indexing)
  super->demod.float_symbol_buffer_ptr &= 0x3FF; //smaller buffer
  super->demod.sample_buffer_ptr       &= 0x3FF; //smaller buffer
  if (super->demod.float_symbol_buffer_ptr == 0) super->demod.float_symbol_buffer_ptr = 192;
  if (super->demod.sample_buffer_ptr == 0) super->demod.sample_buffer_ptr = 192;

  //debug
  if (super->opts.demod_verbosity >= 3)
    fprintf (stderr, "\n FSPTR: %05d; FS: %1.0f; SAPTR: %05d; SAMP: %06d;", super->demod.float_symbol_buffer_ptr, super->demod.float_symbol_buffer[super->demod.float_symbol_buffer_ptr-1], super->demod.sample_buffer_ptr, super->demod.sample_buffer[super->demod.sample_buffer_ptr-1]);

  //return dibit value
  return float_symbol;
}

//voting procedure to determine what the optimal sample value is
short vote_for_sample(Super * super, short * samples)
{

  int i = 0;
  int use_sample = 0;
  short vote = 0;
  
  float difference[10]; memset (difference, 0.0f, 10*sizeof(float));
  float slope[10]; memset (slope, 0.0f, 10*sizeof(float));
  float min_dist = 32767.0f;

  //simple, pick center-ish sample (fallback)
  // vote = samples[super->demod.fsk4_sample_center];

  //find difference between middle samples
  //and find optimal sample for collection
  for (i = 3; i < 7; i++)
  {
    difference[i] = (float)samples[i+1] - (float)samples[i];
    if (fabs(difference[i]) < min_dist)
    {
      min_dist = fabs(difference[i]);
      use_sample = i;
    }
  }

  //calculate tangent line slope between each sample pair
  float dx = PI / 40; //PI / 4 / 10 samples per symbol??
  if (super->opts.demod_verbosity >= 2)
    fprintf (stderr, "\nSLP:");
  for (i = 0; i < 9; i++)
  {
    //y = m(x) + b //good old linear equation
    slope[i] = ( (float)samples[i+1] - (float)samples[i] ) / dx;
    if (super->opts.demod_verbosity >= 2)
      fprintf (stderr, " %6.0f;", slope[i]);
  }

  vote = (short)samples[use_sample+1]; //or +0

  if (super->opts.demod_verbosity >= 2)
  {
    fprintf (stderr, "\nVFS:");
    for (i = 3; i < 7; i++)
      fprintf (stderr, " %6.0f;", difference[i]);
    fprintf (stderr, " USE: %d:%6.0f:%6d;", use_sample, difference[use_sample], vote);
  }

  return vote;
}

//very crude clock recovery based on finding a transition edge (doesn't work very well sometimes)
void clock_recovery(Super * super, short * samples)
{

  int i = 0;
  float first   = 0.0f;
  float fsample = 0.0f;
  float flevel  = 0.0f;
  float fnexts  = 0.0f;
  
  if (super->opts.demod_verbosity >= 3)
  {
    fprintf (stderr, "\nLTS:");
    for (i = 0; i < 10; i++)
      fprintf (stderr, " %06d;", samples[i]);
  }

  first = (float)samples[0];
  if (first < super->demod.fsk4_lmid)
    flevel = -3.0f;
  else if (first > super->demod.fsk4_lmid   && first < super->demod.fsk4_center)
    flevel = -1.0f;
  else if (first > super->demod.fsk4_center && first < super->demod.fsk4_umid)
    flevel = +1.0f;
  else if (first > super->demod.fsk4_umid)
    flevel = +3.0f;

  //look for a transitional value when what we had on sample 0 is no longer true for the ith value
  for (i = 1; i < 10; i++) //1 to 10
  {
    fsample = (float)samples[i];
    if (fsample < super->demod.fsk4_lmid)
      fnexts = -3.0f;
    else if (fsample > super->demod.fsk4_lmid   && fsample < super->demod.fsk4_center)
      fnexts = -1.0f;
    else if (fsample > super->demod.fsk4_center && fsample < super->demod.fsk4_umid)
      fnexts = +1.0f;
    else if (fsample > super->demod.fsk4_umid)
      fnexts = +3.0f;

    //assign the offset to the ith value for the transition edge
    if (fnexts != flevel)
    {
      if (super->opts.demod_verbosity >= 2)
        fprintf (stderr, "\nClock Recovery: i:%d; F: %6.0f; N: %6.0f;", i, first, fsample);
      super->demod.fsk4_offset_correction = i;
      // super->demod.fsk4_offset_correction = 8; //just try one
      break;
    }

  }

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

  //push call history items
  push_call_history(super);

  //frame sync and timing recovery
  memset (super->demod.sync_symbols, 0, 8*sizeof(float));
  super->demod.fsk4_offset_correction = 0;

  //reset buffers here
  memset (super->demod.float_symbol_buffer, 0.0f, 65540*sizeof(float));
  super->demod.float_symbol_buffer_ptr = 192;
  
  memset (super->demod.sample_buffer, 0, 65540*sizeof(short));
  super->demod.sample_buffer_ptr = 192;

  memset (super->demod.dibit_buffer, 0, 65540*sizeof(uint8_t));
  super->demod.dibit_buffer_ptr = 192;

  //reset some decoder elements
  super->m17d.src = 0;
  super->m17d.dst = 0;
  super->m17d.can = -1;

  memset(super->m17d.lsf, 0, sizeof(super->m17d.lsf));
  memset(super->m17d.meta, 0, sizeof(super->m17d.meta));
  super->m17d.dt = 15;
  // super->m17d.enc_et = 0;
  // super->m17d.enc_st = 0;
  // super->m17d.enc_mute = 0;
  sprintf (super->m17d.dst_csd_str, "%s", "         ");
  sprintf (super->m17d.src_csd_str, "%s", "         ");

  // memset (super->m17d.raw, 0, sizeof(super->m17d.raw));
  // sprintf (super->m17d.sms, "%s", "");
  // sprintf (super->m17d.dat, "%s", "");
  // sprintf (super->m17d.arb, "%s", "");

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
    super->demod.input_level    = ( (fabs(super->demod.fsk4_min)) / 32760.0f) * 100.0f;
  else super->demod.input_level = ( (fabs(super->demod.fsk4_max)) / 32760.0f) * 100.0f;

  if (super->opts.demod_verbosity >= 3)
  {
    fprintf (stderr, "\n Last 192 - Min: %6.0f; Max: %5.0f; LMid: %6.0f; UMid: %5.0f; Center: %6.0f; In: %2.0f", 
      super->demod.fsk4_min, super->demod.fsk4_max, super->demod.fsk4_lmid, 
      super->demod.fsk4_umid, super->demod.fsk4_center, super->demod.input_level);
  }

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
    super->demod.input_level    = ( (fabs(super->demod.fsk4_min)) / 32760.0f) * 100.0f;
  else super->demod.input_level = ( (fabs(super->demod.fsk4_max)) / 32760.0f) * 100.0f;
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
  if (super->opts.demod_verbosity >= 4)
    fprintf (stderr, "\n DIBIT PTR: %05d; DIBIT: %d", super->demod.dibit_buffer_ptr, super->demod.dibit_buffer[super->demod.dibit_buffer_ptr-1]);

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
  //quell defined but not used warnings from m17.h
  stfu ();

  if (super->opts.demod_verbosity >= 1)
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
  if (super->opts.demod_verbosity >= 1)
    fprintf (stderr, "INLVL: %2.1f; ", super->demod.input_level);
  fprintf (stderr, "M17 %s Frame Sync (%s): ", syncstr, timestr);
  free (timestr); timestr = NULL;
}

void push_call_history (Super * super)
{

  char dt[9]; memset (dt, 0, 9*sizeof(char));
  if      (super->m17d.dt == 0) sprintf (dt, "RESERVED");
  else if (super->m17d.dt == 1) sprintf (dt, "PKT DATA");
  else if (super->m17d.dt == 2) sprintf (dt, "VOX 3200");
  else if (super->m17d.dt == 3) sprintf (dt, "V+D 1600");
  else if (super->m17d.dt == 4) sprintf (dt, "RESET DM");
  else if (super->m17d.dt == 5) sprintf (dt, "IPF DISC");
  else if (super->m17d.dt == 6) sprintf (dt, "IPF CONN");
  else                          sprintf (dt, "UNK TYPE");

  char * timestr  = getTimeN(super->demod.current_time);
  char * datestr  = getDateN(super->demod.current_time);
  for (uint8_t i = 0; i < 9; i++)
    memcpy (super->m17d.callhistory[i], super->m17d.callhistory[i+1], 500*sizeof(char));
  sprintf (super->m17d.callhistory[9], "%s %s CAN: %02d; SRC: %s; DST: %s; %s;", datestr, timestr, super->m17d.can, super->m17d.src_csd_str, super->m17d.dst_csd_str, dt);

  //TODO: Implement an event log / write events to a file, such as the call history string and any PKT messages

  free (timestr); free (datestr);
}