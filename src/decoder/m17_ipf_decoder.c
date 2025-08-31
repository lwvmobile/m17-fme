/*-------------------------------------------------------------------------------
 * m17_ipf_decoder.c
 * M17 Project - IP Frame Receiver and Decoding
 *
 * LWVMOBILE
 * 2025-09 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void start_ipf (Super * super)
{

  //quell defined but not used warnings from m17.h
  stfu ();

  //Enable Ncurses Terminal
  #ifdef USE_CURSES
  if (super->opts.use_ncurses_terminal == 1)
    open_ncurses_terminal(super);
  #endif

  //Bind UDP Socket
  super->opts.m17_udp_sock = udp_socket_bind(super->opts.m17_hostname, super->opts.m17_portno);

}

void decode_ipf (Super * super, int socket)
{

  int i, j, k;
  int err = 1; //NOTE: err will tell us how many bytes were received, if successful

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
  uint8_t m17p[4]  = {0x4D, 0x31, 0x37, 0x50}; //https://github.com/M17-Project/M17_inet/tree/main Current "Standard"

  unsigned long long int src = 0; //source derived from CONN, DISC, EOTX, and Other Headers

  //set current time for call history, etc
  super->demod.current_time = time(NULL);

  //if reading from socket receiver
  if (super->opts.m17_udp_sock) //double check
  {
    //NOTE: blocking issue resolved with setsockopt in udp_socket_bind

    if (super->opts.use_m17_ipf_decoder == 1)
      err = m17_socket_receiver(super, &ip_frame);
    else if (super->opts.use_m17_duplex_mode == 1)
      err = m17_socket_receiver_duplex(socket, &ip_frame);

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

    fprintf (stderr, "\n M17 IP Stream: %04X; FN: %04X;", sid, fn);
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

    //check source of ping, if not us, send the pong reply
    unsigned long long int ip_src = 0;
    unsigned long long int src = 0;
    unsigned long long int dst = 0;
    for (i = 0; i < 6; i++)
    {
      ip_src <<= 8;
      ip_src |= ip_frame[i+4];
    }

    char d40[50] = "M17-FME  ";
    char s40[50] = "M17-FME  ";
    if (super->m17e.srcs[0] != 0)
      sprintf (s40, "%s", super->m17e.srcs);

    //Encode Callsign Data
    encode_callsign_data(super, d40, s40, &dst, &src);

    uint8_t send_pong = 0;
    if (super->opts.use_m17_duplex_mode == 1 && ip_src != src)
      send_pong = 1;

    //Reply with a PONG, if conditions satisfied
    if (send_pong == 1)
      ip_send_conn_disc_ping_pong(super, 3);

    //clear frame
    memset (ip_frame, 0, sizeof(ip_frame));

    //since there is no destination, let's write PINGED IN into dsts
    sprintf (super->m17d.dst_csd_str, "PINGED IN");

    //drop sync
    super->m17d.dt = 7; //fake for PING message in Call History
    no_carrier_sync(super);

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

    //clear frame
    memset (ip_frame, 0, sizeof(ip_frame));

    //since there is no destination, let's write REFLECTOR into dsts
    sprintf (super->m17d.dst_csd_str, "REFLECTOR");

    //drop sync
    super->m17d.dt = 8; //fake for PONG message in Call History
    no_carrier_sync(super);

  }

  else if (memcmp(ip_frame, m17p, 4) == 0)
  {

    //unpack ip frame into lsf (sans crc)
    unpack_byte_array_into_bit_array(ip_frame+4, super->m17d.lsf, 28);

    //copy received CRC on LSF Portion
    uint16_t crc_ext = (ip_frame[32] << 8) + ip_frame[33];

    //calculate CRC on LSF Portion
    uint16_t crc_cmp = crc16(ip_frame+4, 28);

    fprintf (stderr, "\n M17 IP   M17P;");

    if (crc_ext != crc_cmp) fprintf (stderr, " IP M17P LSF CRC ERR");

    //track errors
    if (crc_ext != crc_cmp) super->error.ipf_crc_err++;

    if (crc_ext == crc_cmp)
      decode_lsf_contents(super);

    //copy received CRC on Payload
    crc_ext = (ip_frame[err-2] << 8) + ip_frame[err-1];

    //calculate CRC on Payload Portion
    crc_cmp = crc16(ip_frame+34, err-2-34); //had to fix, was doing entire ip_frame

    //create and apply encryption keystream to ip_frame at this 
    //point, after magic, lsf, and protocol byte and prior to terminating byte and CRC
    if ((super->m17d.enc_et == 1 && super->enc.scrambler_key) ||
        (super->m17d.enc_et == 2 && super->enc.aes_key_is_loaded) )
    {

      int total = err - 3; //check this again after

      //sanity check (probably don't really need this)
      if (total < 0)
        err = 1;

      //debug
      // fprintf (stderr, " T: %d; ", total);

      //keystream bit and byte arrays
      uint8_t ks_bits[7680]; memset(ks_bits, 0, sizeof(ks_bits));
      uint8_t ks_bytes[960]; memset(ks_bytes, 0, sizeof(ks_bytes));

      enc_pkt_ks_creation(super, ks_bits, ks_bytes, 0);
      for (i = 35; i < total; i++)
        ip_frame[i] ^= ks_bytes[i-35];

      //reset meta (iv) after use
      memset(super->m17d.meta, 0, sizeof(super->m17e.meta));

    }

    //reset meta after use
    memset(super->m17d.meta, 0, sizeof(super->m17d.meta));

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
      decode_pkt_contents (super, ip_frame+34, err-34-3);
    }
      
    if (crc_ext != crc_cmp) fprintf (stderr, " IP M17P Payload CRC ERR");

    //track errors
    if (crc_ext != crc_cmp) super->error.ipf_crc_err++;

    //clear frame
    memset(ip_frame, 0, sizeof(ip_frame));

  }

  //anything else (unknown UDP IP Frames)
  else if (ip_frame[0] != 0)
  {
    if (err > 0)
    {
      fprintf (stderr, "\n Unknown IPF: ");
      for (int i = 0; i < err; i++)
        fprintf (stderr, "%02X ", ip_frame[i]);
    }

    //clear frame
    memset (ip_frame, 0, sizeof(ip_frame));

    //drop sync
    no_carrier_sync(super);

  }

  //refresh ncurses printer, if enabled
  #ifdef USE_CURSES
  if (super->opts.use_ncurses_terminal == 1)
    print_ncurses_terminal(super);
  #endif

  //clear frame (if not recognized format)
  memset(ip_frame, 0, sizeof(ip_frame));

}