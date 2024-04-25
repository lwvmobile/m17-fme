/*-------------------------------------------------------------------------------
 * audio.c
 * Project M17 - Audio Related Functions
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//convenience function to retrieve 1 short input sample from any number of input methods
short get_short_audio_input_sample (Super * super)
{
  short sample = 0;
  int err = 0; UNUSED(err); //will use for TCP, other input methods
  //STDIN
  if (super->opts.use_stdin_input == 1)
    sample = read_stdin_pipe(super);

  #ifdef USE_PULSEAUDIO
  //PULSE AUDIO (obviously)
  else if (super->opts.use_pa_input == 1)
    sample = pa_input_read(super);
  #endif

  //TOOD: Flesh this out
  /*
  
  //OSS
  read (super->opts.audio_in_fd, &sample, 2);

  //TCP (via SF wav input)
  err = sf_read_short(super->opts.tcp_file_in, &sample, 1);

  //??? Others?

  */

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