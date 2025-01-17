/*-------------------------------------------------------------------------------
 * m17_ipf_decoder.c
 * M17 Project - IP Frame Receiver and Decoding
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void decode_ipf (Super * super)
{

  //quell defined but not used warnings from m17.h
  stfu ();

  //Enable Ncurses Terminal
  #ifdef USE_CURSES
  if (super->opts.use_ncurses_terminal == 1)
    open_ncurses_terminal(super);
  #endif

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

  while (!exitflag)
  {

    //set current time for call history, etc
    super->demod.current_time = time(NULL);

    //if reading from socket receiver
    if (super->opts.m17_udp_sock) //double check
    {
      //NOTE: blocking issue resolved with setsockopt in udp_socket_bind

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

      //Enable Sync
      super->demod.in_sync = 1;

      //convert bytes to bits
      k = 0;
      uint8_t ip_bits[462]; memset(ip_bits, 0, sizeof(ip_bits));
      for (i = 0; i < 54; i++)
      {
        for (j = 0; j < 8; j++)
          ip_bits[k++] = (ip_frame[i] >> (7-j)) & 1;
      }

      //copy Stream ID
      uint16_t sid = (uint16_t)convert_bits_into_output(&ip_bits[32], 16);

      //copy LSF
      for (i = 0; i < 224; i++)
        super->m17d.lsf[i] = ip_bits[i+48];

      //get FN and EOT bit
      uint16_t fn = (uint16_t)convert_bits_into_output(&ip_bits[273], 15);
      uint8_t eot = ip_bits[272];

      //for scrambler seed calculation
      super->enc.scrambler_fn_d = fn;
      if (fn == 0)
        super->enc.scrambler_seed_d = super->enc.scrambler_key;

      //update IV CTR from FN
      super->m17d.meta[14] = (uint16_t)convert_bits_into_output(&ip_bits[273], 7);
      super->m17d.meta[15] = (uint16_t)convert_bits_into_output(&ip_bits[280], 8);

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
        decode_lsf_contents(super);

      //Consolodate these two
      if (super->m17d.dt == 2)
        decode_str_payload(super, payload, 2, fn%6);

      else if (super->m17d.dt == 3)
        decode_str_payload(super, payload, 3, fn%6);

      if (super->opts.payload_verbosity >= 1)
      {
        fprintf (stderr, "\n IP:");
        for (i = 0; i < 54; i++)
        {
          if ( ((i%14) == 0) && i != 0)
            fprintf (stderr, "\n    ");
          fprintf (stderr, " %02X", ip_frame[i]);
        }
        fprintf (stderr, "\n     (CRC CHK) E: %04X; C: %04X;", crc_ext, crc_cmp);
      }

      if (crc_ext != crc_cmp) fprintf (stderr, " IP CRC ERR");

      //track errors
      if (crc_ext != crc_cmp) super->error.ipf_crc_err++;

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
      decode_callsign_src(super, src);

      //reflector module user is connecting to
      fprintf (stderr, "Module: %c; ", ip_frame[10]);

      super->m17d.reflector_module = ip_frame[10];

      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < 11; i++)
          fprintf (stderr, "%02X ", ip_frame[i]);
      }

      //since there is no destination, let's write REFLECTOR into dsts
      sprintf (super->m17d.dst_csd_str, "REFLECTOR");

      //clear frame
      memset (ip_frame, 0, sizeof(ip_frame));

      super->m17d.dt = 6; //push an IP CONN to Call History
      push_call_history(super);
    }

    else if (memcmp(ip_frame, disc, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   DISC: ");
      decode_callsign_src(super, src);
      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < 10; i++)
          fprintf (stderr, "%02X ", ip_frame[i]);
      }

      //clear frame
      memset (ip_frame, 0, sizeof(ip_frame));

      //since there is no destination, let's write REFLECTOR into dsts
      sprintf (super->m17d.dst_csd_str, "REFLECTOR");

      //drop sync
      super->m17d.dt = 5; //fake for DISC message in Call History
      no_carrier_sync(super);

    }

    else if (memcmp(ip_frame, eotx, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   EOTX: ");
      decode_callsign_src(super, src);
      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < 10; i++)
          fprintf (stderr, "%02X ", ip_frame[i]);
      }

      //clear frame
      memset (ip_frame, 0, sizeof(ip_frame));

      //drop sync
      no_carrier_sync(super);

    }

    else if (memcmp(ip_frame, ping, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   PING: ");
      decode_callsign_src(super, src);
      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < 10; i++)
          fprintf (stderr, "%02X ", ip_frame[i]);
      }

    }

    else if (memcmp(ip_frame, pong, 4) == 0)
    {
      fprintf (stderr, "\n M17 IP   PONG: ");
      decode_callsign_src(super, src);
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
      uint16_t sid = (uint16_t)convert_bits_into_output(&ip_bits[32], 16);

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

      //apply keystream here if encrypted
      if (super->m17d.enc_et == 1 && super->enc.scrambler_key)
      {
        uint8_t unpacked_pkt[8000]; memset (unpacked_pkt, 0, 8000*sizeof(uint8_t)); //33*25*8 = 6600, but giving some extra space here (stack smash fix on full sized enc frame decrypt)
        unpack_byte_array_into_bit_array(ip_frame+34, unpacked_pkt, err-34-3);

        //new method
        super->enc.scrambler_seed_d = super->enc.scrambler_key; //reset seed to key value
        super->enc.scrambler_seed_d = scrambler_sequence_generator(super, 0);
        int z = 0;
        for (i = 8; i < (err*8); i++)
        {
          unpacked_pkt[i] ^= super->enc.scrambler_pn[z++];
          if (z == 128)
          {
            super->enc.scrambler_seed_d = scrambler_sequence_generator(super, 0);
            z = 0;
          }
        }

        //old method
        // for (i = 8; i < (err*8); i++)
        //   unpacked_pkt[i] ^= super->enc.scrambler_pn[i%768];


        pack_bit_array_into_byte_array(unpacked_pkt, ip_frame+34, err-34-3);
      }

      else if (super->m17d.enc_et == 2 && super->enc.aes_key_is_loaded)
      {
        int ret = err - 34 - 3;
        int klen = (ret*8)/128; //NOTE: This will fall short by % value octets
        int kmod = (ret*8)%128; //This is how many bits we are short, so we need an extra round of AES with padded block

        //debug
        // fprintf (stderr, " AES KLEN: %d; KMOD: %d;", klen, kmod);

        //NOTE: Its pretty redundant to pack and unpack here and in the crypt function,
        //but this is still quicker than writing a new function for only one use case

        uint8_t unpacked_pkt[8000]; memset (unpacked_pkt, 0, 8000*sizeof(uint8_t)); //33*25*8 = 6600, but giving some extra space here (stack smash fix on full sized enc frame decrypt)
        unpack_byte_array_into_bit_array(ip_frame+34, unpacked_pkt, ret+1); //+1 to accomodate the terminating 0x00 byte
        for (i = 0; i < klen; i++)
          aes_ctr_str_payload_crypt (super->m17d.meta, super->enc.aes_key, unpacked_pkt+(128*i)+8, super->m17d.enc_st+1);

        if (kmod != 0) //if a partial block left over to decrypt, decrypt it with the remainder of what is in unpacked_pkt (padding zero bits)
          aes_ctr_str_payload_crypt (super->m17d.meta, super->enc.aes_key, unpacked_pkt+(128*i)+8, super->m17d.enc_st+1);

        //NOTE: This will currently leave residual cipher text octets after the CRC value,
        //but that is fine since M17 has a last significant octet count value when EOT bit

        pack_bit_array_into_byte_array(unpacked_pkt, ip_frame+34, ret+1);
      }

      if (super->opts.payload_verbosity >= 1)
      {
        for (i = 0; i < err; i++)
        {
          if ( (i%25)==0)
            fprintf (stderr, "\n ");
          fprintf (stderr, "%02X ", ip_frame[i]);
        }
        fprintf (stderr, " (CRC CHK) E: %04X; C: %04X;", crc_ext, crc_cmp);
        // fprintf (stderr, "\n M17 IP   RECD: %d", err);
      }

      if (crc_ext == crc_cmp)
      {
        fprintf (stderr, "\n");
        //NOTE: Raw Data may decode with the first octet of CRC attached, depending on source
        decode_pkt_contents (super, ip_frame+34, err-34-3);
      }
        
      if (crc_ext != crc_cmp) fprintf (stderr, " IP CRC ERR");

      //track errors
      if (crc_ext != crc_cmp) super->error.ipf_crc_err++;

      //clear frame
      memset(ip_frame, 0, sizeof(ip_frame));

    }

    //refresh ncurses printer, if enabled
    #ifdef USE_CURSES
    if (super->opts.use_ncurses_terminal == 1)
      print_ncurses_terminal(super);
    #endif

    //clear frame (if not recognized format)
    memset(ip_frame, 0, sizeof(ip_frame));

  }
}