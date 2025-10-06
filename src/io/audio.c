/*-------------------------------------------------------------------------------
 * audio.c
 * M17 Project - Audio Related Functions
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//convenience function to retrieve 1 short input sample from any hardware or sndfile rf audio
short get_short_audio_input_sample (Super * super)
{
  short sample = 0;
  
  //SNFILE Audio
  if (super->opts.use_snd_input == 1)
    sample = snd_input_read(super);

  #ifdef USE_PULSEAUDIO
  //PULSE AUDIO (obviously)
  else if (super->opts.use_pa_input == 1)
    sample = pa_input_read(super);
  else if (super->opts.use_pa_input_vx == 1)
    sample = pa_input_read_vx(super);
  #endif

  //OSS Hot Garbage
  else if (super->opts.use_oss_input == 1)
    sample = oss_input_read(super);

  //contribute sample to raw audio monitor for playback if enabled
  raw_audio_monitor (super, sample);

  return sample;
}

//simple 6x 8K to 48K upsample
void upsample_6x(short input, short * output)
{
  int i;
  for (i = 0; i < 6; i++)
    output[i] = input;
}

//generic rms function to use (use it after hpf)
long int raw_rms(int16_t *samples, int len, int step)
{
  
  int i;
  long int rms;
  long p, t, s;
  double dc, err;

  p = t = 0L;
  for (i=0; i<len; i+=step) {
    s = (long)samples[i];
    t += s;
    p += s * s;
  }
  /* correct for dc offset in squares */
  dc = (double)(t*step) / (double)len;
  err = t * 2 * dc - dc * dc * len;

  rms = (long int)sqrt((p-err) / len);
  if (rms < 0) rms = 150;
  return rms;
}

//applies a float gain value to input voice samples
void input_gain_rf (Super * super, short * input, int len)
{
  int i;
  for (i = 0; i < len; i++)
    input[i] *= super->opts.input_gain_rf;
}

//applies a float gain value to input voice samples
void input_gain_vx (Super * super, short * input, int len)
{
  int i;
  for (i = 0; i < len; i++)
    input[i] *= super->opts.input_gain_vx;
}

//applies a float gain value to output rf samples
void output_gain_rf (Super * super, short * input, int len)
{
  int i;
  for (i = 0; i < len; i++)
    input[i] *= super->opts.output_gain_rf;
}

//applies a float gain value to output voice samples
void output_gain_vx (Super * super, short * input, int len)
{
  int i;
  for (i = 0; i < len; i++)
  {
    input[i] *= super->opts.output_gain_vx;

    //clip gaurd
    if (input[i] > 32760)
      input[i] = 32760;
    else if (input[i] < -32760)
      input[i] = -32760;
  }
}

//reset auto gain at EOT (no_carrier)
void reset_auto_gain_vx (Super * super)
{
  memset (super->demod.max_history_buffer, 0, 256*sizeof(float));
  super->demod.max_history_buffer_ptr = 0;
  super->opts.output_gain_vx = 1.0f;
}

//calculate and apply auto gain to any set of input sample
void auto_gain_vx (Super * super, short * input, int len)
{

  float abs_value = 0.0f;
  float max_samp  = 0.0f;
  float max_buf   = 0.0f;
  int max_mod     = 12*2; //was 256 previously

  float gain_factor_clamp = 25.0f; //was 30.0f previously
  float target_amp_percent = 0.75f; //was 1.0f previously
  float target_amplitude = 16384.0f * target_amp_percent;

  for (int i = 0; i < len; i++)
  {
    abs_value = fabsf((float)input[i]);
    if (abs_value > max_samp)
      max_samp = abs_value;

  }
  super->demod.max_history_buffer[ super->demod.max_history_buffer_ptr % max_mod ] = max_samp;

  //debug
  // fprintf (stderr, " S Max: %f; ", max_samp);

  //lookup max history
  for (int i = super->demod.max_history_buffer_ptr; i < super->demod.max_history_buffer_ptr+max_mod; i++)
  {
    abs_value = super->demod.max_history_buffer[i%max_mod];
    if (abs_value > max_buf)
      max_buf = abs_value;
  }

  //add a miniscule value so we don't divide by zero
  if (max_buf == 0.0f)
    max_buf += 0.01f;

  //debug
  // fprintf (stderr, " H Max: %f; ", max_buf);

  //make adjustment to gain
  if (super->opts.auto_gain_voice == 1)
  {
    float gain_factor = target_amplitude / max_buf;

    if (gain_factor > 0.01f && gain_factor <= gain_factor_clamp)
      super->opts.output_gain_vx = gain_factor;

    //debug
    float dB = 20.0f * log10f(max_buf / 32768);
    if (super->opts.demod_verbosity >= 1)
      fprintf (stderr, " Gain Factor: %0.1f; dB: %0.1f;", gain_factor, dB);
  }

  //increment index pointer
  super->demod.max_history_buffer_ptr++;

  output_gain_vx(super, input, len);

}

void raw_audio_monitor (Super * super, short sample)
{
  //add sample to the raw audio buffer
  super->demod.raw_audio_buffer[super->demod.raw_audio_buffer_ptr++] = sample;

  //if buffer is at saturation, playback and discharge buffer
  if (super->demod.raw_audio_buffer_ptr >= 960)
  {

    //playback buffered samples, if raw audio monitor is enabled and no sync
    if (super->opts.use_raw_audio_monitor && !super->demod.in_sync)
    {
      //Pulse Audio Playback
      #ifdef USE_PULSEAUDIO
      if (super->pa.pa_output_vx_is_open == 1)
        pulse_audio_output_vx(super, super->demod.raw_audio_buffer, 960);
      #else
      if (super->pa.pa_output_vx_is_open == 1) {}
      #endif

      //OSS
      else if (super->opts.oss_output_device)
        oss_output_write(super, super->demod.raw_audio_buffer, 960);

      //STDOUT
      else if (super->opts.stdout_pipe)
        write_stdout_pipe(super, super->demod.raw_audio_buffer, 960);

    }

    //discharge samples and reset pointer
    memset (super->demod.raw_audio_buffer, 0, 960*sizeof(short));
    super->demod.raw_audio_buffer_ptr = 0;
  }
}