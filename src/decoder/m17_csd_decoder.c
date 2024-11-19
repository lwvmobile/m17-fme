/*-------------------------------------------------------------------------------
 * m17_csd_decoder.c
 * M17 Project - Callsign Data Decoder
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void decode_callsign_data(Super * super, unsigned long long int dst, unsigned long long int src)
{
  //quell defined but not used warnings from m17.h
  stfu ();

  int i;
  char c;
  char dst_csd[9]; memset (dst_csd, 0, 9*sizeof(char));
  char src_csd[9]; memset (src_csd, 0, 9*sizeof(char));

  if (dst == 0xFFFFFFFFFFFF)
  {
    fprintf (stderr, " DST: BROADCAST");
    sprintf (super->m17d.dst_csd_str, "BROADCAST");
  }
  else if (dst == 0)
  {
    fprintf (stderr, " DST: RESERVED %012llX", dst);
    sprintf (super->m17d.dst_csd_str, "RESERVED ");
  }

  else if (dst >= 0xEE6B28000000)
  {
    fprintf (stderr, " DST: RESERVED %012llX", dst);
    sprintf (super->m17d.dst_csd_str, "RES: %012llX", dst); //can't fit the whole thing in here
  }
    
  else
  {
    fprintf (stderr, " DST: ");
    for (i = 0; i < 9; i++)
    {
      c = b40[dst % 40];
      dst_csd[i] = c;
      fprintf (stderr, "%c", c);
      dst = dst / 40;
    }

    //assign completed CSD to a more useful string instead
    sprintf (super->m17d.dst_csd_str, "%c%c%c%c%c%c%c%c%c", 
    dst_csd[0], dst_csd[1], dst_csd[2], dst_csd[3], 
    dst_csd[4], dst_csd[5], dst_csd[6], dst_csd[7], dst_csd[8]);

    //debug
    // fprintf (stderr, "DT: %s", super->m17d.dst_csd_str);
  }

  if (src == 0xFFFFFFFFFFFF) 
    fprintf (stderr, " SRC:  UNKNOWN FFFFFFFFFFFF");
  else if (src == 0)
    fprintf (stderr, " SRC: RESERVED %012llX", src);
  else if (src >= 0xEE6B28000000)
    fprintf (stderr, " SRC: RESERVED %012llX", src);
  else
  {
    fprintf (stderr, " SRC: ");
    for (i = 0; i < 9; i++)
    {
      c = b40[src % 40];
      src_csd[i] = c;
      fprintf (stderr, "%c", c);
      src = src / 40;
    }

    //assign completed CSD to a more useful string instead
    sprintf (super->m17d.src_csd_str, "%c%c%c%c%c%c%c%c%c", 
    src_csd[0], src_csd[1], src_csd[2], src_csd[3], 
    src_csd[4], src_csd[5], src_csd[6], src_csd[7], src_csd[8]);

    //debug
    // fprintf (stderr, "ST: %s", super->m17d.src_csd_str);
  }

  //debug
  // fprintf (stderr, " DST: %012llX SRC: %012llX", super->m17d.dst_csd_str, super->m17d.src_csd_str);

}

//version just for IP Frame SRC found in CONN, DISC etc
void decode_callsign_src(Super * super, unsigned long long int src)
{
  int i;
  char c;
  char src_csd[9]; memset (src_csd, 0, 9*sizeof(char));

  if (src == 0xFFFFFFFFFFFF) 
    fprintf (stderr, " SRC:  UNKNOWN FFFFFFFFFFFF");
  else if (src == 0)
    fprintf (stderr, " SRC: RESERVED %012llX", src);
  else if (src >= 0xEE6B28000000)
    fprintf (stderr, " SRC: RESERVED %012llX", src);
  else
  {
    fprintf (stderr, " SRC: ");
    for (i = 0; i < 9; i++)
    {
      c = b40[src % 40];
      src_csd[i] = c;
      fprintf (stderr, "%c", c);
      src = src / 40;
    }

    //assign completed CSD to a more useful string instead
    sprintf (super->m17d.src_csd_str, "%c%c%c%c%c%c%c%c%c", 
    src_csd[0], src_csd[1], src_csd[2], src_csd[3], 
    src_csd[4], src_csd[5], src_csd[6], src_csd[7], src_csd[8]);
  }

}