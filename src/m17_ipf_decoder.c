/*-------------------------------------------------------------------------------
 * m17_ipf_decoder.c
 * Project M17 - IP Frame Receiver and Decoding
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void decode_ipf (Super * super)
{

  //quell defined but not used warnings from m17.h
  stfu ();

  //Tweaks and Enable Ncurses Terminal
  // super->opts.dmr_stereo = 0;
  // super->opts.audio_in_type = 9; //NULL
  // if (super->opts.use_ncurses_terminal == 1)
  //   ncursesOpen(opts, state);

  //NOTE: This Internal Handling is non-blocking and keeps the connection alive
  //in the event of the other end opening and closing often (exit and restart)

  //encode with: dsd-fme -fZ -M M17:1:N0CALL:ALL:48000:1 -o m17:127.0.0.1:17000 -N 2> m17out.ans
  //decode with: dsd-fme -fU -i m17:127.0.0.1:17000 -N 2> m17ip.ans

  //NOTE: Currently, IP Frame decoding cannot be used with -o udp audio output
  //its a rare use case, but should be noted, I think udp_socket_bind does something to block that functionality

  //Bind UDP Socket
  int err = 1; //NOTE: err will tell us how many bytes were received, if successful
  super->opts.m17_udp_sock = udp_socket_bind(super->opts.m17_hostname, super->opts.m17_portno);

  int i, j, k;

  //Standard IP Framing
  uint8_t ip_frame[1000]; memset (ip_frame, 0, sizeof(ip_frame));
  uint8_t magic[4] = {0x4D, 0x31, 0x37, 0x20};
  uint8_t ackn[4]  = {0x41, 0x43, 0x4B, 0x4E};
  uint8_t nack[4]  = {0x4E, 0x41, 0x43, 0x4B};
  uint8_t conn[4]  = {0x43, 0x4F, 0x4E, 0x4E};
  uint8_t disc[4]  = {0x44, 0x49, 0x53, 0x43};
  uint8_t ping[4]  = {0x50, 0x49, 0x4E, 0x47};
  uint8_t pong[4]  = {0x50, 0x4F, 0x4E, 0x47};
  uint8_t eotx[4]  = {0x45, 0x4F, 0x54, 0x58}; //EOTX is not Standard, but going to send / receive anyways
  uint8_t mpkt[4]  = {0x4D, 0x50, 0x4B, 0x54}; //MPKT is not Standard, but I think sending PKT payloads would be viable over UDP (no reason not to)

  unsigned long long int src = 0; //source derived from CONN, DISC, EOTX, and Other Headers
  char c;

  while (!exitflag)
  {

    //if reading from socket receiver
    if (super->opts.m17_udp_sock) //double check
    {
      //NOTE: blocking issue resolved with setsockopt in udp_socket_bind

      //NOTE: Using recvfrom seems to load MSB of array first, 
      //compared to having to push samples through it like with STDIN.

      err = m17_socket_receiver(super, &ip_frame);

      //debug
      // fprintf (stderr, "ERR: %X; ", err);
    }
    else exitflag = 1;

    src = ((unsigned long long int)ip_frame[4] << 40ULL) + ((unsigned long long int)ip_frame[5] << 32ULL) + ((unsigned long long int)ip_frame[6] << 24ULL) +
          ((unsigned long long int)ip_frame[7] << 16ULL) + ((unsigned long long int)ip_frame[8] <<  8ULL) + ((unsigned long long int)ip_frame[9] <<  0ULL);

    //compare header to magic and decode IP voice frame w/ M17 magic header
    if (memcmp(ip_frame, magic, 4) == 0)
    {

      //Enable Carrier, synctype, etc
      // state->carrier = 1;
      // state->synctype = 8;

      //convert bytes to bits
      k = 0;
      uint8_t ip_bits[462]; memset(ip_bits, 0, sizeof(ip_bits));
      for (i = 0; i < 54; i++)
      {
        for (j = 0; j < 8; j++)
          ip_bits[k++] = (ip_frame[i] >> (7-j)) & 1;
      }

      //copy Stream ID
      uint16_t sid = (uint16_t)ConvertBitIntoBytes(&ip_bits[32], 16);

      //copy LSF
      for (i = 0; i < 224; i++)
        super->m17d.lsf[i] = ip_bits[i+48];

      //get FN and EOT bit
      uint16_t fn = (uint16_t)ConvertBitIntoBytes(&ip_bits[273], 15);
      uint8_t eot = ip_bits[272];

      //update IV CTR from FN
      super->m17d.meta[14] = (uint16_t)ConvertBitIntoBytes(&ip_bits[273], 7);
      super->m17d.meta[15] = (uint16_t)ConvertBitIntoBytes(&ip_bits[280], 8);

      fprintf (stderr, "\n M17 IP Stream: %04X; FN: %05d;", sid, fn);
      if (eot) fprintf (stderr, " EOT;");

      //copy payload
      uint8_t payload[128]; memset(payload, 0, sizeof(payload));
      for (i = 0; i < 128; i++)
        payload[i] = ip_bits[i+288];

      //copy received CRC
      uint16_t crc_ext = (ip_frame[52] << 8) + ip_frame[53];

      //calculate CRC on received packet
      uint16_t crc_cmp = crc16(ip_frame, 52);

      if (crc_ext == crc_cmp)
        decode_lsf_contents(super); //double check and test this

      //Consolodate these two
      if (super->m17d.dt == 2)
        decode_str_payload(super, payload, 2);

      else if (super->m17d.dt == 3)
        decode_str_payload(super, payload, 3);

      if (super->opts.payload_verbosity >= 1)
      {
        fprintf (stderr, "\n IP:");
        for (i = 0; i < 54; i++)
        {
          if ( (i%14) == 0 ) fprintf (stderr, "\n    ");
          fprintf (stderr, "[%02X]", ip_frame[i]);
        }
        fprintf (stderr, " (CRC CHK) E: %04X; C: %04X;", crc_ext, crc_cmp);
      }

      if (crc_ext != crc_cmp) fprintf (stderr, " IP CRC ERR");

      //clear frame
      memset (ip_frame, 0, sizeof(ip_frame));

    }

    //other headers from UDP IP
    else if (memcmp(ip_frame, ackn, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   ACNK: ");

      //clear frame
      memset (ip_frame, 0, sizeof(ip_frame));
    }

    else if (memcmp(ip_frame, nack, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   NACK: ");

      //clear frame
      memset (ip_frame, 0, sizeof(ip_frame));
    }

    else if (memcmp(ip_frame, conn, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   CONN: ");

      if (src == 0xFFFFFFFFFFFF) 
        fprintf (stderr, "UNKNOWN FFFFFFFFFFFF");
      else if (src == 0)
        fprintf (stderr, "RESERVED %012llx", src);
      else if (src >= 0xEE6B28000000)
        fprintf (stderr, "RESERVED %012llx", src);
      else
      {
        for (i = 0; i < 9; i++)
        {
          c = b40[src % 40];
          fprintf (stderr, "%c", c);
          src = src / 40;
        }
      }

      //reflector module user is connecting to
      fprintf (stderr, "Module: %c; ", ip_frame[10]);

      super->m17d.reflector_module = ip_frame[10];

      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < 11; i++)
          fprintf (stderr, "%02X ", ip_frame[i]);
      }

      //clear frame
      memset (ip_frame, 0, sizeof(ip_frame));
    }

    else if (memcmp(ip_frame, disc, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   DISC: ");

      if (src == 0xFFFFFFFFFFFF) 
        fprintf (stderr, "UNKNOWN FFFFFFFFFFFF");
      else if (src == 0)
        fprintf (stderr, "RESERVED %012llx", src);
      else if (src >= 0xEE6B28000000)
        fprintf (stderr, "RESERVED %012llx", src);
      else
      {
        for (i = 0; i < 9; i++)
        {
          c = b40[src % 40];
          fprintf (stderr, "%c", c);
          src = src / 40;
        }
      }

      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < 10; i++)
          fprintf (stderr, "%02X ", ip_frame[i]);
      }

      //clear frame
      memset (ip_frame, 0, sizeof(ip_frame));

      // state->carrier = 0;
      // state->synctype = -1;
    }

    else if (memcmp(ip_frame, eotx, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   EOTX: ");

      if (src == 0xFFFFFFFFFFFF) 
        fprintf (stderr, "UNKNOWN FFFFFFFFFFFF");
      else if (src == 0)
        fprintf (stderr, "RESERVED %012llx", src);
      else if (src >= 0xEE6B28000000)
        fprintf (stderr, "RESERVED %012llx", src);
      else
      {
        for (i = 0; i < 9; i++)
        {
          c = b40[src % 40];
          fprintf (stderr, "%c", c);
          src = src / 40;
        }
      }

      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < 10; i++)
          fprintf (stderr, "%02X ", ip_frame[i]);
      }

      //clear frame
      memset (ip_frame, 0, sizeof(ip_frame));

      //drop carrier and sync
      // state->carrier = 0;
      // state->synctype = -1;
    }

    else if (memcmp(ip_frame, ping, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   PING: ");

      if (src == 0xFFFFFFFFFFFF) 
        fprintf (stderr, "UNKNOWN FFFFFFFFFFFF");
      else if (src == 0)
        fprintf (stderr, "RESERVED %012llx", src);
      else if (src >= 0xEE6B28000000)
        fprintf (stderr, "RESERVED %012llx", src);
      else
      {
        for (i = 0; i < 9; i++)
        {
          c = b40[src % 40];
          fprintf (stderr, "%c", c);
          src = src / 40;
        }
      }

      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < 10; i++)
          fprintf (stderr, "%02X ", ip_frame[i]);
      }

    }

    else if (memcmp(ip_frame, pong, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   PONG: ");

      if (src == 0xFFFFFFFFFFFF) 
        fprintf (stderr, "UNKNOWN FFFFFFFFFFFF");
      else if (src == 0)
        fprintf (stderr, "RESERVED %012llx", src);
      else if (src >= 0xEE6B28000000)
        fprintf (stderr, "RESERVED %012llx", src);
      else
      {
        for (i = 0; i < 9; i++)
        {
          c = b40[src % 40];
          fprintf (stderr, "%c", c);
          src = src / 40;
        }
      }

      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < 10; i++)
          fprintf (stderr, "%02X ", ip_frame[i]);
      }
    }

    else if (memcmp(ip_frame, mpkt, 4) == 0)
    {

      //convert bytes to bits
      k = 0;
      uint8_t ip_bits[462]; memset(ip_bits, 0, sizeof(ip_bits));
      for (i = 0; i < 54; i++)
      {
        for (j = 0; j < 8; j++)
          ip_bits[k++] = (ip_frame[i] >> (7-j)) & 1;
      }

      //copy Stream ID (PKT ID)
      uint16_t sid = (uint16_t)ConvertBitIntoBytes(&ip_bits[32], 16);

      //copy LSF
      for (i = 0; i < 224; i++)
        super->m17d.lsf[i] = ip_bits[i+48];
      //copy received CRC
      uint16_t crc_ext = (ip_frame[err-2] << 8) + ip_frame[err-1];

      //calculate CRC on received packet
      uint16_t crc_cmp = crc16(ip_frame, err-2);

      fprintf (stderr, "\n M17 IP   MPKT: %04X;", sid);

      if (crc_ext == crc_cmp)
        decode_lsf_contents(super);

      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < err; i++)
        {
          if ( (i%25)==0)
            fprintf (stderr, "\n                ");
          fprintf (stderr, "%02X ", ip_frame[i]);
        }
        fprintf (stderr, " (CRC CHK) E: %04X; C: %04X;", crc_ext, crc_cmp);
        fprintf (stderr, "\n M17 IP   RECD: %d", err);
      }

      if (crc_ext == crc_cmp)
        decode_pkt_contents (super, ip_frame+34, err-34-3);
      if (crc_ext != crc_cmp) fprintf (stderr, " IP CRC ERR");

    }

    //debug
    // else if (super->opts.payload == 1)
    // {
    //   fprintf (stderr, "\n UDP:");
    //   for (i = 0; i < 54; i++)
    //   {
    //     if ( (i%14) == 0 ) fprintf (stderr, "\n    ");
    //     fprintf (stderr, "[%02X]", ip_frame[i]);
    //   }
    // }

    //refresh ncurses printer, if enabled
    // if (super->opts.use_ncurses_terminal == 1)
    //   ncursesPrinter(opts, state);

  }
}