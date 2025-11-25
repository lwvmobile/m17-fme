/*-------------------------------------------------------------------------------
 * m17_pkt_decoder.c
 * M17 Project - Packet Contents Decoder
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void decode_pkt_contents(Super * super, uint8_t * input, int len)
{
  
  int i;
  char event_string[2400];
  sprintf (event_string, "%s", " ");

  //Decode the completed packet
  uint8_t protocol = input[0];
  super->m17d.packet_protocol = protocol; //store for call history text message display
  // fprintf (stderr, " Protocol:");
  if      (protocol == 0x00) fprintf (stderr, " Raw;");
  else if (protocol == 0x01) fprintf (stderr, " AX.25;");
  else if (protocol == 0x02) fprintf (stderr, " APRS;");
  else if (protocol == 0x03) fprintf (stderr, " 6LoWPAN;");
  else if (protocol == 0x04) fprintf (stderr, " IPv4;");
  else if (protocol == 0x05) fprintf (stderr, " SMS;");
  else if (protocol == 0x06) fprintf (stderr, " Winlink;");
  else if (protocol == 0x07) fprintf (stderr, " TLE;");
  else if (protocol == 0x69) fprintf (stderr, " OTA Key Delivery;"); //m17-fme non standard packet data
  else if (protocol == 0x80) fprintf (stderr, " Meta Text Data V2;"); //internal format only from meta
  else if (protocol == 0x81) fprintf (stderr, " Meta GNSS Position Data;"); //internal format only from meta
  else if (protocol == 0x82) fprintf (stderr, " Meta Extended CSD;"); //internal format only from meta
  else if (protocol == 0x83) fprintf (stderr, " Meta Text Data V3;"); //internal format only from meta
  else if (protocol == 0x91) fprintf (stderr, " PDU GNSS Position Data;"); //PDU Version of GNSS
  else if (protocol == 0x99) fprintf (stderr, " 1600 Arbitrary Data;"); //internal format only from 1600
  else                       fprintf (stderr, " Res/Unk: %02X;", protocol); //any received but unknown protocol type

  //check for encryption, if encrypted but no decryption key loaded, then skip decode and report as encrypted
  if (protocol == 0x69) {} //allow OTAKD passthrough (not encrypted ever)
  else if (protocol >= 0x80 && protocol <= 0x83) {} //allow META passthrough (not encrypted ever)
  else if (super->m17d.enc_et == 2 && super->enc.aes_key_is_loaded == 0)
  {
    fprintf (stderr, " *Encrypted*");
    sprintf (event_string,    "Encrypted Packet Data");
    sprintf (super->m17d.sms, "Encrypted Packet Data");
    event_log_writer (super, event_string, protocol);
    goto PKT_END;
  }
  else if (super->m17d.enc_et == 1 && super->enc.scrambler_key == 0)
  {
    fprintf (stderr, " *Encrypted*");
    sprintf (event_string,    "Encrypted Packet Data");
    sprintf (super->m17d.sms, "Encrypted Packet Data");
    event_log_writer (super, event_string, protocol);
    goto PKT_END;
  }

  //simple UTF-8 SMS Decoder
  if (protocol == 0x05)
  {
    fprintf (stderr, " Text: ");
    for (i = 1; i < len; i++)
      fprintf (stderr, "%c", input[i]);

    //make a better string out of it instead
    memset (super->m17d.sms, 0, 825*sizeof(char));
    sprintf (super->m17d.sms, "%s", "");
    //switch from memcpy to strncpy, it'll also terminate the string
    strncpy (super->m17d.sms, (const char *)input+1, len);

    //send SMS Text Message to event_log_writer
    event_log_writer (super, super->m17d.sms, protocol);

  }

  //TLE UTF-8 Text Decoder
  else if (protocol == 0x07)
  {
    //print first to console, preserving formatting
    fprintf (stderr, "\n");
    for (i = 1; i < len; i++)
      fprintf (stderr, "%c", input[i]);

    //scan input, replace end of line and line breaks
    for (i = 1; i < len; i++)
    {
      if (input[i] == 0x0D) //end of line causing issues in ncurses terminal
        input[i] = ' ';
      else if (input[i] == 0x0A) // \n linebreak
        input[i] = ' ';
    }

    //make a better string out of it instead
    memset (super->m17d.sms, 0, 825*sizeof(char));
    sprintf (super->m17d.sms, "%s", "");
    //switch from memcpy to strncpy, it'll also terminate the string
    strncpy (super->m17d.sms, (const char *)input+1, len);

    //send TLE Text Message to event_log_writer
    event_log_writer (super, super->m17d.sms, protocol);

  }
  
  //OTA Key Delivery Format
  else if (protocol == 0x69)
  {
    //get the encryption type and subtype from the first octet
    uint8_t bits[400]; memset (bits, 0, 400*sizeof(uint8_t));
    char otakd_pkt[825]; memset (otakd_pkt, 0, 825*sizeof(char));
    unpack_byte_array_into_bit_array(input+2, bits, 48); //offset is +2 (easier visualization on line up)
    uint8_t  type = (input[1] >> 6) & 0x3; //enc type
    uint8_t stype = (input[1] >> 4) & 0x3; //enc sub-type
    uint8_t ssn   = (input[1] >> 0) & 0xF; //send sequence number
    if (type != 0x03) fprintf (stderr, "\nEncryption Type: %d; Subtype: %d; Send Sequence Number: %d;", type, stype, ssn);
    if (type == 0x01)
    {
      super->enc.scrambler_key = (uint32_t)convert_bits_into_output(bits, 24);
      sprintf (otakd_pkt, "OTAKD Scrambler Key: %X;", super->enc.scrambler_key);

      //check to see if we haven't already decoded this so we don't spam the event log writer and constantly reset the seed
      if (strcmp(otakd_pkt, super->m17d.sms) != 0)
      {
        super->enc.scrambler_subtype_d = stype;
        fprintf (stderr, "\n");
        scrambler_key_init(super, 0);
        
        sprintf (super->m17d.sms, "OTAKD Scrambler Key: %X;", super->enc.scrambler_key);
        
        //send OTAKD Scrambler to event_log_writer
        event_log_writer (super, super->m17d.sms, protocol);
      }
    }
    else if (type == 0x02)
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
        fprintf (stderr, "\n");
        aes_key_loader (super);
        sprintf (otakd_pkt, "OTAKD AES Key Delivery");

        //check to see if we haven't already decoded this so we don't spam the event log writer 
        if (strcmp(otakd_pkt, super->m17d.sms) != 0)
        {
          sprintf (super->m17d.sms, "OTAKD AES Key Delivery");

          //send OTAKD AES to event_log_writer
          event_log_writer (super, super->m17d.sms, protocol);
        }
      }
      else if (ssn == 4) //complete key over PACKET DATA or IP Frame Delivery
      {
        super->enc.A1 = (unsigned long long int)convert_bits_into_output(bits+00+00+00, 64);
        super->enc.A2 = (unsigned long long int)convert_bits_into_output(bits+64+00+00, 64);
        super->enc.A3 = (unsigned long long int)convert_bits_into_output(bits+64+64+00, 64);
        super->enc.A4 = (unsigned long long int)convert_bits_into_output(bits+64+64+64, 64);
        fprintf (stderr, "\n");
        aes_key_loader (super);
        sprintf (otakd_pkt, "OTAKD AES Key Delivery");

        //check to see if we haven't already decoded this so we don't spam the event log writer 
        if (strcmp(otakd_pkt, super->m17d.sms) != 0)
        {
          sprintf (super->m17d.sms, "OTAKD AES Key Delivery");

          //send OTAKD AES to event_log_writer
          event_log_writer (super, super->m17d.sms, protocol);
        }
      }
    }
    else if (type == 0x03)
    {
      memcpy (super->m17d.ecdsa.public_key, input+2, 64);
      super->m17d.ecdsa.keys_loaded = 1;
      fprintf (stderr, "\nOTASK Signature Public Key Delivery;");
      fprintf (stderr, "\nPub Key:");
      for (int j = 0; j < 64; j++)
      {
        if (j == 16 || j == 32 || j == 48)
          fprintf (stderr, "\n        ");
        fprintf (stderr, " %02X", super->m17d.ecdsa.public_key[j]);
      }

      sprintf (otakd_pkt, "OTASK Signature Public Key Delivery;");
      if (strcmp(otakd_pkt, super->m17d.sms) != 0)
      {
        sprintf (super->m17d.sms, "OTASK Signature Public Key Delivery;");

        //send OTASK to event_log_writer
        event_log_writer (super, super->m17d.sms, protocol);
      }

    }
 
  }
  
  //Extended Call Sign Data
  else if (protocol == 0x82)
  {

    //NOTE: If doing a shift addition like this, make sure ALL values have (unsigned long long int) in front of it, not just the ones that 'needed' it
    unsigned long long int src  = ((unsigned long long int)input[1] << 40ULL) + ((unsigned long long int)input[2] << 32ULL) + ((unsigned long long int)input[3] << 24ULL) + ((unsigned long long int)input[4]  << 16ULL) + ((unsigned long long int)input[5]  << 8ULL) + ((unsigned long long int)input[6]  << 0ULL);
    unsigned long long int dst  = ((unsigned long long int)input[7] << 40ULL) + ((unsigned long long int)input[8] << 32ULL) + ((unsigned long long int)input[9] << 24ULL) + ((unsigned long long int)input[10] << 16ULL) + ((unsigned long long int)input[11] << 8ULL) + ((unsigned long long int)input[12] << 0ULL);
    char cf1[10]; memset (cf1, 0, 10*sizeof(char));
    char cf2[10]; memset (cf2, 0, 10*sizeof(char));

    //debug input
    // for (i = 1; i < 15; i++)
    //   fprintf (stderr, " %02X,", input[i]);

    //debug src/dst values
    // fprintf (stderr, " SRC: %012llX; DST: %012llX; ", src, dst );

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
      fprintf (stderr, " CF2: "); //Reflector Name
      for (i = 0; i < 9; i++)
      {
        char c = b40[dst % 40];
        fprintf (stderr, "%c", c);
        cf2[i] = c;
        dst = dst / 40;
      }
    }

    //check for optional cf2
    if (cf2[0] != 0)
      sprintf (super->m17d.dat, "Extended CSD - CF1: %s; CF2: %s;", cf1, cf2);
    else sprintf (super->m17d.dat, "Extended CSD - CF1: %s; ", cf1);
    
    //send Extended CSD to event_log_writer
    // event_log_writer (super, super->m17d.dat, protocol);

  }

  //GNSS Positioning (version 2.0 spec)
  else if (protocol == 0x81 || protocol == 0x91)
  {
    //Decode GNSS Elements
    uint8_t  data_source  = (input[1] >> 4);
    uint8_t  station_type = (input[1] & 0xF);
    uint8_t  validity     = (input[2] >> 4);
    uint8_t  radius       = (input[2] >> 1) & 0x7;
    uint16_t bearing      = ((input[2] & 0x1) << 8) + input[3];
    uint32_t latitude     = (input[4] << 16) + (input[5] << 8) + input[6];
    uint32_t longitude    = (input[7] << 16) + (input[8] << 8) + input[9];
    uint16_t altitude     = (input[10] << 8) + input[11];
    uint16_t speed        = (input[12] << 4) + (input[13] >> 4);
    uint16_t reserved     = ((input[13] & 0xF) << 8) + input[14];

    //signed-ness and two's complement (needs to be tested and verified)
    double lat_sign = +1.0;
    if (latitude & 0x800000)
    {
      if (latitude > 0x800000)
      {
        latitude &= 0x7FFFFF;
        latitude = 0x800000 - latitude;
      }

      lat_sign = -1.0f;
    }

    double lon_sign = 1.0f;
    if (longitude & 0x800000)
    {
      if (longitude > 0x800000)
      {
        longitude &= 0x7FFFFF;
        longitude = 0x800000 - longitude;
      }

      lon_sign = -1.0f;
    }

    //encoding calculation
    // double lat_float = ((double)latitude / 90.0f)  * 8388607.0f * lat_sign;
    // double lon_float = ((double)longitude / 180.0f) * 8388607.0f * lon_sign;

    //decoding calculation
    double lat_float = ((double)latitude * 90.0f)  / 8388607.0f * lat_sign;
    double lon_float = ((double)longitude * 180.0f) / 8388607.0f * lon_sign;

    float radius_float = powf(2.0f, radius);
    float speed_float = ((float)speed * 0.5f);
    float altitude_float = ((float)altitude * 0.5f) - 500.0f;

    char deg_glyph[4];
    sprintf (deg_glyph, "%s", "°");

    //TESTING: Fake GNSS Location based on specific coordinates (30.324104,-82.964468)
    // input[1] = 0xF0; //reserved source and fixed station
    // input[2] = 0xF2; //validity all, radius 1 (2.0 meters?), bearing 180 (1 MSB)
    // input[3] = 0xB4; //bearing 180 (8 LSB)

    // input[4] = 0x2B; //Latitude (two's compliment, positive sign)
    // input[5] = 0x20;
    // input[6] = 0xAB;

    // input[7] = 0xC5; //Longitude (two's compliment, negative sign)
    // input[8] = 0x00;
    // input[9] = 0xC8;

    // input[10] = 0x04; //altitude 30 meters (decimal 1060)
    // input[11] = 0x24; //altitude 30 meters (decimal 1060)

    // input[12] = 0x06; //100 km/h speed (8 MSB)
    // input[13] = 0x40; //100 km/h speed (4 MSB) + reserved 0 (4 MSB)
    // input[14] = 0x00; //reserved 0 (8 LSB)

    //User Input: -Z 01F0F2B42B20ABC500C80424064000 (Meta)
    //User Input: -R 91F0F2B42B20ABC500C80424064000 (Packet)

    if (validity & 0x8)
      fprintf (stderr, "\n GNSS: (%f, %f);", lat_float, lon_float);
    else fprintf (stderr, "\n POS Not Valid;");

    if (validity & 0x4)
      fprintf (stderr, " Altitude: %.1f m;", altitude_float);

    if (validity & 0x2)
    {
      fprintf (stderr, " Speed: %.1f km/h;", speed_float);
      fprintf (stderr, " Bearing: %d%s;", bearing, deg_glyph);
    }

    if (validity & 0x1)
      fprintf (stderr, "\n       Radius: %.1f;", radius_float);

    if (reserved)
      fprintf (stderr, " Reserved: %03X;", reserved);

    if      (data_source == 0) fprintf (stderr, " M17 Client;");
    else if (data_source == 1) fprintf (stderr, " OpenRTX;");
    else  fprintf (stderr, " Other Data Source: %0X;", data_source);

    if      (station_type == 0) fprintf (stderr, " Fixed Station;");
    else if (station_type == 1) fprintf (stderr, " Mobile Station;");
    else if (station_type == 2) fprintf (stderr, " Handheld;");
    else fprintf (stderr, " Reserved Station Type: %0X;", station_type);

    char st[4];
    if      (station_type == 0) sprintf (st, "FS");
    else if (station_type == 1) sprintf (st, "MS");
    else if (station_type == 2) sprintf (st, "HH");
    else                        sprintf (st, "%02X", station_type);

    //Test with User Input: -Z 0169001E135152397C0A0000005A45
    sprintf (super->m17d.dat, "(%f, %f); Alt: %.1f; Spd: %.1f; Ber: %d; St: %s;",
      lat_float, lon_float, altitude_float, speed_float, bearing, st);

    //now it is okay to change the protocol to 0x81
    if (protocol == 0x91) //PDU variant of GNSS
      protocol = 0x81;

    //send GNSS data to event_log_writer
    event_log_writer (super, super->m17d.dat, protocol);

  }

  //Meta Text Messages Version 2.0 (4-segment with bitmapping)
  else if (protocol == 0x80)
  {

    uint8_t bitmap_len = (input[1] >> 4);
    uint8_t bitmap_segment = input[1] & 0xF;
    uint8_t segment_len = 1;
    uint8_t segment_num = 1;

    //convert bitmap to actual values
    if (bitmap_len == 0x1)
      segment_len = 1;
    else if (bitmap_len == 0x3)
      segment_len = 2;
    else if (bitmap_len == 0x7)
      segment_len = 3;
    else if (bitmap_len == 0xF)
      segment_len = 4;
    else segment_len = 1; //if none of these, then treat this like a single segment w/ len 1

    //convert bitmap to actual values
    if (bitmap_segment == 0x1)
      segment_num = 1;
    else if (bitmap_segment == 0x2)
      segment_num = 2;
    else if (bitmap_segment == 0x4)
      segment_num = 3;
    else if (bitmap_segment == 0x8)
      segment_num = 4;
    else segment_num = 1; //if none of these, then treat this like a single segment w/ len 1

    //show Control Byte Len and Segment Values on Meta Text
    fprintf (stderr, " %d/%d; ", segment_num, segment_len);
    if (super->opts.payload_verbosity > 0)
    {
      for (i = 2; i < len; i++)
        fprintf (stderr, "%c", input[i]);
    }

    //copy current segment into .dat
    int ptr = (segment_num-1)*13;
    memcpy (super->m17d.sms+ptr, input+2, 13*sizeof(char));

    //NOTE: there is no checkdown to see if all segments have arrived or not
    //terminate the string on completion, dump completed string
    if (segment_len == segment_num)
    {
      super->m17d.sms[ptr+13] = 0;
      fprintf (stderr, "\n Complete Meta Text: %s", super->m17d.sms);
    }

    //send to event_log_writer
    if (segment_len == segment_num)
      event_log_writer (super, super->m17d.sms, protocol);

  }

  //Meta Text Messages Version 3.0 (15-segment sequential)
  else if (protocol == 0x83)
  {

    uint8_t segment_len = (input[1] >> 4) & 0xF;
    uint8_t segment_num = (input[1] >> 0) & 0xF;

    //show Control Byte Len and Segment Values on Meta Text
    fprintf (stderr, " %d/%d; ", segment_num, segment_len);
    if (super->opts.payload_verbosity > 0)
    {
      for (i = 2; i < len; i++)
        fprintf (stderr, "%c", input[i]);
    }

    //copy current segment into .dat
    int ptr = (segment_num-1)*13;
    memcpy (super->m17d.sms+ptr, input+2, 13*sizeof(char));

    //NOTE: there is no checkdown to see if all segments have arrived or not
    //terminate the string on completion, dump completed string
    if (segment_len == segment_num)
    {
      super->m17d.sms[ptr+13] = 0;
      fprintf (stderr, "\n Complete Meta Text: %s", super->m17d.sms);
    }

    //send to event_log_writer
    if (segment_len == segment_num)
      event_log_writer (super, super->m17d.sms, protocol);

  }

  //1600 Arbitrary Data as ASCII Text String
  else if (protocol == 0x99)
  {
    //TODO: A better checkdown than this
    uint8_t is_ascii = 1;
    for (i = 1; i < len; i++)
    {
      if (input[i] != 0 && (input[i] < 0x20 || input[i] > 0x7F))
      {
        is_ascii = 0;
        break;
      }
    }
    
    sprintf (super->m17d.arb, "%s", "");

    if (is_ascii == 1)
    {

      fprintf (stderr, " ");
      for (i = 1; i < len; i++)
        fprintf (stderr, "%c", input[i]);

      memcpy (super->m17d.arb, input+1, len);
      super->m17d.arb[len] = '\0'; //terminate string
    }
    else
    {
      fprintf (stderr, " Unknown Format;");
      sprintf (super->m17d.arb, "%s", "Unknown Arbitrary Data Format;");
    }

    event_log_writer (super, super->m17d.arb, protocol);
  }

  //Any Other Raw or Unknown Data Protocol as Hex
  else
  {
    fprintf (stderr, " ");
    for (i = 1; i <= len; i++)
      fprintf (stderr, "%02X", input[i]);

    //copy out
    memset (super->m17d.raw, 0, sizeof(super->m17d.raw));
    memcpy (super->m17d.raw, input+1, len);

    //send RAW data (as string) to event_log_writer
    
    char in[3];
    for (i = 1; i <= len; i++)
    {
      sprintf (in, "%02X", input[i]);
      strcat (event_string, in);
    }
    event_log_writer (super, event_string, protocol);
  }

  //quell defined but not used warnings from m17.h
  stfu ();

  PKT_END: ; //do nothing

}