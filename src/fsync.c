/*-------------------------------------------------------------------------------
 * fsync.c
 * Project M17 - Frame Sync and Symbol / Dibit Extraction
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "fsync.h"

void framesync (config_opts * opts, pa_state * pa, m17_decoder_state * m17d, demod_state * demod)
{
  //look for frame synchronization

  UNUSED(opts);
  UNUSED(m17d);

  while (!exitflag)
  {
    if (pa->pa_input_is_open)
      demod->sample_buffer[(demod->sample_buffer_ptr%65535)] = pa_input_read(pa);

    //TODO: Actually find a frame sync, pretty sure libm17 likes float samples for its euclidean voodoo
    demod->float_sample_buffer[(demod->sample_buffer_ptr%65535)] = (float)demod->sample_buffer[(demod->sample_buffer_ptr%65535)];

    //increment the sample buffer pointer
    demod->sample_buffer_ptr++;

  }
}