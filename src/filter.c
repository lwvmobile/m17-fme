/*-------------------------------------------------------------------------------
 * filter.c
 * Project M17 - Filter Related Functions
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

#define PI 3.141592653

void HPFilter_Init(HPFilter *filter, float cutoffFreqHz, float sampleTimeS)
{

	float RC=0.0;
	RC=1.0/(2*PI*cutoffFreqHz);

	filter->coef=RC/(sampleTimeS+RC);

	filter->v_in[0]=0.0;
	filter->v_in[1]=0.0;

	filter->v_out[0]=0.0;
	filter->v_out[1]=0.0;

}

float HPFilter_Update(HPFilter *filter, float v_in)
{
    
	filter->v_in[1]=filter->v_in[0];
	filter->v_in[0]=v_in;

	filter->v_out[1]=filter->v_out[0];
	filter->v_out[0]=filter->coef * (filter->v_in[0] - filter->v_in[1]+filter->v_out[1]);

	return (filter->v_out[0]);

}

//high pass filter
void hpfilter(HPFilter * hpf, short * input, int len)
{
  int i;
  for (i = 0; i < len; i++)
	{
		// fprintf (stderr, "\n in: %05d", input[i]);
		input[i] = HPFilter_Update(hpf, input[i]);
		// fprintf (stderr, "\n out: %05d", input[i]);
	}
}

//double check this one, make sure its doing what its suppoed to now
void hpfilter_d(Super * super, short * input, int len)
{
  int i;
  for (i = 0; i < len; i++)
	{
		// fprintf (stderr, "\n in: %05d", input[i]);
		input[i] = HPFilter_Update(&super->hpf_d, input[i]);
		// fprintf (stderr, "\n out: %05d", input[i]);
	}
}