/*-------------------------------------------------------------------------------
 * framesync.c
 * Project M17 - Frame Sync and Symbol / Dibit Extraction
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void framesync (config_opts * opts, pa_state * pa, m17_decoder_state * m17d, demod_state * demod)
{
  //look for frame synchronization
  
  //quell defined but not used warnings from m17.h
  UNUSED(b40); UNUSED(m17_scramble); UNUSED(p1); UNUSED(p3); UNUSED(symbol_map); UNUSED(m17_rrc);
  UNUSED(opts);
  UNUSED(m17d);

  while (!exitflag)
  {
    if (pa->pa_input_is_open)
    {
      #ifdef USE_PULSEAUDIO
      demod->sample_buffer[(demod->sample_buffer_ptr%65535)] = pa_input_read(pa);
      #endif
    }

    //TODO: Actually find a frame sync, pretty sure libm17 likes float samples for its euclidean voodoo
    demod->float_sample_buffer[(demod->sample_buffer_ptr%65535)] = (float)demod->sample_buffer[(demod->sample_buffer_ptr%65535)];

    //increment the sample buffer pointer
    demod->sample_buffer_ptr++;

  }
}