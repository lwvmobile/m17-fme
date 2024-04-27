/*-------------------------------------------------------------------------------
 * m17_pkt_decoder.c
 * Project M17 - Packet Contents Decoder
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void decode_pkt_contents(Super * super, uint8_t * input, int len)
{

  //super strct is currently unsused here, but we may want it here in the future.
  UNUSED(super);

  //quell defined but not used warnings from m17.h
  UNUSED(b40); UNUSED(m17_scramble); UNUSED(p1); UNUSED(p3); UNUSED(symbol_map); UNUSED(m17_rrc);
  UNUSED(lsf_sync_symbols); UNUSED(str_sync_symbols); UNUSED(pkt_sync_symbols); UNUSED(symbol_levels);

  //Decode the completed packet
  int i;

  uint8_t protocol = input[0];
  fprintf (stderr, " Protocol:");
  if (protocol == 0) fprintf (stderr, " Raw;");
  else if (protocol == 1) fprintf (stderr, " AX.25;");
  else if (protocol == 2) fprintf (stderr, " APRS;");
  else if (protocol == 3) fprintf (stderr, " 6LoWPAN;");
  else if (protocol == 4) fprintf (stderr, " IPv4;");
  else if (protocol == 5) fprintf (stderr, " SMS;");
  else if (protocol == 6) fprintf (stderr, " Winlink;");
  else if (protocol == 90)fprintf (stderr, " Meta Text Data;"); //internal format only from meta
  else if (protocol == 91)fprintf (stderr, " Meta GNSS Position Data;"); //internal format only from meta
  else if (protocol == 92)fprintf (stderr, " Meta Extended CSD;"); //internal format only from meta
  else if (protocol == 99)fprintf (stderr, " 1600 Arbitrary Data;"); //internal format only from 1600
  else fprintf (stderr, " Res: %02X;", protocol);

  //simple UTF-8 SMS Decoder
  if (protocol == 5)
  {
    fprintf (stderr, "\n SMS:\n      ");
    for (i = 1; i < len; i++)
    {
      fprintf (stderr, "%c", input[i]);

      //add line break to keep it under 80 columns
      if ( (i%71) == 0 && i != 0)
        fprintf (stderr, "\n      ");
    }
  }
  //Extended Call Sign Data
  else if (protocol == 92)
  {
    unsigned long long int src  = ((unsigned long long int)input[1] << 40ULL) + ((unsigned long long int)input[2] << 32ULL) + (input[3] << 24ULL) + (input[4]  << 16ULL) + (input[5]  << 8ULL) + (input[6]  << 0ULL);
    unsigned long long int dst  = ((unsigned long long int)input[7] << 40ULL) + ((unsigned long long int)input[8] << 32ULL) + (input[9] << 24ULL) + (input[10] << 16ULL) + (input[11] << 8ULL) + (input[12] << 0ULL);
    fprintf (stderr, " CF1: "); //Originator
    for (i = 0; i < 9; i++)
    {
      char c = b40[src % 40];
      fprintf (stderr, "%c", c);
      src = src / 40;
    }
    if (dst != 0) //if used
    {
      fprintf (stderr, " REF: "); //Reflector Name
      for (i = 0; i < 9; i++)
      {
        char c = b40[dst % 40];
        fprintf (stderr, "%c", c);
        dst = dst / 40;
      }
    }
  }
  //GNSS Positioning
  else if (protocol == 91)
  {
    //Decode GNSS Elements
    uint8_t  data_source  = input[1];
    uint8_t  station_type = input[2];
    uint8_t  lat_deg_int  = input[3];
    uint32_t lat_deg_dec  = (input[4] << 8) + input[5];
    uint8_t  lon_deg_int  = input[6];
    uint32_t lon_deg_dec  = (input[7] << 8) + input[8];
    uint8_t  indicators   = input[9]; //nsew, validity bits
    uint16_t altitude     = (input[10] << 8) + input[11];
    uint16_t bearing      = (input[12] << 8) + input[13];
    uint8_t  speed        = input[14];

    fprintf (stderr, "\n Latitude: %03d.%05d ", lat_deg_int, lat_deg_dec * 65535);
    if (indicators & 1) fprintf (stderr, "S;");
    else                fprintf (stderr, "N;");
    fprintf (stderr, " Longitude: %03d.%05d ", lon_deg_int, lon_deg_dec * 65535);
    if (indicators & 2) fprintf (stderr, "W;");
    else                fprintf (stderr, "E;");
    if (indicators & 4) fprintf (stderr, "Altitude: %d;", altitude + 1500);
    if (indicators & 8) fprintf (stderr, "Speed: %d MPH;", speed);
    if (indicators & 0x10) fprintf (stderr, "Bearing: %d Degrees;", bearing);

    if      (data_source == 0) fprintf (stderr, " M17 Client;");
    else if (data_source == 1) fprintf (stderr, " OpenRTX;");
    else if (data_source == 0xFF) fprintf (stderr, " Other;");
    else fprintf (stderr, " Reserved Data Source: %02X;", data_source);

    if      (station_type == 0) fprintf (stderr, " Fixed Station;");
    else if (station_type == 1) fprintf (stderr, " Mobile Station;");
    else if (station_type == 2) fprintf (stderr, " Handheld;");
    else fprintf (stderr, " Reserved Station Type: %02X;", station_type);

  }
  //Any Other Text Based
  else if (protocol >= 90)
  {
    fprintf (stderr, " ");
    for (i = 1; i < len; i++)
      fprintf (stderr, "%c", input[i]);
  }
  //Any Other Raw Data as Hex
  else
  {
    fprintf (stderr, " ");
    for (i = 1; i < len; i++)
      fprintf (stderr, "%02X", input[i]);
  }

}