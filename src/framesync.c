/*-------------------------------------------------------------------------------
 * framesync.c
 * Project M17 - Frame Sync and Symbol / Dibit Extraction
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void framesync (Super * super)
{
  //look for frame synchronization

  //quell defined but not used warnings from m17.h
  UNUSED(b40); UNUSED(m17_scramble); UNUSED(p1); UNUSED(p3); UNUSED(symbol_map); UNUSED(m17_rrc);

  while (!exitflag)
  {
    if (super->pa.pa_input_is_open)
    {
      #ifdef USE_PULSEAUDIO
      // super->demod.sample_buffer[(super->demod.sample_buffer_ptr%65535)] = pa_input_read(pa);
      #endif
    }

    //TODO: Actually find a frame sync, pretty sure libm17 likes float samples for its euclidean voodoo
    super->demod.float_sample_buffer[(super->demod.sample_buffer_ptr%65535)] = (float)super->demod.sample_buffer[(super->demod.sample_buffer_ptr%65535)];

    //increment the sample buffer pointer
    super->demod.sample_buffer_ptr++;

  }
}