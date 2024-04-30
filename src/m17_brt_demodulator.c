/*-------------------------------------------------------------------------------
 * m17_brt_demodulator.c
 * Project M17 - BERT (Bit Error Rate Test) Frame Demodulation and Debug
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

//just dump what comes over the air, not going to setup and run the BERT test in this commit
void demod_brt(Super * super, uint8_t * input, int debug)
{
  //quell defined but not used warnings from m17.h
  stfu ();

  int i;
  uint8_t dbuf[184]; memset (dbuf, 0, sizeof(dbuf));
  uint8_t bert[368]; memset (bert, 0, sizeof(bert));

  if (debug == 0)
  {
    for (i = 0; i < 184; i++)
      dbuf[i] = get_dibit(super);

    //convert dbuf into a bit array
    for (i = 0; i < 184; i++)
    {
      bert[i*2+0] = (dbuf[i] >> 1) & 1;
      bert[i*2+1] = (dbuf[i] >> 0) & 1;
    }
  }
  else memcpy (bert, input, 368);

  for (i = 0; i < 46; i++)
    fprintf (stderr, "%02X", (uint8_t)ConvertBitIntoBytes(&bert[i*8], 8));

  //get rid of this if it costs too much CPU / skips / lags
  super->demod.sync_time = super->demod.current_time = time(NULL);

}