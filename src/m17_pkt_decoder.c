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
  
  int i;

  //Decode the completed packet
  uint8_t protocol = input[0];
  fprintf (stderr, " Protocol:");
  if      (protocol == 0) fprintf (stderr, " Raw;");
  else if (protocol == 1) fprintf (stderr, " AX.25;");
  else if (protocol == 2) fprintf (stderr, " APRS;");
  else if (protocol == 3) fprintf (stderr, " 6LoWPAN;");
  else if (protocol == 4) fprintf (stderr, " IPv4;");
  else if (protocol == 5) fprintf (stderr, " SMS;");
  else if (protocol == 6) fprintf (stderr, " Winlink;");
  else if (protocol == 9) fprintf (stderr, " OTA Key Delivery;");
  else if (protocol == 90)fprintf (stderr, " Meta Text Data;"); //internal format only from meta
  else if (protocol == 91)fprintf (stderr, " Meta GNSS Position Data;"); //internal format only from meta
  else if (protocol == 92)fprintf (stderr, " Meta Extended CSD;"); //internal format only from meta
  else if (protocol == 99)fprintf (stderr, " 1600 Arbitrary Data;"); //internal format only from 1600
  else fprintf (stderr, " Res: %02X;", protocol);

  //simple UTF-8 SMS Decoder
  if (protocol == 5)
  {
    fprintf (stderr, "\n SMS: ");
    for (i = 1; i < len; i++)
    {
      fprintf (stderr, "%c", input[i]);

      //add line break to keep it under 80 columns
      if ( (i%71) == 0 && i != 0)
        fprintf (stderr, "\n      ");
    }

    //make a better string out of it instead
    sprintf (super->m17d.sms, "%s", "");
    memcpy (super->m17d.sms, input+1, len);
  }
  #ifdef OTA_KEY_DELIVERY
  //OTA Key Delivery Format
  else if (protocol == 9)
  {
    //get the encryption type and subtype from the first octet
    uint8_t bits[400]; memset (bits, 0, 400*sizeof(uint8_t));
    unpack_byte_array_into_bit_array(input+2, bits, 48); //offset is +2 (easier visualization on line up)
    uint8_t  type = (input[1] >> 6) & 0x3; //enc type
    uint8_t stype = (input[1] >> 4) & 0x3; //enc sub-type
    uint8_t ssn   = (input[1] >> 0) & 0xF; //send sequence number
    fprintf (stderr, "\n Encryption Type: %d; Subtype: %d; Send Sequence Number: %d;\n", type, stype, ssn);
    if (type == 1)
    {
      super->enc.scrambler_key = (uint32_t)convert_bits_into_output(bits, 24);
      pn_sequence_generator (super);
      sprintf (super->m17d.sms, "OTAKD Scrambler Key: %X;", super->enc.scrambler_key);
    }
    if (type == 2)
    {
      //sending full sized AES key over Embedded LSF OTAKD will require 4 embedded LSF frames
      if      (ssn == 0)
        super->enc.A1 = (unsigned long long int)convert_bits_into_output(bits+00+00+00, 64);
      else if (ssn == 1)
        super->enc.A2 = (unsigned long long int)convert_bits_into_output(bits+00+00+00, 64);
      else if (ssn == 2)
        super->enc.A3 = (unsigned long long int)convert_bits_into_output(bits+00+00+00, 64);
      else if (ssn == 3)
      {
        super->enc.A4 = (unsigned long long int)convert_bits_into_output(bits+00+00+00, 64);
        sprintf (super->m17d.sms, "OTAKD AES Key: %016llX %016llX %016llX %016llX",
                 super->enc.A1, super->enc.A2, super->enc.A3, super->enc.A4);
        aes_key_loader (super);
      }
      else if (ssn == 4) //complete key over PACKET DATA or IPFrame Delivery
      {
        super->enc.A1 = (unsigned long long int)convert_bits_into_output(bits+00+00+00, 64);
        super->enc.A2 = (unsigned long long int)convert_bits_into_output(bits+64+00+00, 64);
        super->enc.A3 = (unsigned long long int)convert_bits_into_output(bits+64+64+00, 64);
        super->enc.A4 = (unsigned long long int)convert_bits_into_output(bits+64+64+64, 64);
        sprintf (super->m17d.sms, "OTAKD AES Key: %016llX %016llX %016llX %016llX",
                 super->enc.A1, super->enc.A2, super->enc.A3, super->enc.A4);
        aes_key_loader (super);
      }
    }
  }
  #endif
  //Extended Call Sign Data
  else if (protocol == 92)
  {
    unsigned long long int src  = ((unsigned long long int)input[1] << 40ULL) + ((unsigned long long int)input[2] << 32ULL) + (input[3] << 24ULL) + (input[4]  << 16ULL) + (input[5]  << 8ULL) + (input[6]  << 0ULL);
    unsigned long long int dst  = ((unsigned long long int)input[7] << 40ULL) + ((unsigned long long int)input[8] << 32ULL) + (input[9] << 24ULL) + (input[10] << 16ULL) + (input[11] << 8ULL) + (input[12] << 0ULL);
    char cf1[10]; memset (cf1, 0, 10*sizeof(char));
    char cf2[10]; memset (cf2, 0, 10*sizeof(char));
    fprintf (stderr, " CF1: "); //Originator
    for (i = 0; i < 9; i++)
    {
      char c = b40[src % 40];
      fprintf (stderr, "%c", c);
      cf1[i] = c;
      src = src / 40;
    }
    if (dst != 0) //if used
    {
      fprintf (stderr, " REF: "); //Reflector Name
      for (i = 0; i < 9; i++)
      {
        char c = b40[dst % 40];
        fprintf (stderr, "%c", c);
        cf1[i] = c;
        dst = dst / 40;
      }
    }

    //
    sprintf (super->m17d.dat, "Original SRC: %s; Reflector: %s;", cf1, cf2);

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
    if (indicators & 8) fprintf (stderr, "Bearing: %d Degrees;", bearing);

    if      (data_source == 0) fprintf (stderr, " M17 Client;");
    else if (data_source == 1) fprintf (stderr, " OpenRTX;");
    else if (data_source == 0xFF) fprintf (stderr, " Other;");
    else fprintf (stderr, " Reserved Data Source: %02X;", data_source);

    if      (station_type == 0) fprintf (stderr, " Fixed Station;");
    else if (station_type == 1) fprintf (stderr, " Mobile Station;");
    else if (station_type == 2) fprintf (stderr, " Handheld;");
    else fprintf (stderr, " Reserved Station Type: %02X;", station_type);

    //Make a condensed version of this output for Ncurses Display
    char ns[2]; char ew[2];
    if (indicators & 1) sprintf (ns, "S");
    else                sprintf (ns, "N");
    if (indicators & 2) sprintf (ew, "W");
    else                sprintf (ew, "E");

    char st[4];
    if      (station_type == 0) sprintf (st, "FS");
    else if (station_type == 1) sprintf (st, "MS");
    else if (station_type == 2) sprintf (st, "HH");
    else                        sprintf (st, "%02X;", station_type);

    //TODO: Encode some raw data as GNSS and test this
    sprintf (super->m17d.dat, " Lat: %d.%05d %s; Lon: %d.%05d %s; Alt: %d; Spd: %d; Ber: %d; St: %s;", 
      lat_deg_int, lat_deg_dec*65535, ns, lon_deg_int, lon_deg_dec*65535, ew, altitude*1500, speed, bearing, st);

  }
  //Any Other Text Based
  else if (protocol >= 90)
  {
    fprintf (stderr, " ");
    for (i = 1; i < len; i++)
      fprintf (stderr, "%c", input[i]);

    //make a better string out of it instead
    sprintf (super->m17d.arb, "%s", "");
    memcpy (super->m17d.arb, input+1, len);
  }
  //Any Other Raw Data as Hex
  else
  {
    fprintf (stderr, " ");
    for (i = 1; i < len; i++)
      fprintf (stderr, "%02X", input[i]);

    //copy out
    memset (super->m17d.raw, 0, sizeof(super->m17d.raw));
    memcpy (super->m17d.raw, input+1, len);
  }

  //quell defined but not used warnings from m17.h
  stfu ();

}