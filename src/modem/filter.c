/*-------------------------------------------------------------------------------
 * filter.c
 * M17 Project - Filter Related Functions
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void hpfilter_init(hpfilter *filter, float cutoffFreqHz, float sampleTimeS)
{

  float RC=0.0;
  RC=1.0/(2*PI*cutoffFreqHz);

  filter->coef=RC/(sampleTimeS+RC);

  filter->v_in[0]=0.0;
  filter->v_in[1]=0.0;

  filter->v_out[0]=0.0;
  filter->v_out[1]=0.0;

}

float hpfilter_update(hpfilter *filter, float v_in)
{
    
  filter->v_in[1]=filter->v_in[0];
  filter->v_in[0]=v_in;

  filter->v_out[1]=filter->v_out[0];
  filter->v_out[0]=filter->coef * (filter->v_in[0] - filter->v_in[1]+filter->v_out[1]);

  return (filter->v_out[0]);

}

//high pass filter for Codec2 Digital Audio Output
void hpfilter_d(Super * super, short * input, int len)
{
  int i;

  //apply filtering
  for (i = 0; i < len; i++)
    input[i] = hpfilter_update(&super->hpf_d, input[i]);

  //boost gain by factor of 1.75f to compensate for audio level drop
  for (i = 0; i < len; i++)
    input[i] *= 1.75f;
}

//10x Upscale and RRC filtering lifted from M17_Implementations / libM17
void upscale_and_rrc_output_filter (int * output_symbols, float * mem, short * baseband)
{
  int i = 0; int j = 0; int k = 0; int x = 0;
  float mac = 0.0f;
  for (i = 0; i < 192; i++)
  {
    mem[0] = (float)output_symbols[i] * 7168.0f;

    for (j = 0; j < 10; j++)
    {

      mac = 0.0f;

      //calc the sum of products
      for (k = 0; k < 81; k++)
        mac += mem[k]*m17_rrc[k]*sqrtf(10.0);

      //shift the delay line right by 1
      for (k = 80; k > 0; k--)
        mem[k] = mem[k-1];

      mem[0] = 0.0f;

      baseband[x++] = (short)mac;
    }
  }
}

short rrc_input_filter(float * mem, short sample)
{
  int i = 0;
	int len = 79;
  float sum = 0.0f;
  float out = 0.0f;
	float gain = 10.0f;

  //push memory
  for (i = 0; i < (len-1); i++)
    mem[i] = mem[i+1];
  mem[len-1] = (float)sample;

  for (i = 0; i < len; i++)
    sum += m17_input_rrc[i] * mem[i];

  out = sum / gain;

  return (short)out;
}

void asljdf()
{
	stfu();
}