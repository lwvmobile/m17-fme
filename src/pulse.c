/*-------------------------------------------------------------------------------
 * pulse.c
 * Project M17 - Pulse Audio Handler
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

#ifdef USE_PULSEAUDIO

int err; char session_name[50]; char app_name[10];
void open_pulse_audio_input (Super * super)
{
  //differentiate input names so that the mixer will remember the input device used locally
  if (super->opts.use_m17_rfa_decoder == 1)
  {
    sprintf (session_name, "RF Input %04X", super->opts.random_number);
    sprintf (app_name, "M17-FME1");
  }
  else
  {
    sprintf (session_name, "Voice Input %04X", super->opts.random_number);
    sprintf (app_name, "M17-FME2");
  }
  super->pa.pa_input_device = pa_simple_new(NULL, app_name, PA_STREAM_RECORD, NULL, session_name, &super->pa.input, NULL, &super->pa.inputlt, &err);
  super->pa.pa_input_is_open = 1;
  if (err < 0)
  {
    fprintf (stderr, "%s", pa_strerror(err));
    exit(0);
  }
}

void open_pulse_audio_output_rf (Super * super)
{
  sprintf (session_name, "RF Output %04X", super->opts.random_number);
  super->pa.pa_output_device_rf = pa_simple_new(NULL, "M17-FME3", PA_STREAM_PLAYBACK, NULL, session_name, &super->pa.output_rf, NULL, NULL, &err);
  super->pa.pa_output_rf_is_open = 1;
  if (err < 0)
  {
    fprintf (stderr, "%s", pa_strerror(err));
    exit(0);
  }
}

void open_pulse_audio_output_vx (Super * super)
{
  sprintf (session_name, "Voice Output %04X", super->opts.random_number);
  super->pa.pa_output_device_vx = pa_simple_new(NULL, "M17-FME4", PA_STREAM_PLAYBACK, NULL, session_name, &super->pa.output_vx, NULL, NULL, &err);
  super->pa.pa_output_vx_is_open = 1;
  if (err < 0)
  {
    fprintf (stderr, "%s", pa_strerror(err));
    exit(0);
  }
}

void close_pulse_audio_input (Super * super)
{
  pa_simple_free (super->pa.pa_input_device);
  super->pa.pa_input_is_open = 0;
}

void close_pulse_audio_output_rf (Super * super)
{
  pa_simple_drain(super->pa.pa_output_device_rf, NULL);
  pa_simple_free (super->pa.pa_output_device_rf);
  super->pa.pa_output_rf_is_open = 0;
}

void close_pulse_audio_output_vx (Super * super)
{
  pa_simple_drain(super->pa.pa_output_device_vx, NULL);
  pa_simple_free (super->pa.pa_output_device_vx);
  super->pa.pa_output_vx_is_open = 0;
}

//return a single short sample from pulse audio input
short pa_input_read (Super * super)
{
  short sample = 0;
  pa_simple_read(super->pa.pa_input_device, &sample, 2, &err);
  if (err < 0)
  {
    fprintf (stderr, "%s", pa_strerror(err));
    // exit(0); //unsure if we want to exit here, may report underrun or similar issues, unknown
  }

  return sample;
}

void pulse_audio_output_rf(Super * super, short * out, size_t nsam)
{
  pa_simple_write(super->pa.pa_output_device_rf, out, nsam*2, &err);
  if (err < 0)
  {
    fprintf (stderr, "%s", pa_strerror(err));
    // exit(0);
  }
}

void pulse_audio_output_vx(Super * super, short * out, size_t nsam)
{
  pa_simple_write(super->pa.pa_output_device_vx, out, nsam*2, &err);
  if (err < 0)
  {
    fprintf (stderr, "%s", pa_strerror(err));
    // exit(0);
  }
}

#endif