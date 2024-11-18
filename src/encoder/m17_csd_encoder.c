/*-------------------------------------------------------------------------------
 * m17_csd_encoder.c
 * M17 Project - Callsign Data Encoder
 *
 * LWVMOBILE
 * 2024-11 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void encode_callsign_data(Super * super, char * d40, char * s40, unsigned long long int * dst, unsigned long long int * src)
{

  //quell defined but not used warnings from m17.h
  stfu ();
  
  int i, j;

  if (super->m17e.srcs[0] != 0)
    sprintf (s40, "%s", super->m17e.srcs);

  if (super->m17e.dsts[0] != 0)
    sprintf (d40, "%s", super->m17e.dsts);

  //if special values, then assign them
  if (strcmp (d40, "BROADCAST") == 0)
    *dst = 0xFFFFFFFFFFFF;
  if (strcmp (d40, "ALL") == 0)
    *dst = 0xFFFFFFFFFFFF;

  //Only if not already set to a reserved value
  if (*dst < 0xEE6B28000000)
  {
    for(i = strlen((const char*)d40)-1; i >= 0; i--)
    {
      for(j = 0; j < 40; j++)
      {
        if(d40[i]==b40[j])
        {
          *dst = *dst *40 + j;
          break;
        }
      }
    }
  }

  if (*src < 0xEE6B28000000)
  {
    for(i = strlen((const char*)s40)-1; i >= 0; i--)
    {
      for(j = 0; j < 40; j++)
      {
        if(s40[i]==b40[j])
        {
          *src = *src * 40 + j;
          break;
        }
      }
    }
  }

}