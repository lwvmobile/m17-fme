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
  fprintf (stderr, " Protocol:");
  if      (protocol == 0x00) fprintf (stderr, " Raw;");
  else if (protocol == 0x01) fprintf (stderr, " AX.25;");
  else if (protocol == 0x02) fprintf (stderr, " APRS;");
  else if (protocol == 0x03) fprintf (stderr, " 6LoWPAN;");
  else if (protocol == 0x04) fprintf (stderr, " IPv4;");
  else if (protocol == 0x05) fprintf (stderr, " SMS;");
  else if (protocol == 0x06) fprintf (stderr, " Winlink;");
  else if (protocol == 0x09) fprintf (stderr, " OTA Key Delivery;"); //m17-fme non standard packet data
  else if (protocol == 0x80) fprintf (stderr, " Meta Text Data;"); //internal format only from meta
  else if (protocol == 0x81) fprintf (stderr, " Meta GNSS Position Data;"); //internal format only from meta
  else if (protocol == 0x82) fprintf (stderr, " Meta Extended CSD;"); //internal format only from meta
  else if (protocol == 0x89) fprintf (stderr, " 1600 Arbitrary Data;"); //internal format only from 1600
  else                       fprintf (stderr, " Res/Unk: %02X;", protocol); //any received but unknown protocol type

  //check for encryption, if encrypted but no decryption key loaded, then skip decode and report as encrypted
  if (protocol == 0x09) {} //allow OTAKD passthrough (not encrypted ever)
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
    fprintf (stderr, "\n SMS: ");
    for (i = 1; i < len; i++)
    {
      fprintf (stderr, "%c", input[i]);

      //add line break to keep it under 80 columns
      // if ( (i%71) == 0 && i != 0)
      //   fprintf (stderr, "\n      ");
    }

    //make a better string out of it instead
    memset (super->m17d.sms, 0, 800*sizeof(char));
    sprintf (super->m17d.sms, "%s", "");
    memcpy (super->m17d.sms, input+1, len);

    //if eastern langauge encoded as UTF-8 (i.e., Japanese with 3-byte encoding, will need to manually terminate string, 
    //depending on 'len' value and number of characters, print or ncurses display may have stale or trailing garbage
    //glyphs especially if following an OTAKD message or SMS where the preceeding SMS text is longer than the new SMS

    //m17-fme -o pulserf -2 -P -S 日本語
    super->m17d.sms[len-1] = '\0';
    super->m17d.sms[len+0] = '\0';
    super->m17d.sms[len+1] = '\0';
    super->m17d.sms[len+2] = '\0';

    //send SMS Text Message to event_log_writer
    event_log_writer (super, super->m17d.sms, protocol);

  }
  
  //OTA Key Delivery Format
  else if (protocol == 0x09)
  {
    //get the encryption type and subtype from the first octet
    uint8_t bits[400]; memset (bits, 0, 400*sizeof(uint8_t));
    unpack_byte_array_into_bit_array(input+2, bits, 48); //offset is +2 (easier visualization on line up)
    uint8_t  type = (input[1] >> 6) & 0x3; //enc type
    uint8_t stype = (input[1] >> 4) & 0x3; //enc sub-type
    uint8_t ssn   = (input[1] >> 0) & 0xF; //send sequence number
    if (type != 0x03) fprintf (stderr, "\nEncryption Type: %d; Subtype: %d; Send Sequence Number: %d;", type, stype, ssn);
    if (type == 0x01)
    {
      super->enc.scrambler_key = (uint32_t)convert_bits_into_output(bits, 24);
      fprintf (stderr, "\n");
      scrambler_key_init(super, 0);
      super->enc.scrambler_seed_d = super->enc.scrambler_key; //NOTE: This will constantly retrigger the SEED calc in pyl_decoder
      sprintf (super->m17d.sms, "OTAKD Scrambler Key: %X;", super->enc.scrambler_key);

      //send OTAKD Scrambler to event_log_writer
      // event_log_writer (super, super->m17d.sms, protocol); //disabled, otherwise, spams the event log
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
        sprintf (super->m17d.sms, "OTAKD AES Key Delivery");
        fprintf (stderr, "\n");
        aes_key_loader (super);
      }
      else if (ssn == 4) //complete key over PACKET DATA or IP Frame Delivery
      {
        super->enc.A1 = (unsigned long long int)convert_bits_into_output(bits+00+00+00, 64);
        super->enc.A2 = (unsigned long long int)convert_bits_into_output(bits+64+00+00, 64);
        super->enc.A3 = (unsigned long long int)convert_bits_into_output(bits+64+64+00, 64);
        super->enc.A4 = (unsigned long long int)convert_bits_into_output(bits+64+64+64, 64);
        fprintf (stderr, "\n");
        sprintf (super->m17d.sms, "OTAKD AES Key Delivery");
        aes_key_loader (super);

        //send OTAKD AES to event_log_writer
        event_log_writer (super, super->m17d.sms, protocol);
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
      sprintf (super->m17d.sms, "OTASK Signature Public Key Delivery;");

      //send OTASK to event_log_writer
      event_log_writer (super, super->m17d.sms, protocol);

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
      fprintf (stderr, " REF: "); //Reflector Name
      for (i = 0; i < 9; i++)
      {
        char c = b40[dst % 40];
        fprintf (stderr, "%c", c);
        cf2[i] = c;
        dst = dst / 40;
      }
    }

    //
    sprintf (super->m17d.dat, "Extended CSD - Original SRC: %s; Reflector: %s;", cf1, cf2);
    
    //send Extended CSD to event_log_writer
    event_log_writer (super, super->m17d.dat, protocol);

  }
  //GNSS Positioning
  else if (protocol == 0x81)
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

    //TESTING: Fake GNSS Location based on specific coordinates (30.32411315644497, -82.96444902305485)
    // input[1] = 0x69; //reserved source M17-FME
    // input[2] = 0x00; //fixed station
    // input[3] = 0x1E; //30 dec N
    // input[4] = 0x13; //324113156÷65535 (rounded) ~= 0x1351
    // input[5] = 0x51;
    // input[6] = 0x52; //82 decimal W (negative)
    // input[7] = 0x39; //964449023÷65535 (rounded) ~= 0x397C
    // input[8] = 0x7C;
    // input[9] = 0x0A;  //0000 1010 (NW, invalid altitude, and valid bearing and speed)
    // input[10] = 0x00; //altitude just using 0
    // input[11] = 0x00; //altitude just using 0
    // input[12] = 0x00; //90 degrees (due west)
    // input[13] = 0x5A; //90 degrees (due west)
    // input[14] = 0x45; //69 MPH

    //User Input: -Z 0169001E135152397C0A0000005A45 (Meta)
    //User Input: -R 8169001E135152397C0A0000005A45 (Packet)

    fprintf (stderr, "\n Latitude: %02d.%05d ", lat_deg_int, lat_deg_dec * 65535);
    if (indicators & 1) fprintf (stderr, "S;");
    else                fprintf (stderr, "N;");
    fprintf (stderr, " Longitude: %03d.%05d ", lon_deg_int, lon_deg_dec * 65535);
    if (indicators & 2) fprintf (stderr, "W;");
    else                fprintf (stderr, "E;");
    if (indicators & 4) fprintf (stderr, " Altitude: %d;", altitude + 1500);
    if (indicators & 8) fprintf (stderr, " Speed: %d MPH;", speed);
    if (indicators & 8) fprintf (stderr, " Bearing: %d Degrees;", bearing);

    if      (data_source == 0) fprintf (stderr, " M17 Client;");
    else if (data_source == 1) fprintf (stderr, " OpenRTX;");
    else if (data_source == 0x69) fprintf (stderr, " FME Data Source;");
    else if (data_source == 0xFF) fprintf (stderr, " Other Data Source;");
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

    //Test with User Input: -Z 0169001E135152397C0A0000005A45
    sprintf (super->m17d.dat, "Lat: %d.%05d %s; Lon: %d.%05d %s; Alt: %d; Spd: %d; Ber: %d; St: %s;",
      lat_deg_int, lat_deg_dec*65535, ns, lon_deg_int, lon_deg_dec*65535, ew, altitude+1500, speed, bearing, st);

    //send GNSS data to event_log_writer
    event_log_writer (super, super->m17d.dat, protocol);

  }
  //META Text Message, or 1600 Arbitrary Data
  //TODO: Seperate these two so we can assemble a completed Meta Text Message properly
  else if (protocol == 0x80 || protocol == 0x89)
  {
    fprintf (stderr, " ");

    if (protocol == 0x80) //Meta
    { 
      //show Control Byte Len and Segment Values on Meta Text
      fprintf (stderr, "%d/%d; ", (input[1] >> 4), input[1] & 0xF);
      for (i = 2; i < len; i++)
        fprintf (stderr, "%c", input[i]);
    }
    else
    {
      for (i = 1; i < len; i++)
        fprintf (stderr, "%c", input[i]);
    }

    //make a better string out of it instead
    sprintf (super->m17d.arb, "%s", "");
    if (protocol == 0x80) //Meta Text with the control byte
      memcpy (super->m17d.dat, input+2, len); //skip over control byte
    else memcpy (super->m17d.arb, input+1, len);

    //send to event_log_writer
    if (protocol == 0x80)
      event_log_writer (super, super->m17d.dat, protocol);
    else event_log_writer (super, super->m17d.arb, protocol);
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