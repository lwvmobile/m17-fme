/*-------------------------------------------------------------------------------
 * fsync.c
 * Project M17 - Frame Sync and Symbol / Dibit Extraction
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "fsync.h"

void framesync (config_opts * opts, pa_state * pa, m17_state * m17)
{
  //look for frame synchronization
  short sample = 0;
  UNUSED(opts);
  UNUSED(sample);
  UNUSED(m17);
  while (!exitflag)
  {
    if (pa->pa_input_is_open)
      sample = pa_input_read(pa);
  }
}