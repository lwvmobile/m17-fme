/*-------------------------------------------------------------------------------
 * framesync.c
 * M17 Project - Frame Sync and Float Symbol / uint8_t Dibit Functions
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void framesync (Super * super)
{
  //sync type
  int type = -1;

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
    type = dist_and_sync(super, super->demod.sync_symbols);

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
int dist_and_sync(Super * super, float * last)
{

  float dist = 99.0f; //euclidean distance

  //LSF
  super->demod.sync_distance = 
  dist = eucl_norm(last, lsf_sync_symbols, 8);
  if (dist < 2.0f) return 1;
  dist = 99.0f; //reset

  //STR
  super->demod.sync_distance = 
  dist = eucl_norm(last, str_sync_symbols, 8);
  if (dist < 2.0f) return 2;
  dist = 99.0f; //reset

  //PKT
  super->demod.sync_distance = 
  dist = eucl_norm(last, pkt_sync_symbols, 8);
  if (dist < 2.0f) return 3;
  dist = 99.0f; //reset

  //BRT
  super->demod.sync_distance = 
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
  uint8_t dibit = 0;

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
    dibit = fgetc (super->opts.dibit_in);

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
    for (i = 0; i < super->demod.fsk4_samples_per_symbol; i++)
    {
      //retrieve sample from audio input handler
      sample = get_short_audio_input_sample(super);

      //Apply Gain to Input
      input_gain_rf (super, &sample, 1);

      //RRC input filtering on sample
      if (!super->opts.disable_rrc_filter)
        sample = rrc_input_filter(super->demod.rrc_input_mem, sample);

      //store locally for inspection inspection
      samples[i] = sample;

      //make adjustments to timing here
      if (super->demod.fsk4_timing_correction > 0)
      {
        i--;
        super->demod.fsk4_timing_correction--;
      }
      else if (super->demod.fsk4_timing_correction < 0)
      {
        i++;
        samples[i] = sample;
        super->demod.fsk4_timing_correction++;
      }

    }

    //calculate min/lmid/center/umid/max vs buffer of last 192 samples
    buffer_refresh_min_max_center(super);

    if (super->opts.disable_symbol_timing)
    {
      // select the best sample based on distance
      sample = basic_sample_selector(super, samples);

      //look at symbol timing (but no corrective value assignment)
      symbol_timing(super, samples);
    }

    else if (!super->opts.disable_symbol_timing)
    {
      //sample as average of 4 center samples
      sample = average_sample_calc(samples);

      //look at symbol timing, set correction value
      super->demod.fsk4_timing_correction = 
        symbol_timing(super, samples);
    } 

    //slice float_symbol from provided sample
    float_symbol = float_symbol_slicer(super, sample);
    
  }

  //store float symbol
  super->demod.float_symbol_buffer[super->demod.float_symbol_buffer_ptr++] = float_symbol;

  //store sample used
  super->demod.sample_buffer[super->demod.sample_buffer_ptr++] = sample;

  //save symbol stream format (M17_Implementations), if opened
  if (super->opts.float_symbol_out && !super->opts.use_m17_duplex_mode && !super->opts.use_m17_textgame_mode)
    fwrite(&float_symbol, sizeof(float), 1, super->opts.float_symbol_out); //sizeof(float) is 4 (usually)

  //save dibits to DSD-FME compatible dibit "symbol" capture bin file format
  if (super->opts.dibit_out && !super->opts.use_m17_duplex_mode && !super->opts.use_m17_textgame_mode) //use -C output.bin to use this format for output
  {
    dibit = digitize_symbol_to_dibit(float_symbol);
    fputc (dibit, super->opts.dibit_out);
  }

  //NOTE: The index pointers are cast as uint8_t values, so they "SHOULD" never be 'negative', or core dump,
  //they will just perpetually roll over back to zero each time so its effectively a ring buffer of 255

  //debug
  if (super->opts.demod_verbosity >= 3)
  {
    fprintf (stderr, "\n FSPTR: %05d; FS: %1.0f; SAPTR: %05d; SAMP: %06d;", 
    super->demod.float_symbol_buffer_ptr, super->demod.float_symbol_buffer[super->demod.float_symbol_buffer_ptr-1], 
    super->demod.sample_buffer_ptr, super->demod.sample_buffer[super->demod.sample_buffer_ptr-1]);
  }

  //return dibit value
  return float_symbol;
}

//return average of four center samples
short average_sample_calc(short * samples)
{
  uint8_t i;
  float average = 0.0f;
  for (i = 3; i < 7; i++) //3,4,5,6
    average += (float)samples[i];

  average /= 4.0f;

  return (short)average;
}

//evaluate 10 samples gathered and determine where the transition edges are
int symbol_timing (Super * super, short * samples)
{

  int i;
  int transition_idx = 0;
  int transition_num = 0;
  float last_symbol = super->demod.float_symbol_buffer[super->demod.float_symbol_buffer_ptr-1];
  float this_symbol = 0.0f;

  if (super->opts.demod_verbosity >= 3)
    fprintf (stderr, "\n Symbol Edge Timing: ");

  sprintf (super->demod.fsk4_timing_string, "Symbol Edge Timing: | ");

  for (i = 0; i < 10; i++)
  {
    this_symbol = float_symbol_slicer(super, samples[i]);
    if (this_symbol == 0.0f) //sample is 0 (nothing on input), and symbol is 0.0f
    {
      if (super->opts.demod_verbosity >= 3)
        fprintf (stderr, "0");

      strcat (super->demod.fsk4_timing_string, "0");
    }
    else if (this_symbol == last_symbol) //no transition, or zero crossing
    {
      if (super->opts.demod_verbosity >= 3)
        fprintf (stderr, "-");
      strcat (super->demod.fsk4_timing_string, "-");
    }
    else if (this_symbol != last_symbol)
    {
      if (!transition_idx)  //if initial transition not set
        transition_idx = i; //set initial transition
      transition_num++;     //number of transitions observed (flips)

      if (this_symbol > last_symbol)
      {
        if (super->opts.demod_verbosity >= 3)
        {
          fprintf (stderr, "/"); //upward
        }
        strcat (super->demod.fsk4_timing_string, "/"); //upward
      }

      else if (this_symbol < last_symbol)
      {
        if (super->opts.demod_verbosity >= 3)
        {
          fprintf (stderr, "\\"); //downward
        }
        strcat (super->demod.fsk4_timing_string, "\\"); //downward
      }

      last_symbol = this_symbol;

    }
  }

  if (super->opts.demod_verbosity >= 3)
    fprintf (stderr, "; Initial Transition: %02d; Number of Transitions: %02d;", transition_idx, transition_num);

  if (super->opts.demod_verbosity >= 3)
    fprintf (stderr, "; Last Symbol: %2.0f; This Symbol: %2.0f;", last_symbol, this_symbol); //look at samples when zero crossing (same symbol repeated)

  if (super->opts.demod_verbosity >= 3)
  {
    fprintf (stderr, "\n Samples:");
    for (i = 0; i < 10; i++)
    {
      if (samples[i] > 0)
        fprintf (stderr, " +%05d;", samples[i]);
      else fprintf (stderr, " %06d;", samples[i]);
    }
  }

  strcat (super->demod.fsk4_timing_string, " |");

  if (this_symbol != 0.0f && super->demod.in_sync == 0)
  {
    if      (  transition_num == 0     ) return +0;
    else if ( (transition_idx - 5) > 0 ) return -1;
    else if ( (transition_idx - 5) < 0 ) return +1;
  }

  return +0;

}

//crude selection procedure to determine what the optimal sample value is
short basic_sample_selector(Super * super, short * samples)
{

  //NOTE: This function is not meant to be a true method of FSK4
  //clock aquisition or proper sampling, its just super basic and
  //meant to get through the development phase.

  int i = 0;
  int use_sample = 0;
  short sample = 0;
  
  float difference[10]; memset (difference, 0.0f, 10*sizeof(float));
  float min_dist = 32767.0f;

  //find difference between middle samples and find optimal sample for collection
  for (i = 0; i < 9; i++) //3, 7
  {
    difference[i] = (float)samples[i+1] - (float)samples[i];
    if (i >= 3 && i <= 7) //3,4,5,6,7 (half of symbol period)
    {
      if (fabs(difference[i]) < min_dist)
      {
        min_dist = fabs(difference[i]);
        use_sample = i;
      }
    }
  }

  sample = (short)samples[use_sample+1]; //or +0

  if (super->opts.demod_verbosity >= 2)
  {
    fprintf (stderr, "\n Samples:");
    for (i = 0; i < 10; i++)
    {
      if (samples[i] > 0)
        fprintf (stderr, " +%05d;", samples[i]);
      else fprintf (stderr, " %06d;", samples[i]);
    }
  }

  if (super->opts.demod_verbosity >= 2)
  {
    fprintf (stderr, "\nDIFF:");
    for (i = 0; i < 9; i++)
      fprintf (stderr, " %6.0f;", difference[i]);
    fprintf (stderr, " USE: %d:%6.0f:%6d;", use_sample, difference[use_sample], sample);
  }

  return sample;
}

float float_symbol_slicer(Super * super, short sample)
{
  float float_symbol = 0.0f;

  //return 0.0f, if a zero sample (nothing on input)
  if (sample == 0)
    return float_symbol;

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

  //init BERT
  init_brt();

  //push call history items
  push_call_history(super);

  //clear out lasteventstring to allow a new call from same source go to call history
  sprintf (super->m17d.lasteventstring, "%s", "");

  //close per call wav file, if opened
  if (super->wav.wav_out_pc)
    close_wav_out_pc (super);

  //frame sync
  memset (super->demod.sync_symbols, 0, 8*sizeof(float));
  super->demod.sync_distance = 8.0f;

  //reset buffers here
  memset (super->demod.float_symbol_buffer, 0.0f, 256*sizeof(float));  
  memset (super->demod.sample_buffer, 0, 256*sizeof(short));
  memset (super->demod.dibit_buffer, 0, 256*sizeof(uint8_t));

  memset(super->m17d.meta, 0, sizeof(super->m17d.meta));
  memset(super->m17d.lsf, 0, sizeof(super->m17d.lsf));

  if (super->opts.use_m17_duplex_mode == 0)
  {
    //reset some decoder elements
    super->m17d.src = 0;
    super->m17d.dst = 0;
    super->m17d.can = -1;

    // memset(super->m17d.lsf, 0, sizeof(super->m17d.lsf));
    // memset(super->m17d.meta, 0, sizeof(super->m17d.meta));
    super->m17d.dt = 15;
    super->m17d.enc_et = 0;
    super->m17d.enc_st = 0;
    super->m17d.enc_mute = 0;
    sprintf (super->m17d.dst_csd_str, "%s", "         ");
    sprintf (super->m17d.src_csd_str, "%s", "         ");
  }

  //reset scrambler fn value and seed value
  super->enc.scrambler_fn_d = 0;
  super->enc.scrambler_seed_d = super->enc.scrambler_key;

  //below items were disabled, causing stale GNSS (any reason why this was disabled, probably so it doesn't clear out of the encode window in ncurses?)
  memset (super->m17d.raw, 0, sizeof(super->m17d.raw));
  sprintf (super->m17d.sms, "%s", "");
  sprintf (super->m17d.dat, "%s", "");
  sprintf (super->m17d.arb, "%s", "");
  super->m17d.packet_protocol = 0; //may only need this one, then we can keep the items above in the display

  //ECDSA
  memset (super->m17d.ecdsa.curr_stream_pyl, 0, 16*sizeof(uint8_t));
  memset (super->m17d.ecdsa.last_stream_pyl, 0, 16*sizeof(uint8_t));
  memset (super->m17d.ecdsa.signature, 0, 64*sizeof(uint8_t));

}

void buffer_refresh_min_max_center (Super * super)
{

  uint8_t i   = 0;
  uint8_t ptr = 0;

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

uint8_t convert_float_symbol_to_dibit_and_store(Super * super, float float_symbol)
{
  uint8_t dibit = 0;

  //digitize the float symbol into a dibit
  dibit = digitize_symbol_to_dibit(float_symbol);

  //store dibit
  super->demod.dibit_buffer[super->demod.dibit_buffer_ptr++] = dibit;

  //NOTE: The index pointers are cast as uint8_t values, so they will never be 'negative', or core dump,
  //they will just perpetually roll over back to zero each time so its effectively a ring buffer of 255

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
  char * datestr = get_date_n(super->demod.current_time);
  char * timestr = get_time_n(super->demod.current_time);
  char * syncstr = get_sync_type_string(type);
  fprintf (stderr, "\n");
  
  fprintf (stderr, "(%s %s) M17 %s Frame Sync: ", datestr, timestr, syncstr);

  if (super->opts.demod_verbosity >= 1)
    fprintf (stderr, "INLVL: %2.1f; ", super->demod.input_level);

  //free allocated memory
  if (datestr != NULL)
  {
    free (datestr);
    datestr = NULL;
  }

  if (timestr != NULL)
  {
    free (timestr);
    timestr = NULL;
  }

}