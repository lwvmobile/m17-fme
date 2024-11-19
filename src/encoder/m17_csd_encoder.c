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

  //TODO: Add any known 'reserved addressed denoted by a #
  //when developers/users declare any in future works
  //for now, if a # is used, and it isn't a broadcast dst,
  //we will simply set the src or dst to a reserved address value

  //Source
  if (super->m17e.srcs[0] == '#')
    *src = 0xEE6B28000001; 

  else if (super->m17e.srcs[0] != 0)
    sprintf (s40, "%s", super->m17e.srcs);

  //Destination
  if (strcmp (super->m17e.dsts, "#BROADCAST") == 0)
    *dst = 0xFFFFFFFFFFFF;

  else if (strcmp (super->m17e.dsts, "#ALL") == 0)
    *dst = 0xFFFFFFFFFFFF;

  else if (strcmp (super->m17e.dsts, "BROADCAST") == 0)
    *dst = 0xFFFFFFFFFFFF;

  else if (strcmp (super->m17e.dsts, "ALL") == 0)
    *dst = 0xFFFFFFFFFFFF;

  else if (super->m17e.dsts[0] == '#')
    *dst = 0xEE6B28000001;

  else if (super->m17e.dsts[0] != 0)
    sprintf (d40, "%s", super->m17e.dsts);

  //if dst and src not a reserved address, encode them now
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