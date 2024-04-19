/*-------------------------------------------------------------------------------
 * init.c
 * Project M17 - Pulse Audio Handler
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//TODO: Add Error Handling

void open_pulse_audio_input (pa_state * pa)
{
  pa->pa_input_device = pa_simple_new(NULL, "M17-FME1", PA_STREAM_RECORD, NULL, "Voice Input", &pa->input, NULL, &pa->inputlt, NULL);
  pa->pa_input_is_open = 1;
}

void open_pulse_audio_output_rf (pa_state * pa)
{
  pa->pa_output_device_rf = pa_simple_new(NULL, "M17-FME3", PA_STREAM_PLAYBACK, NULL, "RF Audio Output", &pa->output_rf, NULL, NULL, NULL);
  pa->pa_output_rf_is_open = 1;
}

void open_pulse_audio_output_vx (pa_state * pa)
{
  pa->pa_output_device_vx = pa_simple_new(NULL, "M17-FME2", PA_STREAM_PLAYBACK, NULL, "Voice Output", &pa->output_vx, NULL, NULL, NULL);
  pa->pa_output_vx_is_open = 1;
}

void close_pulse_audio_input (pa_state * pa)
{
  pa_simple_free (pa->pa_input_device);
  pa->pa_input_is_open = 0;
}

void close_pulse_audio_output_rf (pa_state * pa)
{
  pa_simple_free (pa->pa_output_device_rf);
  pa->pa_output_rf_is_open = 0;
}

void close_pulse_audio_output_vx (pa_state * pa)
{
  pa_simple_free (pa->pa_output_device_vx);
  pa->pa_output_vx_is_open = 0;
}

//return a single short sample from pulse audio input
short pa_input_read (pa_state * pa)
{
  short sample = 0;
  pa_simple_read(pa->pa_input_device, &sample, 2, NULL );

  return sample;
}

void pulse_audio_output_rf(pa_state * pa, short * out, size_t nsam)
{
  pa_simple_write(pa->pa_output_device_rf, out, nsam*2, NULL);
}

void pulse_audio_output_vx(pa_state * pa, short * out, size_t nsam)
{
  pa_simple_write(pa->pa_output_device_vx, out, nsam*2, NULL);
}

//simple 6x 8K to 48K upsample
void upsample_6x(short input, short * output)
{
  int i;
  for (i = 0; i < 6; i++)
    output[i] = input;
}

