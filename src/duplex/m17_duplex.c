/*-------------------------------------------------------------------------------
* m17_duplex.c
* M17 Project - Duplex Mode Operations
*
* LWVMOBILE
* 2024-06 M17 Project - Florida Man Edition
*-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

static int m17_udp_socket_duplex;
int samp_num = 1920*34; //discarded LSF Frame + up to 33 packet frames, prevent loopback decoding in game

//detached ip frame conn and disc frame sending on start and exit
void ip_send_conn_disc (Super * super, int cd)
{
  int i, j;

  //src string and value
  unsigned long long int src = 0;
  char s40[50] = "M17-FME  ";
  if (super->m17e.srcs[0] != 0)
    sprintf (s40, "%s", super->m17e.srcs);

  //encode callsign
  if (src < 0xEE6B27FFFFFF)
  {
    for(i = strlen((const char*)s40)-1; i >= 0; i--)
    {
      for(j = 0; j < 40; j++)
      {
        if(s40[i]==b40[j])
        {
          src=src*40+j;
          break;
          }
      }
    }
  }

  uint8_t conn[11]; memset (conn, 0, sizeof(conn));
  uint8_t disc[10]; memset (disc, 0, sizeof(disc));
  conn[0] = 0x43; conn[1] = 0x4F; conn[2] = 0x4E; conn[3] = 0x4E; conn[10] = super->m17e.reflector_module;
  disc[0] = 0x44; disc[1] = 0x49; disc[2] = 0x53; disc[3] = 0x43;

  conn[4] = (src >> 40UL) & 0xFF; conn[5] = (src >> 32UL) & 0xFF; conn[6] = (src >> 24UL) & 0xFF;
  conn[7] = (src >> 16UL) & 0xFF; conn[8] = (src >> 8UL)  & 0xFF; conn[9] = (src >> 0UL)  & 0xFF;
  for (i = 0; i < 6; i++)
    disc[i+4] = conn[i+4];

  //1 for conn, 0 for disc
  if (cd)
    m17_socket_blaster (super, 11, conn);
  else m17_socket_blaster (super, 10, disc);

}

void decode_ipf_duplex (Super * super)
{

  int err = 1; //NOTE: err will tell us how many bytes were received, if successful
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

  // while (!exitflag) //single pass only
  {

    //set current time for call history, etc
    super->demod.current_time = time(NULL);

    //read from socket, if available, or pass
    if (m17_udp_socket_duplex) //double check
      err = m17_socket_receiver_duplex(m17_udp_socket_duplex, &ip_frame);

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
        int kmod = (ret*8)%128; //This is how many bits we are short, so we need to account with a partial ks application

        //debug
        // fprintf (stderr, " AES KLEN: %d; KMOD: %d;", klen, kmod);

        //NOTE: Its pretty redundant to pack and unpack here and in the crypt function,
        //but this is still quicker than writing a new function for only one use case
        
        uint8_t unpacked_pkt[8000]; memset (unpacked_pkt, 0, 8000*sizeof(uint8_t)); //33*25*8 = 6600, but giving some extra space here (stack smash fix on full sized enc frame decrypt)
        unpack_byte_array_into_bit_array(ip_frame+34, unpacked_pkt, ret);
        for (i = 0; i < klen; i++)
          aes_ctr_str_payload_crypt (super->m17d.meta, super->enc.aes_key, unpacked_pkt+(128*i)+8, super->m17d.enc_st+1);

        //if there are leftovers (kmod), then run a keystream and partial application to left over bits
        uint8_t aes_ks_bits[128]; memset(aes_ks_bits, 0, 128*sizeof(uint8_t));
        int kmodstart = klen*128;

        //set to 8 IF kmodstart == 0 so we don't try to decrypt the protocol byte on short single block packets
        if (kmodstart == 0) kmodstart = 8;

        if (kmod != 0)
        {
          aes_ctr_str_payload_crypt (super->m17d.meta, super->enc.aes_key, aes_ks_bits, super->m17d.enc_st+1);
          for (i = 0; i < kmod; i++)
            unpacked_pkt[i+kmodstart] ^= aes_ks_bits[i];
        }
          
        pack_bit_array_into_byte_array(unpacked_pkt, ip_frame+34, ret);
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
        if (super->opts.use_m17_textgame_mode)
          decode_game_sms_gate (super, ip_frame+34, err-34-3);
        else decode_pkt_contents (super, ip_frame+34, err-34-3);
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

void m17_duplex_str (Super * super, uint8_t use_ip, int udpport, uint8_t reflector_module)
{

  float mem[81];

  //initialize RRC memory buffer
  memset (mem, 0, 81*sizeof(float));

  //set stream type value here so we can change 3200 or 1600 accordingly
  uint8_t st = 2; //stream type: 0 = res; 1 = data; 2 = voice(3200); 3 = voice(1600) + data;
  if (super->opts.m17_str_encoder_dt == 3) st = 3; //this is set to 3 IF -S user text string is called at CLI
  else st = 2; //otherwise, just use 3200 voice

  //IP Frame Things and User Variables for Reflectors, etc
  uint8_t nil[368]; //empty array to send to RF during Preamble, EOT Marker, or Dead Air
  memset (nil, 0, sizeof(nil));

  //User Defined Variables
  uint8_t can = 7; //channel access number
  //numerical representation of dst and src after b40 encoding, or special/reserved value
  unsigned long long int dst = 0;
  unsigned long long int src = 0;
  //DST and SRC Callsign Data (pick up to 9 characters from the b40 char array)
  char d40[50] = "M17-FME  "; //DST
  char s40[50] = "M17-FME  "; //SRC
  //end User Defined Variables

  //configure User Defined Variables, if defined at CLI
  if (super->m17e.can != -1) //has a set value
    can = super->m17e.can;

  if (super->m17e.srcs[0] != 0)
    sprintf (s40, "%s", super->m17e.srcs);

  if (super->m17e.dsts[0] != 0)
    sprintf (d40, "%s", super->m17e.dsts);

  //if special values, then assign them
  if (strcmp (d40, "ALL") == 0) //check for this first, or in duplex mode, we get a mix-match of ALL and BROADCAST
    sprintf (d40, "%s", "BROADCAST");

  if (strcmp (d40, "BROADCAST") == 0)
    dst = 0xFFFFFFFFFFFF;
  
  int i, j, k, x;    //basic utility counters
  short sample = 0;  //individual audio sample from source
  size_t nsam = 160; //number of samples to be read in (default is 160 samples for codec2 3200 bps)
  int dec = super->opts.input_sample_rate / 8000; //number of samples to run before selecting a sample from source input
  uint8_t eot_out = 0;

  //send dead air with type 99
  for (i = 0; i < 25; i++)
    encode_rfa (super, nil, mem, 99);

  //Standard IP Framing
  uint8_t magic[4] = {0x4D, 0x31, 0x37, 0x20};
  uint8_t ackn[4]  = {0x41, 0x43, 0x4B, 0x4E}; UNUSED(ackn);
  uint8_t nack[4]  = {0x4E, 0x41, 0x43, 0x4B}; UNUSED(nack);
  uint8_t conn[11]; memset (conn, 0, sizeof(conn));
  uint8_t disc[10]; memset (disc, 0, sizeof(disc));
  uint8_t ping[10]; memset (ping, 0, sizeof(ping));
  uint8_t pong[10]; memset (pong, 0, sizeof(pong));
  uint8_t eotx[10]; memset (eotx, 0, sizeof(eotx));
  int udp_return = 0; UNUSED(udp_return);
  uint8_t sid[2];   memset (sid, 0, sizeof(sid));
  uint8_t m17_ip_frame[432]; memset (m17_ip_frame, 0, sizeof(m17_ip_frame));
  uint8_t m17_ip_packed[54]; memset (m17_ip_packed, 0, sizeof(m17_ip_packed));
  uint16_t ip_crc = 0;

  //NONCE
  time_t epoch = 1577836800L;           //Jan 1, 2020, 00:00:00 UTC
  time_t ts = time(NULL) - epoch;      //timestamp since epoch
  srand((unsigned int)ts&0xFFFFFFFE); //randomizer seed based on timestamp

  //Stream ID value
  sid[0] = rand() & 0xFF;
  sid[1] = rand() & 0xFF;

  //initialize a nonce (if AES ENC is used)
  uint8_t nonce[14]; memset (nonce, 0, sizeof(nonce));
  //32bit LSB of the timestamp
  nonce[0]  = (ts >> 24) & 0xFF;
  nonce[1]  = (ts >> 16) & 0xFF;
  nonce[2]  = (ts >> 8)  & 0xFF;
  nonce[3]  = (ts >> 0)  & 0xFF;
  //64-bit of rnd data
  nonce[4]  = rand() & 0xFF;
  nonce[5]  = rand() & 0xFF;
  nonce[6]  = rand() & 0xFF;
  nonce[7]  = rand() & 0xFF;
  nonce[8]  = rand() & 0xFF;
  nonce[9]  = rand() & 0xFF;
  nonce[10] = rand() & 0xFF;
  nonce[11] = rand() & 0xFF;
  //Spec updated and removes the CTR_HIGH value
  nonce[12] = rand() & 0xFF;
  nonce[13] = rand() & 0xFF;

  #ifdef USE_CODEC2
  if      (st == 2)
    nsam = codec2_samples_per_frame(super->m17e.codec2_3200);
  else if (st == 3)
    nsam = codec2_samples_per_frame(super->m17e.codec2_1600);
  else nsam = 160; //default to 160 if RES, even if we don't handle those
  #endif

  //note: changed to always use the larger size, users can now
  //load and toggle arb data, so that will cause a segfault if we 
  //have a smaller malloc and then try to load more later
  //same for the voice1 and voice2 arrays
  short * samp1 = malloc (sizeof(short) * 320);
  short * samp2 = malloc (sizeof(short) * 320);

  short voice1[320]; //read in xxx ms of audio from input source
  short voice2[320]; //read in xxx ms of audio from input source
  
  //frame sequence number and eot bit
  uint16_t fsn = 0;
  uint8_t eot = 0;
  uint8_t lich_cnt = 0;   //lich frame number counter
  uint8_t lsf_count = 0; //rolling embedded LSF counter for sending substitution LSF embedded frames 

  uint8_t lsf_chunk[6][48]; //40 bit chunks of link information spread across 6 frames
  uint8_t m17_lsf[244];    //the complete LSF + 4 trailing bits

  memset (lsf_chunk, 0, sizeof(lsf_chunk));
  memset (m17_lsf, 0, sizeof(m17_lsf));

  //NOTE: Most lich and lsf_chunk bits can be pre-set before the while loop,
  //only need to refresh the lich_cnt value, nonce, and golay
  uint16_t lsf_ps = 1;                      //packet or stream indicator bit
  uint16_t lsf_dt = st;                     //stream type
  uint16_t lsf_et = super->enc.enc_type;    //encryption type
  uint16_t lsf_es = super->enc.enc_subtype; //encryption sub-type
  uint16_t lsf_cn = can;                    //can value
  uint16_t lsf_rs = 0;                      //reserved bits

  if (super->m17e.ecdsa.keys_loaded)
    lsf_rs = lsf_rs | (uint8_t)0x1; //OR 0x01 for ECDSA
  else lsf_rs = 0; //reset to zero

  if (lsf_et == 1)
  {
    scrambler_key_init (super, 1);
    lsf_es = super->enc.enc_subtype; //encryption sub-type
  }

  //if not encrypted, and meta data available, set lsf_es to met_st instead
  if (lsf_et == 0 && super->m17e.meta_data[0] != 0)
    lsf_es = super->m17e.met_st;

  //compose the 16-bit frame information from the above sub elements
  uint16_t lsf_fi = 0;
  lsf_fi = (lsf_ps & 1) + (lsf_dt << 1) + (lsf_et << 3) + (lsf_es << 5) + (lsf_cn << 7) + (lsf_rs << 11);
  for (i = 0; i < 16; i++) m17_lsf[96+i] = (lsf_fi >> (15-i)) & 1;

  //Convert base40 CSD to numerical values (lifted from libM17)

  //Only if not already set to a reserved value
  if (dst < 0xEE6B27FFFFFF)
  {
    for(i = strlen((const char*)d40)-1; i >= 0; i--)
    {
      for(j = 0; j < 40; j++)
      {
        if(d40[i]==b40[j])
        {
          dst=dst*40+j;
          break;
          }
      }
    }
  }

  if (src < 0xEE6B27FFFFFF)
  {
    for(i = strlen((const char*)s40)-1; i >= 0; i--)
    {
      for(j = 0; j < 40; j++)
      {
        if(s40[i]==b40[j])
        {
          src=src*40+j;
          break;
          }
      }
    }
  }
  //end CSD conversion

  //Setup conn, disc, eotx, ping, pong values
  conn[0] = 0x43; conn[1] = 0x4F; conn[2] = 0x4E; conn[3] = 0x4E; conn[10] = reflector_module;
  disc[0] = 0x44; disc[1] = 0x49; disc[2] = 0x53; disc[3] = 0x43;
  ping[0] = 0x50; ping[1] = 0x49; ping[2] = 0x4E; ping[3] = 0x47;
  pong[0] = 0x50; pong[1] = 0x4F; pong[2] = 0x4E; pong[3] = 0x47;
  eotx[0] = 0x45; eotx[1] = 0x4F; eotx[2] = 0x54; eotx[3] = 0x58; //EOTX is not Standard, but going to send / receive anyways

  //these values were not loaded correctly before, so just manually handle one and copy to others
  conn[4] = (src >> 40UL) & 0xFF; conn[5] = (src >> 32UL) & 0xFF; conn[6] = (src >> 24UL) & 0xFF;
  conn[7] = (src >> 16UL) & 0xFF; conn[8] = (src >> 8UL)  & 0xFF; conn[9] = (src >> 0UL)  & 0xFF;
  for (i = 0; i < 6; i++)
  {
    disc[i+4] = conn[i+4]; ping[i+4] = conn[i+4];
    pong[i+4] = conn[i+4]; eotx[i+4] = conn[i+4];
  }
  
  //load dst and src values into the LSF
  for (i = 0; i < 48; i++) m17_lsf[i+00] = (dst >> (47ULL-(unsigned long long int)i)) & 1;
  for (i = 0; i < 48; i++) m17_lsf[i+48] = (src >> (47ULL-(unsigned long long int)i)) & 1;

  //load the nonce from packed bytes to a bitwise iv array
  uint8_t iv[112]; memset(iv, 0, sizeof(iv));
  k = 0;
  for (j = 0; j < 14; j++)
  {
    for (i = 0; i < 8; i++)
      iv[k++] = (nonce[j] >> (7-i))&1;
  }

  //if AES enc employed, insert the iv into LSF
  if (lsf_et == 2)
  {
    for (i = 0; i < 112; i++)
      m17_lsf[i+112] = iv[i];
  }
  //else if not ENC and Meta data provided, unpack Meta data into META Field (up to 112/8 = 14 octets or chars)
  else if (lsf_et == 0 && super->m17e.meta_data[0] != 0)
  {
    unpack_byte_array_into_bit_array(super->m17e.meta_data+1, m17_lsf+112, 14);

    //Decode Meta Data Once For Ncurses Display if not loopback
    if (super->opts.internal_loopback_decoder == 0)
    {
      uint8_t meta_data[16]; memset (meta_data, 0, sizeof(meta_data));
      meta_data[0] = lsf_es + 0x80; //flip MSB bit to signal META
      memcpy (meta_data+1, super->m17e.meta_data+1, 14);
      fprintf (stderr, "\n ");
      decode_pkt_contents (super, meta_data, 15); //decode META
    }
  }

  //pack and compute the CRC16 for LSF
  uint16_t crc_cmp = 0;
  uint8_t lsf_packed[30];
  memset (lsf_packed, 0, sizeof(lsf_packed));
  for (i = 0; i < 28; i++)
      lsf_packed[i] = (uint8_t)convert_bits_into_output(&m17_lsf[i*8], 8);
  crc_cmp = crc16(lsf_packed, 28);

  //attach the crc16 bits to the end of the LSF data
  for (i = 0; i < 16; i++) m17_lsf[224+i] = (crc_cmp >> (15-i)) & 1;

  //pack the CRC
  for (i = 28; i < 30; i++)
      lsf_packed[i] = (uint8_t)convert_bits_into_output(&m17_lsf[i*8], 8);

  //Craft and Send Initial LSF frame to be decoded

  //LSF w/ convolutional encoding (double check array sizes)
  uint8_t m17_lsfc[488]; memset (m17_lsfc, 0, sizeof(m17_lsfc));

  //LSF w/ P1 Puncturing
  uint8_t m17_lsfp[368]; memset (m17_lsfp, 0, sizeof(m17_lsfp));

  //LSF w/ Interleave
  uint8_t m17_lsfi[368]; memset (m17_lsfi, 0, sizeof(m17_lsfi));

  //LSF w/ Scrambling
  uint8_t m17_lsfs[368]; memset (m17_lsfs, 0, sizeof(m17_lsfs));

  //Use the convolutional encoder to encode the LSF Frame
  simple_conv_encoder (m17_lsf, m17_lsfc, 244);

  //P1 puncture
  x = 0;
  for (i = 0; i < 488; i++)
  {
    if (p1[i%61] == 1)
      m17_lsfp[x++] = m17_lsfc[i];
  }

  //interleave the bit array using Quadratic Permutation Polynomial
  //function π(x) = (45x + 92x^2 ) mod 368
  for (i = 0; i < 368; i++)
  {
    x = ((45*i)+(92*i*i)) % 368;
    m17_lsfi[x] = m17_lsfp[i];
  }

  //scramble/randomize the frame
  for (i = 0; i < 368; i++)
    m17_lsfs[i] = (m17_lsfi[i] ^ m17_scramble[i]) & 1;

  //flag to determine if we send a new LSF frame for new encode
  //only send once at the appropriate time when encoder is toggled on
  int new_lsf = 1;

  while (!eot_out) //while we haven't sent an EOT frame out yet
  {

    //refresh these here to allow toggle control
    lsf_et = super->enc.enc_type;    //encryption type
    lsf_es = super->enc.enc_subtype; //encryption sub-type

    //if not encrypted, and meta data available, set lsf_es to met_st instead
    if (lsf_et == 0 && super->m17e.meta_data[0] != 0)
      lsf_es = super->m17e.met_st;

    if (super->m17e.ecdsa.keys_loaded)
      lsf_rs = lsf_rs | (uint8_t)0x1; //OR 0x01 for ECDSA
    else lsf_rs = 0; //reset to zero


    //this is set to 3 IF -A  arb user text string is called in ncurses
    if (super->opts.m17_str_encoder_dt == 3)
    {
      lsf_dt = 3;
      st = 3;
    }
    else //otherwise, just use 3200 voice
    {
      lsf_dt = 2;
      st = 2;
    }

    //compose the 16-bit frame information from the above sub elements
    lsf_fi = 0;
    lsf_fi = (lsf_ps & 1) + (lsf_dt << 1) + (lsf_et << 3) + (lsf_es << 5) + (lsf_cn << 7) + (lsf_rs << 11);
    for (i = 0; i < 16; i++) m17_lsf[96+i] = (lsf_fi >> (15-i)) & 1;

    //pack local meta for aes encryption function (bugfix for internal loopback AES not working)
    uint8_t meta[16]; memset (meta, 0, sizeof(meta));
    if (lsf_et == 2)
    {
      for (i = 0; i < 14; i++)
        meta[i] = nonce[i];
      for (i = 0; i < 8; i++)
      {
        meta[14] <<= 1;
        meta[15] <<= 1;
        meta[14] += ((fsn >> 8) >> (7-i)) & 1;
        meta[15] += ((fsn >> 0) >> (7-i)) & 1;
      }
    }

    //if not decoding internally, assign values for ncurses display
    if (super->opts.internal_loopback_decoder == 0)
    {
      sprintf (super->m17d.src_csd_str, "%s", s40);
      sprintf (super->m17d.dst_csd_str, "%s", d40);
      super->m17d.src = src;
      super->m17d.dst = dst;
      super->m17d.can = can;
      super->m17d.dt = lsf_dt;
      super->m17d.enc_et = lsf_et;
      super->m17d.enc_st = lsf_es;
      for (i = 0; i < 16; i++)
        super->m17d.meta[i] = meta[i];
    }

    //read in short audio input samples from source
    for (i = 0; i < (int)nsam; i++)
    {
      for (j = 0; j < dec; j++)
        sample = get_short_audio_input_sample(super);
      voice1[i] = sample; //only store the ith sample
    }

    if (st == 2)
    {
      for (i = 0; i < (int)nsam; i++)
      {
        for (j = 0; j < dec; j++)
          sample = get_short_audio_input_sample(super);
        voice2[i] = sample; //only store the ith sample
      }
    }

    //Apply Gain to Input
    input_gain_vx (super, voice1, nsam);
    if (st == 2)
      input_gain_vx (super, voice2, nsam);

    //convert out audio input into CODEC2 8 byte data stream
    uint8_t vc1_bytes[8]; memset (vc1_bytes, 0, sizeof(vc1_bytes));
    uint8_t vc2_bytes[8]; memset (vc2_bytes, 0, sizeof(vc2_bytes));

    #ifdef USE_CODEC2
    if (st == 2)
    {
      codec2_encode(super->m17e.codec2_3200, vc1_bytes, voice1);
      codec2_encode(super->m17e.codec2_3200, vc2_bytes, voice2);
    }
    if (st == 3)
      codec2_encode(super->m17e.codec2_1600, vc1_bytes, voice1);
    #endif

    //Fill vc2_bytes with arbitrary data, UTF-8 chars (up to 48)
    if (st == 3)
      memcpy (vc2_bytes, super->m17e.arb+(lich_cnt*8), 8);
    
    //initialize and start assembling the completed frame

    //Data/Voice Portion of Stream Data Link Layer w/ FSN
    uint8_t m17_v1[148]; memset (m17_v1, 0, sizeof(m17_v1));

    //Data/Voice Portion of Stream Data Link Layer w/ FSN (after Convolutional Encode)
    uint8_t m17_v1c[296]; memset (m17_v1c, 0, sizeof(m17_v1c));

    //Data/Voice Portion of Stream Data Link Layer w/ FSN (after P2 Puncturing)
    uint8_t m17_v1p[272]; memset (m17_v1p, 0, sizeof(m17_v1p));

    //LSF Chunk + LICH CNT of Stream Data Link Layer
    uint8_t m17_l1[48]; memset (m17_l1, 0, sizeof(m17_l1));

    //LSF Chunk + LICH CNT of Stream Data Link Layer (after Golay 24,12 Encoding)
    uint8_t m17_l1g[96]; memset (m17_l1g, 0, sizeof(m17_l1g));

    //Type 4c - Combined LSF Content Chuck and Voice/Data (96 + 272)
    uint8_t m17_t4c[368]; memset (m17_t4c, 0, sizeof(m17_t4c));

    //Type 4i - Interleaved Bits
    uint8_t m17_t4i[368]; memset (m17_t4i, 0, sizeof(m17_t4i));

    //Type 4s - Interleaved Bits with Scrambling Applied
    uint8_t m17_t4s[368]; memset (m17_t4s, 0, sizeof(m17_t4s));

    //insert the voice bytes into voice bits, and voice bits into v1 in their appropriate location
    uint8_t v1_bits[64]; memset (v1_bits, 0, sizeof(v1_bits));
    uint8_t v2_bits[64]; memset (v2_bits, 0, sizeof(v2_bits));

    k = 0; x = 0;
    for (j = 0; j < 8; j++)
    {
      for (i = 0; i < 8; i++)
      {
        v1_bits[k++] = (vc1_bytes[j] >> (7-i)) & 1;
        v2_bits[x++] = (vc2_bytes[j] >> (7-i)) & 1;
      }
    }

    for (i = 0; i < 64; i++)
    {
      m17_v1[i+16]    = v1_bits[i];
      m17_v1[i+16+64] = v2_bits[i];
    }

    //Apply Encryption to Voice and/or Arbitrary Data if Key Available

    //Scrambler
    if (super->enc.enc_type == 1 && super->enc.scrambler_key != 0)
    {
      super->enc.scrambler_seed_e = scrambler_sequence_generator(super, 1);
      for (i = 0; i < 128; i++)
        m17_v1[i+16] ^= super->enc.scrambler_pn[i];
    }

    //AES-CTR
    else if (super->enc.enc_type == 2 && super->enc.aes_key_is_loaded)
      aes_ctr_str_payload_crypt (meta, super->enc.aes_key, m17_v1+16, super->enc.enc_subtype+1);

    //if using encryption(or not), copy back to v1 an v2 bits so the IPF paylaod is also properly ENC'd
    for (i = 0; i < 64; i++)
    {
      v1_bits[i] = m17_v1[i+16];
      v2_bits[i] = m17_v1[i+16+64];
    }

    //ECDSA pack and track (after encryption)
    if (super->m17e.str_encoder_tx == 1)
    {
      uint8_t ecdsa_bytes[16]; memcpy(ecdsa_bytes, super->m17e.ecdsa.last_stream_pyl, 16*sizeof(uint8_t));
      pack_bit_array_into_byte_array (m17_v1+16, super->m17e.ecdsa.curr_stream_pyl, 16);
      for (i = 0; i < 16; i++)
        ecdsa_bytes[i] ^= super->m17e.ecdsa.curr_stream_pyl[i];
      left_shift_byte_array(ecdsa_bytes, super->m17e.ecdsa.last_stream_pyl, 16);

      //debug
      // fprintf (stderr, "\n ODG:");
      // for (i = 0; i < 16; i++)
      //   fprintf (stderr, "%02X", ecdsa_bytes[i]);

    }

    //set end of tx bit on the exitflag (sig, results not gauranteed) or toggle eot flag (always triggers)
    if (exitflag)
    {
      super->m17e.str_encoder_tx = 0;
      super->m17e.str_encoder_eot = 1;
      eot = 1;
    }
    if (super->m17e.str_encoder_eot) eot = 1;

    if (!super->m17e.ecdsa.keys_loaded)
      m17_v1[0] = (uint8_t)eot; //set as first bit of the stream
    else m17_v1[0] = 0;

    //set current frame number as bits 1-15 of the v1 stream
    for (i = 0; i < 15; i++)
      m17_v1[i+1] = ( (uint8_t)(fsn >> (14-i)) ) &1;

    //Use the convolutional encoder to encode the voice / data stream
    simple_conv_encoder (m17_v1, m17_v1c, 148);

    //use the P2 puncture to...puncture and collapse the voice / data stream
    k = 0; x = 0;
    for (i = 0; i < 25; i++)
    {
      m17_v1p[k++] = m17_v1c[x++];
      m17_v1p[k++] = m17_v1c[x++];
      m17_v1p[k++] = m17_v1c[x++];
      m17_v1p[k++] = m17_v1c[x++];
      m17_v1p[k++] = m17_v1c[x++];
      m17_v1p[k++] = m17_v1c[x++];
      m17_v1p[k++] = m17_v1c[x++];
      m17_v1p[k++] = m17_v1c[x++];
      //quit early on last set of i when 272 k bits reached 
      //index from 0 to 271,so 272 is breakpoint with k++
      if (k == 272) break;
      m17_v1p[k++] = m17_v1c[x++];
      m17_v1p[k++] = m17_v1c[x++];
      m17_v1p[k++] = m17_v1c[x++];
      x++;
    }

    //add punctured voice / data bits to the combined frame
    for (i = 0; i < 272; i++)
      m17_t4c[i+96] = m17_v1p[i];

    //make a backup copy of the original LSF
    memcpy (super->m17e.lsf_bkp, m17_lsf, 240*sizeof(uint8_t));

    //prepare substitution LSF with embedded OTAKD segment in it
    if (super->opts.use_otakd == 1)
    {
      if (super->enc.enc_type != 0 && ((lsf_count%5) != 0) )
        encode_ota_key_delivery_emb(super, m17_lsf, &lsf_count);
    }

    //load up the lsf chunk for this cnt
    for (i = 0; i < 40; i++)
      lsf_chunk[lich_cnt][i] = m17_lsf[((lich_cnt)*40)+i];

    //update lich_cnt in the current LSF chunk
    lsf_chunk[lich_cnt][40] = (lich_cnt >> 2) & 1;
    lsf_chunk[lich_cnt][41] = (lich_cnt >> 1) & 1;
    lsf_chunk[lich_cnt][42] = (lich_cnt >> 0) & 1;

    //encode with golay 24,12 and load into m17_l1g
    golay_24_12_encode (lsf_chunk[lich_cnt]+00, m17_l1g+00);
    golay_24_12_encode (lsf_chunk[lich_cnt]+12, m17_l1g+24);
    golay_24_12_encode (lsf_chunk[lich_cnt]+24, m17_l1g+48);
    golay_24_12_encode (lsf_chunk[lich_cnt]+36, m17_l1g+72);

    //add lsf chunk to the combined frame
    for (i = 0; i < 96; i++)
      m17_t4c[i] = m17_l1g[i];

    //interleave the bit array using Quadratic Permutation Polynomial
    //function π(x) = (45x + 92x^2 ) mod 368
    for (i = 0; i < 368; i++)
    {
      x = ((45*i)+(92*i*i)) % 368;
      m17_t4i[x] = m17_t4c[i];
    }

    //scramble/randomize the frame
    for (i = 0; i < 368; i++)
      m17_t4s[i] = (m17_t4i[i] ^ m17_scramble[i]) & 1;

    //-----------------------------------------

    //decode stream with the M17STR_debug
    if (super->m17e.str_encoder_tx == 1) //when toggled on
    {
      //Enable sync
      super->demod.in_sync = 1;

      //send LSF frame once, if new encode session
      if (new_lsf == 1)
      {

        //send the OTA key before LSF (AES and Scrambler)
        if (super->opts.use_otakd == 1)
        {
          if (super->enc.enc_type != 0)
            encode_ota_key_delivery_pkt(super, use_ip, sid, super->enc.enc_type, super->enc.enc_subtype);
        }

        //send the OTA key before LSF (Signature Public Key)
        if (super->opts.use_otask == 1)
        {
          if (super->m17e.ecdsa.keys_loaded == 1)
            encode_ota_key_delivery_pkt(super, use_ip, sid, 3, 0);
        }

        fprintf (stderr, "\n M17 LSF    (ENCODER): ");
        if (super->opts.internal_loopback_decoder == 1)
          demod_lsf(super, m17_lsfs, 1);
        else fprintf (stderr, " To Audio Out: %s", super->pa.pa_outrf_idx);

        //convert bit array into symbols and RF/Audio
        memset (nil, 0, sizeof(nil));
        encode_rfa (super,      nil, mem, 11); //Preamble
        // for (i = 0; i < 2; i++)
          encode_rfa (super, m17_lsfs, mem, 1); //LSF

        //flag off after sending
        new_lsf = 0;

        //flag to indicate to send one eot afterwards
        eot_out = 0; //redundant now?
      }

      fprintf (stderr, "\n M17 Stream (ENCODER): ");
      if (super->opts.internal_loopback_decoder == 1)
        demod_str(super, m17_t4s, 1);
      else fprintf (stderr, " To Audio Out: %s", super->pa.pa_outrf_idx);

      //show UDP if active
      if (use_ip == 1 && lich_cnt != 5)
        fprintf (stderr, " UDP: %s:%d", super->opts.m17_hostname, udpport);

      #ifdef USE_PULSEAUDIO
      //debug show pulse input latency
      if (super->opts.use_pa_input == 1 && super->opts.demod_verbosity >= 2)
      {
        unsigned long long int latency = pa_simple_get_latency (super->pa.pa_input_device, NULL);
        fprintf (stderr, " Latency: %05lld;", latency);
      }
      #endif

      //convert bit array into symbols and RF/Audio
      encode_rfa (super, m17_t4s, mem, 2);
      
      //Contruct an IP frame using previously created arrays
      memset (m17_ip_frame, 0, sizeof(m17_ip_frame));
      memset (m17_ip_packed, 0, sizeof(m17_ip_packed));

      //add MAGIC
      k = 0;
      for (j = 0; j < 4; j++)
      {
        for (i = 0; i < 8; i++)
          m17_ip_frame[k++] = (magic[j] >> (7-i)) &1;
      }

      //add StreamID
      for (j = 0; j < 2; j++)
      {
        for (i = 0; i < 8; i++)
          m17_ip_frame[k++] = (sid[j] >> (7-i)) &1;
      }

      //add the current LSF, sans CRC
      for (i = 0; i < 224; i++)
        m17_ip_frame[k++] = m17_lsf[i];

      //add eot bit flag
      m17_ip_frame[k++] = eot&1;

      //add current fsn value
      for (i = 0; i < 15; i++)
        m17_ip_frame[k++] = (fsn >> (14-i))&1;

      //voice and/or data payload
      for (i = 0; i < 64; i++)
        m17_ip_frame[k++] = v1_bits[i];

      //voice and/or data payload
      for (i = 0; i < 64; i++)
        m17_ip_frame[k++] = v2_bits[i];

      //pack current bit array into a byte array for a CRC check
      for (i = 0; i < 52; i++)
        m17_ip_packed[i] = (uint8_t)convert_bits_into_output(&m17_ip_frame[i*8], 8);
      ip_crc = crc16(m17_ip_packed, 52);

      //add CRC value to the ip frame
      for (i = 0; i < 16; i++)
        m17_ip_frame[k++] = (ip_crc >> (15-i))&1;
      
      //pack CRC into the byte array as well
      for (i = 52; i < 54; i++)
        m17_ip_packed[i] = (uint8_t)convert_bits_into_output(&m17_ip_frame[i*8], 8);

      //Send packed IP frame to UDP port if enabled
      if (use_ip == 1)
      {
        udp_return = m17_socket_blaster (super, 54, m17_ip_packed);

        //read socket, particularly if sent to broadcast ADDR, discard frame to prevent buffering
        m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
      }
        

      //increment lich_cnt, reset on 6
      lich_cnt++;
      if (lich_cnt == 6)
      {
        lich_cnt = 0;
        lsf_count++;
      }

      //increment frame sequency number, trunc to maximum value, roll nonce if needed
      fsn++;
      if (fsn > 0x7FFB) //changed from 0x7FFF to 0x7FFB to account for ECDSA frame numbers
      {
        fsn = 0;
        nonce[13]++;
        if (nonce[13] == 0) //if 0xFF rolls back over to zero, then
        {
          nonce[13] = 0; //roll over to zero of exceeds 0xFF
          nonce[12]++;
          nonce[12] &= 0xFF; //trunc for potential rollover (doesn't spill over)
        }
      }

      //restore original LSF (move to bottom so IP Frames can also have the embedded OTAKD)
      memcpy (m17_lsf, super->m17e.lsf_bkp, 240*sizeof(uint8_t));

    } //end if (super->m17d.strencoder_tx)

    else //if not tx, reset values, drop sync
    {

      //Send last IP Frame with EOT, pack this before resetting
      memset (m17_ip_frame, 0, sizeof(m17_ip_frame));
      memset (m17_ip_packed, 0, sizeof(m17_ip_packed));

      //add MAGIC
      k = 0;
      for (j = 0; j < 4; j++)
      {
        for (i = 0; i < 8; i++)
          m17_ip_frame[k++] = (magic[j] >> (7-i)) &1;
      }

      //add StreamID
      for (j = 0; j < 2; j++)
      {
        for (i = 0; i < 8; i++)
          m17_ip_frame[k++] = (sid[j] >> (7-i)) &1;
      }

      //add the current LSF, sans CRC
      for (i = 0; i < 224; i++)
        m17_ip_frame[k++] = m17_lsf[i];

      //add eot bit flag
      m17_ip_frame[k++] = eot&1;

      //add current fsn value
      for (i = 0; i < 15; i++)
        m17_ip_frame[k++] = (fsn >> (14-i))&1;

      //voice and/or data payload
      for (i = 0; i < 64; i++)
        m17_ip_frame[k++] = v1_bits[i];

      //voice and/or data payload
      for (i = 0; i < 64; i++)
        m17_ip_frame[k++] = v2_bits[i];

      //pack current bit array into a byte array for a CRC check
      for (i = 0; i < 52; i++)
        m17_ip_packed[i] = (uint8_t)convert_bits_into_output(&m17_ip_frame[i*8], 8);
      ip_crc = crc16(m17_ip_packed, 52);

      //add CRC value to the ip frame
      for (i = 0; i < 16; i++)
        m17_ip_frame[k++] = (ip_crc >> (15-i))&1;

      //pack CRC into the byte array as well
      for (i = 52; i < 54; i++)
        m17_ip_packed[i] = (uint8_t)convert_bits_into_output(&m17_ip_frame[i*8], 8);

      //reset
      if (!super->m17e.ecdsa.keys_loaded)
      {
        lich_cnt = 0;
        super->demod.in_sync = 0;
      }
      fsn = 0;
      lsf_count = 0;

      //update timestamp
      ts = time(NULL) - epoch;

      //update randomizer seed and SID
      srand((unsigned int)ts&0xFFFFFFFE); //randomizer seed based on timestamp

      //update Stream ID
      sid[0] = rand() & 0xFF;
      sid[1] = rand() & 0xFF;

      //update nonce
      nonce[0]  = (ts >> 24) & 0xFF;
      nonce[1]  = (ts >> 16) & 0xFF;
      nonce[2]  = (ts >> 8)  & 0xFF;
      nonce[3]  = (ts >> 0)  & 0xFF;
      nonce[4]  = rand() & 0xFF;
      nonce[5]  = rand() & 0xFF;
      nonce[6]  = rand() & 0xFF;
      nonce[7]  = rand() & 0xFF;
      nonce[8]  = rand() & 0xFF;
      nonce[9]  = rand() & 0xFF;
      nonce[10] = rand() & 0xFF;
      nonce[11] = rand() & 0xFF;
      nonce[12] = rand() & 0xFF;
      nonce[13] = rand() & 0xFF;

      //load the nonce from packed bytes to a bitwise iv array
      memset(iv, 0, sizeof(iv));
      k = 0;
      for (j = 0; j < 14; j++)
      {
        for (i = 0; i < 8; i++)
          iv[k++] = (nonce[j] >> (7-i))&1;
      }

      //if AES enc employed, insert the iv into LSF
      if (lsf_et == 2)
      {
        for (i = 0; i < 112; i++)
          m17_lsf[i+112] = iv[i];
      }
      //Scrambler
      else if (lsf_et == 1)
      {
        super->enc.scrambler_seed_e = super->enc.scrambler_key; //reset seed value
      }
      //else if not ENC and Meta data provided, unpack Meta data into META Field (up to 112/8 = 14 octets or chars)
      else if (lsf_et == 0 && super->m17e.meta_data[0] != 0)
        unpack_byte_array_into_bit_array(super->m17e.meta_data+1, m17_lsf+112, 14);
      else //zero out the meta field (prevent bad meta data decodes on decoder side)
      {
        for (i = 0; i < 112; i++)
          m17_lsf[i+112] = 0;
      }

      //repack, new CRC, and update rest of lsf as well
      memset (lsf_packed, 0, sizeof(lsf_packed));
      for (i = 0; i < 28; i++)
        lsf_packed[i] = (uint8_t)convert_bits_into_output(&m17_lsf[i*8], 8);
      crc_cmp = crc16(lsf_packed, 28);

      //attach the crc16 bits to the end of the LSF data
      for (i = 0; i < 16; i++) m17_lsf[224+i] = (crc_cmp >> (15-i)) & 1;

      //repack the CRC
      for (i = 28; i < 30; i++)
          lsf_packed[i] = (uint8_t)convert_bits_into_output(&m17_lsf[i*8], 8);

      //Recraft and Prepare New LSF frame for next encoding session
      //this is primarily to make sure the LSF has the refreshed nonce
      memset (m17_lsfc, 0, sizeof(m17_lsfc));
      memset (m17_lsfp, 0, sizeof(m17_lsfp));
      memset (m17_lsfi, 0, sizeof(m17_lsfi));
      memset (m17_lsfs, 0, sizeof(m17_lsfs));

      //Use the convolutional encoder to encode the LSF Frame
      simple_conv_encoder (m17_lsf, m17_lsfc, 244);

      //P1 puncture
      x = 0;
      for (i = 0; i < 488; i++)
      {
        if (p1[i%61] == 1)
          m17_lsfp[x++] = m17_lsfc[i];
      }

      //interleave the bit array using Quadratic Permutation Polynomial
      //function π(x) = (45x + 92x^2 ) mod 368
      for (i = 0; i < 368; i++)
      {
        x = ((45*i)+(92*i*i)) % 368;
        m17_lsfi[x] = m17_lsfp[i];
      }

      //scramble/randomize the frame
      for (i = 0; i < 368; i++)
        m17_lsfs[i] = (m17_lsfi[i] ^ m17_scramble[i]) & 1;

      //flush the last frame with the eot bit on, if not using ECDSA
      if (eot && !eot_out && !super->m17e.ecdsa.keys_loaded)
      {
        fprintf (stderr, "\n M17 Stream (ENCODER): ");
        if (super->opts.internal_loopback_decoder == 1)
          demod_str(super, m17_t4s, 1);
        else fprintf (stderr, " To Audio Out: %s", super->pa.pa_outrf_idx);

        //show UDP if active
        if (use_ip == 1 && lich_cnt != 5)
          fprintf (stderr, " UDP: %s:%d", super->opts.m17_hostname, udpport);

        //convert bit array into symbols and RF/Audio
        encode_rfa (super, m17_t4s, mem, 2); //Last Stream Frame

        memset (nil, 0, sizeof(nil));
        encode_rfa (super, nil, mem, 55);    //EOT Marker

        //send dead air with type 99
        memset (nil, 0, sizeof(nil));
        for (i = 0; i < 25; i++)
          encode_rfa (super, nil, mem, 99);

        //send IP Frame with EOT bit
        if (use_ip == 1)
        {
          udp_return = m17_socket_blaster (super, 54, m17_ip_packed);
          
          //read socket, particularly if sent to broadcast ADDR, discard frame to prevent buffering
          m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
        }

        //SEND EOTX to reflector
        if (use_ip == 1)
        {
          udp_return = m17_socket_blaster (super, 10, eotx);

          //read socket, particularly if sent to broadcast ADDR, discard frame to prevent buffering
          m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
        }

        //reset indicators
        eot = 0;
        eot_out = 1;
        super->m17e.str_encoder_eot = 0;
      }

      //flush the last 4 frames with attached ECDSA Signatures
      else if (eot && !eot_out && super->m17e.ecdsa.keys_loaded)
      {

        encode_str_ecdsa(super, lich_cnt, mem, use_ip, udpport, can, st, sid, src, dst);

        memset (nil, 0, sizeof(nil));
        encode_rfa (super, nil, mem, 55);    //EOT Marker

        //send dead air with type 99
        memset (nil, 0, sizeof(nil));
        for (i = 0; i < 25; i++)
          encode_rfa (super, nil, mem, 99);

        //SEND EOTX to reflector
        if (use_ip == 1)
        {
          udp_return = m17_socket_blaster (super, 10, eotx);

          //read socket, particularly if sent to broadcast ADDR, discard frame to prevent buffering
          m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
        }

        //reset indicators
        eot = 0;
        eot_out = 1;
        super->m17e.str_encoder_eot = 0;
        lich_cnt = 0;
        super->demod.in_sync = 0;

      }

      //reset ECSDA payloads until tx restart
      memset (super->m17e.ecdsa.curr_stream_pyl, 0, 16*sizeof(uint8_t));
      memset (super->m17e.ecdsa.last_stream_pyl, 0, 16*sizeof(uint8_t));
      memset (super->m17e.ecdsa.signature, 0, 64*sizeof(uint8_t));

      //flag on when restarting the encoder
      new_lsf = 1;

      //flush decoder side meta last, primarily the last two octets with the lich_cnt in them
      memset(super->m17d.meta, 0, sizeof(super->m17d.meta));

      //flush decoder side lsf, may be redundant, but using to make sure no stale values loaded during debug
      memset(super->m17d.lsf, 0, sizeof(super->m17d.lsf));

    }

    //refresh ncurses printer, if enabled
    #ifdef USE_CURSES
    if (super->opts.use_ncurses_terminal == 1)
      print_ncurses_terminal(super);
    #endif

    #ifdef USE_CODEC2
    if      (st == 2)
      nsam = codec2_samples_per_frame(super->m17e.codec2_3200);
    else if (st == 3)
      nsam = codec2_samples_per_frame(super->m17e.codec2_1600);
    else nsam = 160; //default to 160 if RES, even if we don't handle those
    #endif
    
  }
  
  //free allocated memory
  free(samp1);
  free(samp2);

  //get rid of this if it costs too much CPU / skips / lags
  super->demod.current_time = time(NULL);

}

//the duplex loop
void m17_duplex_mode (Super * super)
{

  //quell defined but not used warnings from m17.h
  stfu ();

  //use packet_burst_time to periodically signal a burst packet (testing)
  time_t packet_burst_time = time(NULL);

  //open any output files
  open_file_output(super);

  //Disable Stream Encoder TX on Duplex
  super->m17e.str_encoder_tx = 0;

  //Disable VOX mode on Duplex
  super->m17e.str_encoder_vox = 0;

  //Disable Ncurses IO Display (save screen space)
  super->opts.ncurses_show_io = 0;

  //Disable Ncurses Banner Display (save screen space)
  // super->opts.ncurses_show_banner = 0;

  //Disable Loopback Mode
  super->opts.internal_loopback_decoder = 0;

  //Enable Ncurses Terminal, and open if not already opened
  #ifdef USE_CURSES
  super->opts.use_ncurses_terminal = 1;
  if (super->opts.ncurses_is_open == 0)
    open_ncurses_terminal(super);
  #endif

  //will require Pulse Audio use for now
  open_pulse_audio_output_vx(super);
  super->opts.use_pa_input_vx = 0;
  super->opts.use_pa_input = 0;

  //open only if not using IP Frames
  if (super->opts.m17_use_ip == 0)
  {
    open_pulse_audio_output_rf(super);
    open_pulse_audio_input_rf(super);
    super->opts.use_pa_input = 1;
  }

  //NOTE: UDP IP Frames now working properly in UDP IP mode, IP Address is TARGET IP Address
  //example:
  //local  machine: m17-fme -D 2> m17e.txt -I -U 192.168.7.5:17000
  //remote machine: m17-fme -D 2> m17e.txt -I -U 192.168.7.8:17000
  //or use broadcast IP address:
  //both machines: m17-fme -D 2> m17e.txt -I -U 192.168.7.255:17000

  //NOTE: DO NOT use localhost or 127.0.0.1 as the -U address, or use the default addresss
  //check for localhost, or 127.0.0.1, disable ip if target address is self
  //need potential IP6 variations, if possible, not sure if net_udp is configured to allow IP6 traffic

  if (super->opts.m17_use_ip == 1)
  {
    if ( (strcmp(super->opts.m17_hostname, "127.0.0.1") == 0) || 
         (strcmp(super->opts.m17_hostname, "localhost") == 0)  )
    {
      fprintf (stderr, "Cannot use host: %s as target address on Duplex Mode.\n", super->opts.m17_hostname);
      super->opts.m17_use_ip = 0;
    }
  }

  //Open UDP port to default or user defined values, if enabled
  uint8_t use_ip = 0;
  int udpport = super->opts.m17_portno;
  uint8_t reflector_module = (uint8_t)super->m17e.reflector_module;
  int sock_err = 0;
  if (super->opts.m17_use_ip == 1)
  {
    
    //Bind UDP Socket for recevier (127.0.0.1:17000)
    m17_udp_socket_duplex = udp_socket_bind("localhost", 17000);

    //Socket Connect for sender
    sock_err = udp_socket_connectM17(super);

    if (m17_udp_socket_duplex < 0 && sock_err < 0)
    {
      fprintf (stderr, "Error Configuring UDP Socket for M17 IP Frame :( \n");
      use_ip = 0;
      super->opts.m17_use_ip = 0;
    }
    else
    {
      use_ip = 1;
      ip_send_conn_disc(super, 1);
      m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
    }
  }

  int k = 0; //packet rounds

  while (!exitflag)
  {

    //initial current_time set
    super->demod.current_time = time(NULL);

    if (super->m17e.str_encoder_tx == 1)
    {

      //Toggle PA Input modes
      super->opts.use_pa_input_vx = 1;
      super->opts.use_pa_input = 0;

      //open one, close the other
      if (super->opts.m17_use_ip == 0)
        close_pulse_audio_input(super);
      open_pulse_audio_input_vx(super);

      m17_duplex_str (super, use_ip, udpport, reflector_module);

      //read any trailing rf samples, but discard (trailing framesync bug)
      for (int i = 0; i < 1920*6; i++)
        get_short_audio_input_sample(super);

      //open one, close the other
      close_pulse_audio_input_vx(super);
      if (super->opts.m17_use_ip == 0)
        open_pulse_audio_input_rf(super);

      //send a packet after voice stream
      if (super->opts.use_m17_packet_burst == 2)
      {
        memset  (super->m17e.raw, 0, sizeof(super->m17e.raw));
        sprintf (super->m17e.sms, "%s", "This is a trailing SMS Text Messages sent after Voice Stream.");
        encode_pkt(super, 0);
      }

      //update packet_burst_time so it doesn't fire off immediately after a voice stream
      packet_burst_time = time(NULL); 

    }

    //screwing around with sending random packet bursts / beacons on an interval
    else if ( ((super->demod.current_time - packet_burst_time) > 30) && (super->demod.in_sync == 0) && (super->opts.use_m17_packet_burst == 1) )
    {
      super->demod.in_sync = 1;

      memset  (super->m17e.raw, 0, sizeof(super->m17e.raw));
      sprintf (super->m17e.sms, "%s", "");

      
      if (k == 0)
        sprintf (super->m17e.sms, "The Current Time is: %s %s (UTC -04:00)", get_date_n(super->demod.current_time), get_time_n(super->demod.current_time));
      else if (k == 1)
        sprintf (super->m17e.sms, "This is a Packet Beacon Originating from: %s", super->m17e.srcs);
      else
        sprintf (super->m17e.sms, "%s", ""); //set as blank and it'll send the Lorem text message
      
      //send current loaded packet
      encode_pkt(super, 0);
      packet_burst_time = time(NULL);
      
      //increment k, reset if greater than x
      k++;
      if (k >= 3)
        k = 0;

      super->demod.in_sync = 0;
    }

    else if (!use_ip)
    {

      //Toggle PA Input modes
      super->opts.use_pa_input_vx = 0;
      super->opts.use_pa_input = 1;

      //refresh ncurses printer, if enabled
      #ifdef USE_CURSES
      if (super->opts.use_ncurses_terminal == 1)
        print_ncurses_terminal(super);
      #endif

      //look for framesync
      framesync (super);

      //extra verbosity debug info dump
      if (super->opts.payload_verbosity >= 3)
        print_debug_information(super);

      //calculate sync time_delta for when to reset carrier sync ptrs and demod/decode values
      super->demod.current_time = time(NULL);
      time_t time_delta = super->demod.current_time - super->demod.sync_time;

      //no carrier sync if we were in sync and time_delta is equal to or more than 1 second
      if (super->demod.in_sync == 1 && time_delta > 0)
        no_carrier_sync(super);
    }

    else if (use_ip)
      decode_ipf_duplex(super);
  }

  //close UDP Socket afterwards
  if (m17_udp_socket_duplex)
  {
    ip_send_conn_disc(super, 0);
    m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
    close(m17_udp_socket_duplex);
  }

}

//m17 text based games for repeaters
void m17_text_games (Super * super)
{

  uint32_t progress = 0x00000000;

  //use idle_time to periodically signal a beacon
  time_t idle_time = 0;

  //open any output files
  open_file_output(super);

  //will require Pulse Audio use for now
  open_pulse_audio_output_vx(super);
  super->opts.use_pa_input_vx = 0;
  super->opts.use_pa_input = 0;

  //open only if not using IP Frames
  if (super->opts.m17_use_ip == 0)
  {
    open_pulse_audio_output_rf(super);
    open_pulse_audio_input_rf(super);
    super->opts.use_pa_input = 1;
  }

  //NOTE: UDP IP Frames now working properly in UDP IP mode, IP Address is TARGET IP Address
  //example:
  //local  machine: m17-fme -D 2> m17e.txt -I -U 192.168.7.5:17000
  //remote machine: m17-fme -D 2> m17e.txt -I -U 192.168.7.8:17000
  //or use broadcast IP address:
  //both machines: m17-fme -D 2> m17e.txt -I -U 192.168.7.255:17000

  //NOTE: DO NOT use localhost or 127.0.0.1 as the -U address, or use the default addresss
  //check for localhost, or 127.0.0.1, disable ip if target address is self
  //need potential IP6 variations, if possible, not sure if net_udp is configured to allow IP6 traffic

  if (super->opts.m17_use_ip == 1)
  {
    if ( (strcmp(super->opts.m17_hostname, "127.0.0.1") == 0) || 
         (strcmp(super->opts.m17_hostname, "localhost") == 0)  )
    {
      fprintf (stderr, "Cannot use host: %s as target address on Duplex Mode.\n", super->opts.m17_hostname);
      super->opts.m17_use_ip = 0;
    }
  }

  //Open UDP port to default or user defined values, if enabled
  uint8_t use_ip = 0; UNUSED(use_ip);
  // int udpport = super->opts.m17_portno;
  // uint8_t reflector_module = (uint8_t)super->m17e.reflector_module;
  int sock_err = 0;
  if (super->opts.m17_use_ip == 1)
  {
    
    //Bind UDP Socket for recevier (127.0.0.1:17000)
    m17_udp_socket_duplex = udp_socket_bind("localhost", 17000);

    //Socket Connect for sender
    sock_err = udp_socket_connectM17(super);

    if (m17_udp_socket_duplex < 0 && sock_err < 0)
    {
      fprintf (stderr, "Error Configuring UDP Socket for M17 IP Frame :( \n");
      use_ip = 0;
      super->opts.m17_use_ip = 0;
    }
    else
    {
      use_ip = 1;
      ip_send_conn_disc(super, 1);
      m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
    }
  }

  while (!exitflag)
  {

    //initial current_time set
    super->demod.current_time = time(NULL);

    //Send Game Advertisement on a 30 idle
    if ( ((super->demod.current_time - idle_time) > 30) )
    {
      super->demod.in_sync = 1;

      if (super->m17d.game_progress != 0)
        progress = super->m17d.game_progress;
      else progress = super->m17e.game_progress;

      //load current repeating message to send
      load_game_advertisement(super, progress);
      
      //send current loaded packet
      encode_pkt(super, 0);
      idle_time = time(NULL);

      //read any trailing rf samples, but discard (trailing framesync bug)
      for (int i = 0; i < samp_num; i++)
        get_short_audio_input_sample(super);

      //read but discard sent IP frame to prevent read/reply loop
      if (m17_udp_socket_duplex) //one for MPKT, one for EOTX
      {
        m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
        m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
      }

      super->demod.in_sync = 0;
    }

    else if (!use_ip)
    {

      //Toggle PA Input modes
      super->opts.use_pa_input_vx = 0;
      super->opts.use_pa_input = 1;

      //look for framesync
      framesync (super);

      //extra verbosity debug info dump
      if (super->opts.payload_verbosity >= 3)
        print_debug_information(super);

      //calculate sync time_delta for when to reset carrier sync ptrs and demod/decode values
      super->demod.current_time = time(NULL);
      time_t time_delta = super->demod.current_time - super->demod.sync_time;

      if (super->demod.in_sync == 1)
        idle_time = time(NULL);

      //no carrier sync if we were in sync and time_delta is equal to or more than 1 second
      if (super->demod.in_sync == 1 && time_delta > 0)
        no_carrier_sync(super);
    }

    else if (use_ip)
      decode_ipf_duplex(super);
  }

  //close UDP Socket afterwards
  if (m17_udp_socket_duplex)
  {
    ip_send_conn_disc(super, 0);
    m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
    close(m17_udp_socket_duplex);
  }

}

//gate packet data and if SMS, see what reply is required
void decode_game_sms_gate(Super * super, uint8_t * input, int len)
{
  int i = 0;
  uint8_t protocol = input[0];
  if (protocol != 0x05)
  {
    super->demod.in_sync = 1;

    memset  (super->m17e.raw, 0, sizeof(super->m17e.raw));
    sprintf (super->m17e.sms, "Please Use Compatible SMS Format Protocol 0x05. See M17 Specifications. ");
    
    //send current loaded packet
    encode_pkt(super, 0);

    //read any trailing rf samples, but discard (trailing framesync bug)
    for (int i = 0; i < samp_num; i++)
      get_short_audio_input_sample(super);

    //read but discard sent IP frame to prevent read/reply loop
    if (m17_udp_socket_duplex) //one for MPKT, one for EOTX
    {
      m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
      m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
    }

    super->demod.in_sync = 0;
  }
  else //echo and decode players text message and send appropriate reply
  {
    fprintf (stderr, "\n SMS: ");
    for (i = 1; i < len; i++)
    {
      fprintf (stderr, "%c", input[i]);
    }

    memset (super->m17d.sms, 0, 800*sizeof(char));
    sprintf (super->m17d.sms, "%s", "");
    memcpy (super->m17d.sms, input+1, len);
    memset  (super->m17e.raw, 0, sizeof(super->m17e.raw));

    //gameplay logic
    generate_game_sms_reply(super, super->m17d.sms);
    
  }
}

//basically, just a bunch of string compares, progression, and then load a text message
void generate_game_sms_reply(Super * super, char * input)
{
  uint8_t okay = 1; //command is okay, or has additional progression to send
  if ( (strncmp(input, "start", 5) == 0) )
  {
    sprintf (super->m17e.sms, "Welcome to Text Exploration Game.");
    super->m17e.game_progress = 0x80000000;
  }

  else if ( (strncmp(input, "continue", 8) == 0) )
  {
    //Read extended input to get game_progress value
    sscanf(input+9, "%X", &super->m17e.game_progress);
    if (super->m17e.game_progress == 0)
      sprintf (super->m17e.sms, "No Progress Point Entered.");
    else if (!(super->m17e.game_progress & 0x80000000)) //game start bit not flipped
    {
      sprintf (super->m17e.sms, "Unknown Progress Entry: %08X ", super->m17e.game_progress);
      // super->m17e.game_progress = super->m17d.game_progress; //revert to last point instead
      super->m17e.game_progress = 0x80000000; //go back to start?
    }
    else sprintf (super->m17e.sms, "Continuing from Save Progress: %08X ", super->m17e.game_progress);

  }

  else if ( (strncmp(input, "quit", 4) == 0) )
  {
    sprintf (super->m17e.sms, "You have chosen to quit. Save Progress: %08X;", super->m17e.game_progress);
    fprintf (stderr, "\n QUIT: %s \n", super->m17e.sms);
    super->m17e.game_progress = 0x00000000; //reset progress back to advertisement
  }

  else if ( (strncmp(input, "north", 5) == 0) )
  {
    sprintf (super->m17e.sms, "Traveling North... ");
    super->m17e.game_progress += 0x00010000;
  }

  else if ( (strncmp(input, "south", 5) == 0) )
  {
    sprintf (super->m17e.sms, "Traveling South... ");
    super->m17e.game_progress -= 0x00010000;
  }

  else if ( (strncmp(input, "east", 4) == 0) )
  {
    sprintf (super->m17e.sms, "Traveling East... ");
    super->m17e.game_progress += 0x00000001;
  }

  else if ( (strncmp(input, "west", 4) == 0) )
  {
    sprintf (super->m17e.sms, "Traveling West... ");
    super->m17e.game_progress -= 0x00000001;
  }

  //any unknown command
  else
  {
    okay = 0;
    sprintf (super->m17e.sms, "Unknown Command. Please Try Again. ");
  }

  //initial reply
  fprintf (stderr, "\n Reply: %s", super->m17e.sms);
  super->demod.in_sync = 1;
  //send current loaded packet
  encode_pkt(super, 0);
  //read any trailing rf samples, but discard (trailing framesync bug)
  for (int i = 0; i < samp_num; i++)
    get_short_audio_input_sample(super);

  //read but discard sent IP frame to prevent read/reply loop
  if (m17_udp_socket_duplex) //one for MPKT, one for EOTX
  {
    m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
    m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
  }

  super->demod.in_sync = 0;

  //load progress and send second message
  if (okay)
  {
    game_text(super);
    fprintf (stderr, "\n Reply: %s", super->m17e.sms);
    super->demod.in_sync = 1;
    //send current loaded packet
    encode_pkt(super, 0);
    //read any trailing rf samples, but discard (trailing framesync bug)
    for (int i = 0; i < samp_num; i++)
      get_short_audio_input_sample(super);

    //read but discard sent IP frame to prevent read/reply loop
    if (m17_udp_socket_duplex) //one for MPKT, one for EOTX
    {
      m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
      m17_socket_receiver_duplex(m17_udp_socket_duplex, NULL);
    }

    super->demod.in_sync = 0;
  }

  super->m17d.game_progress = super->m17e.game_progress;

}

void load_game_advertisement(Super * super, uint32_t input)
{
  memset  (super->m17e.raw, 0, sizeof(super->m17e.raw));
  if (input == 0x00000000)
    sprintf (super->m17e.sms, "This is a game repeater brought to you by: %s; Text 'start' to start the game. ", super->m17e.srcs);

  // leave disabled to just repeat on loop the last sent SMS message in case user missed it, may need a timeout reset after x minutes?
  // else sprintf (super->m17e.sms, "This is a game repeater brought to you by: %s; Text 'start' to start your game. ", super->m17e.srcs);

  //else just send last sent message

}

//WARNING! DO NOT SCROLL DOWN ANY FURTHER UNLESS YOU WANT TO SPOIL THE EXPLORATION / ADVENTURE GAME TEXT










































































//DON'T BE A DIRTY CHEATER















































//LAST WARNING FOR SPOILERS


























































void game_text(Super * super)
{
  uint32_t progress = super->m17e.game_progress;
  sprintf (super->m17e.sms, " Commands: north, south, east, west, quit; Progression Point: %08X\n", progress);
  switch (progress)
  {
    case 0x00000000:
      sprintf (super->m17e.sms, "Thank you for playing the game.");
      break;

    //starting location on map
    case 0x80000000:
      strcat (super->m17e.sms, "You find yourself in a dimly lit room, a candle flickering on top of a small night stand, casting the hue of its flames against the run down wall. There is a door to the north. To the south, a grimy dirty mattress.");
      break;

    //Traveling North...
    case 0x80010000:
      strcat (super->m17e.sms, "You venture outside of the small shack, once inhabited by yourself. To the north there appears to be an old graveyard.");
      break;

    case 0x80020000:
      strcat (super->m17e.sms, "Entering the graveyard, you see skeletons rising from the grave.");
      // super->m17e.game_progress = 0x80000000;
      break;

    case 0x7FFF0000:
      strcat (super->m17e.sms, "You lay on the dirty filthy matress, roll over and close your eyes. Head North to wake up.");
      break;

    default:
      sprintf (super->m17e.sms, "There nothing but emptiness for as far as the eye can see, either that, or you may have bumped your head into the wall. Unknown Progression Point: %08x", progress);

  }

}