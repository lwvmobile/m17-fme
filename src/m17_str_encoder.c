/*-------------------------------------------------------------------------------
 * m17_str_encoder.c
 * Project M17 - Stream Voice Encoder
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

//encode and create audio of a Project M17 Stream signal
void encodeM17STR(Super * super)
{
  float mem[81];

  //quell defined but not used warnings from m17.h
  UNUSED(b40); UNUSED(m17_scramble); UNUSED(p1); UNUSED(p3); UNUSED(symbol_map); UNUSED(m17_rrc);
  UNUSED(lsf_sync_symbols); UNUSED(str_sync_symbols); UNUSED(pkt_sync_symbols); UNUSED(symbol_levels);

  //initialize RRC memory buffer
  memset (mem, 0, 81*sizeof(float));

  //set stream type value here so we can change 3200 or 1600 accordingly
  uint8_t st = 2; //stream type: 0 = res; 1 = data; 2 = voice(3200); 3 = voice(1600) + data;
  if (super->opts.m17_str_encoder_dt == 3) st = 3; //this is set to 3 IF -S user text string is called at CLI
  else st = 2; //otherwise, just use 32066 voice

  //IP Frame Things and User Variables for Reflectors, etc
  uint8_t nil[368]; //empty array to send to RF during Preamble, EOT Marker, or Dead Air
  memset (nil, 0, sizeof(nil));

  //Enable frame, TX and Ncurses Printer
  super->m17e.str_encoder_tx = 1;
  
  // if (super->opts.use_ncurses_terminal == 1)
  //   ncursesOpen(opts, state);

  //if using the ncurses terminal, disable TX on startup until user toggles it with the '\' key, if not vox enabled
  if (super->opts.use_ncurses_terminal == 1 && super->opts.use_m17_str_encoder == 1 && super->m17e.str_encoder_vox == 0)
    super->m17e.str_encoder_tx = 0;

  //User Defined Variables
  int use_ip = 0; //1 to enable IP Frame Broadcast over UDP
  int udpport = super->opts.m17_portno; //port number for M17 IP Frame (default is 17000)
  //set at startup now via CLI, or use default if no user value specified
  uint8_t reflector_module = super->m17e.reflector_module;
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
  if (strcmp (d40, "BROADCAST") == 0)
    dst = 0xFFFFFFFFFFFF;
  if (strcmp (d40, "ALL") == 0)
    dst = 0xFFFFFFFFFFFF;
  //end
  
  int i, j, k, x;    //basic utility counters
  short sample = 0;  //individual audio sample from source
  size_t nsam = 160; //number of samples to be read in (default is 160 samples for codec2 3200 bps)
  int dec = super->opts.input_sample_rate / 8000; //number of samples to run before selecting a sample from source input
  int sql_hit = 11; //squelch hits, hit enough, and deactivate vox
  int eot_out =  1; //if we have already sent the eot out once

  //send dead air with type 99
  for (i = 0; i < 25; i++)
    encodeM17RF (super, nil, mem, 99);

  //Open UDP port to default or user defined values, if enabled
  int sock_err;
  if (super->opts.m17_use_ip == 1)
  {
    //
    sock_err = udp_socket_connectM17(super);
    if (sock_err < 0)
    {
      fprintf (stderr, "Error Configuring UDP Socket for M17 IP Frame :( \n");
      use_ip = 0;
      super->opts.m17_use_ip = 0;
    }
    else use_ip = 1;
  }

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
  time_t ts = time(NULL); //timestamp since epoch / "Unix Time"
  srand(ts); //randomizer seed based on timestamp

  //Stream ID value
  sid[0] = rand() & 0xFF;
  sid[1] = rand() & 0xFF;

  //initialize a nonce (if ENC is required in future)
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
  //The last two octets are the CTR_HIGH value (upper 16 bits of the frame number),
  //but you would need to talk non-stop for over 20 minutes to roll it, so just using rnd
  //also, using zeroes seems like it may be a security issue, so using rnd as a base
  nonce[12] = rand() & 0xFF;
  nonce[13] = rand() & 0xFF;

  #ifdef USE_CODEC2
  if      (st == 2)
    nsam = codec2_samples_per_frame(super->m17e.codec2_3200);
  else if (st == 3)
    nsam = codec2_samples_per_frame(super->m17e.codec2_1600);
  else nsam = 160; //default to 160 if RES or DATA, even if we don't handle those
  #endif

  short * samp1 = malloc (sizeof(short) * nsam);
  short * samp2 = malloc (sizeof(short) * nsam);

  short voice1[nsam]; //read in xxx ms of audio from input source
  short voice2[nsam]; //read in xxx ms of audio from input source
  
  //frame sequence number and eot bit
  uint16_t fsn = 0;
  uint8_t eot = 0;
  uint8_t lich_cnt = 0; //lich frame number counter

  uint8_t lsf_chunk[6][48]; //40 bit chunks of link information spread across 6 frames
  uint8_t m17_lsf[240];    //the complete LSF

  memset (lsf_chunk, 0, sizeof(lsf_chunk));
  memset (m17_lsf, 0, sizeof(m17_lsf));

  //NOTE: Most lich and lsf_chunk bits can be pre-set before the while loop,
  //only need to refresh the lich_cnt value, nonce, and golay
  uint16_t lsf_ps   = 1; //packet or stream indicator bit
  uint16_t lsf_dt  = st; //stream type
  uint16_t lsf_et   = 0; //encryption type
  uint16_t lsf_es   = 0; //encryption sub-type
  uint16_t lsf_cn = can; //can value
  uint16_t lsf_rs   = 0; //reserved bits

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

  //SEND CONN to reflector
  if (use_ip == 1)
    udp_return = m17_socket_blaster (super, 11, conn);

  //TODO: Read UDP ACKN/NACK value, disable use_ip if NULL or nack return
  
  //load dst and src values into the LSF
  for (i = 0; i < 48; i++) m17_lsf[i] = (dst >> (47ULL-(unsigned long long int)i)) & 1;
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

  //pack and compute the CRC16 for LSF
  uint16_t crc_cmp = 0;
  uint8_t lsf_packed[30];
  memset (lsf_packed, 0, sizeof(lsf_packed));
  for (i = 0; i < 28; i++)
      lsf_packed[i] = (uint8_t)ConvertBitIntoBytes(&m17_lsf[i*8], 8);
  crc_cmp = crc16(lsf_packed, 28);

  //attach the crc16 bits to the end of the LSF data
  for (i = 0; i < 16; i++) m17_lsf[224+i] = (crc_cmp >> (15-i)) & 1;

  //pack the CRC
  for (i = 28; i < 30; i++)
      lsf_packed[i] = (uint8_t)ConvertBitIntoBytes(&m17_lsf[i*8], 8);

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

  while (!exitflag) //while the software is running
  {

    // if not decoding internally, assign values for ncurses display
    if (super->opts.monitor_encode_internally == 0)
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
        super->m17d.meta[i] = 0;
      if (lsf_et == 2)
      {
        for (i = 0; i < 14; i++)
          super->m17d.meta[i] = nonce[i];
        for (i = 0; i < 8; i++)
        {
          super->m17d.meta[14] <<= 1;
          super->m17d.meta[15] <<= 1;
          super->m17d.meta[14] += ((fsn >> 7) >> (7-i)) & 1;
          super->m17d.meta[15] += ((fsn >> 0) >> (7-i)) & 1;
        }
      }
    }

    //read in short audio input samples from source
    for (i = 0; i < (int)nsam; i++)
    {
      for (j = 0; j < dec; j++)
        sample = get_short_audio_input_sample(super);
      voice1[i] = sample; //only store the 6th sample
    }

    if (st == 2)
    {
      for (i = 0; i < (int)nsam; i++)
      {
        for (j = 0; j < dec; j++)
          sample = get_short_audio_input_sample(super);
        voice2[i] = sample; //only store the 6th sample
      }
    }

    //read in RMS value for vox function; NOTE: will not work correctly SOCAT STDIO TCP due to blocking when no samples to read
    if (super->opts.use_pa_input != 3)
      super->demod.input_rms = raw_rms(voice1, nsam, 1) / 2; //dividing by two so mic isn't so sensitive on vox

    // //low pass filter
    // if (super->opts.use_lpf == 1)
    // {
    //   lpf (state, voice1, 160);
    //   if (st == 2)
    //     lpf (state, voice2, 160);
    // }

    // //high pass filter
    // if (super->opts.use_hpf == 1)
    // {
    //   hpf (state, voice1, 160);
    //   if (st == 2)
    //     hpf (state, voice2, 160);
    // }
    
    // //passband filter
    // if (super->opts.use_pbf == 1)
    // {
    //   pbf (state, voice1, 160);
    //   if (st == 2)
    //     pbf (state, voice2, 160);
    // }

    //manual gain control
    // if (super->opts.audio_gainA > 0.0f)
    // {
    //   analog_gain (opts, state, voice1, 160);
    //   if (st == 2)
    //     analog_gain (opts, state, voice2, 160);
    // }

    //automatic gain control
    // else
    // {
    //   agsm (opts, state, voice1, 160);
    //   if (st == 2)
    //     agsm (opts, state, voice2, 160);
    // }

    //convert out audio input into CODEC2 (3200bps) 8 byte data stream
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

    //tally consecutive squelch hits based on RMS value, or reset
    if (super->demod.input_rms > super->demod.input_sql) sql_hit = 0;
    else sql_hit++; //may eventually roll over to 0 again 

    //if vox enabled, toggle tx/eot with sql_hit comparison
    if (super->m17e.str_encoder_vox == 1)
    {
      if (sql_hit > 10 && lich_cnt == 0) //licn_cnt 0 to prevent new LSF popping out
      {
        super->m17e.str_encoder_tx = 0;
        eot = 1;
      }
      else
      {
        super->m17e.str_encoder_tx = 1;
        eot = 0;
      }
    }

    //set end of tx bit on the exitflag (sig, results not gauranteed) or toggle eot flag (always triggers)
    if (exitflag) eot = 1;
    if (super->m17e.str_encoder_eot) eot = 1;
    m17_v1[0] = (uint8_t)eot; //set as first bit of the stream

    //set current frame number as bits 1-15 of the v1 stream
    for (i = 0; i < 15; i++)
      m17_v1[i+1] = ( (uint8_t)(fsn >> (14-i)) ) &1;

    //Use the convolutional encoder to encode the voice / data stream
    simple_conv_encoder (m17_v1, m17_v1c, 148); //was 144, not 144+4

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

    //load up the lsf chunk for this cnt
    for (i = 0; i < 40; i++)
      lsf_chunk[lich_cnt][i] = m17_lsf[((lich_cnt)*40)+i];

    //update lich_cnt in the current LSF chunk
    lsf_chunk[lich_cnt][40] = (lich_cnt >> 2) & 1;
    lsf_chunk[lich_cnt][41] = (lich_cnt >> 1) & 1;
    lsf_chunk[lich_cnt][42] = (lich_cnt >> 0) & 1;

    //This is not M17 standard, but use the LICH reserved bits to signal can and dt
    // lsf_chunk[lich_cnt][43] = (lsf_cn >> 2) & 1;
    // lsf_chunk[lich_cnt][44] = (lsf_cn >> 1) & 1;
    // lsf_chunk[lich_cnt][45] = (lsf_cn >> 0) & 1;

    // lsf_chunk[lich_cnt][46] = (lsf_dt >> 1) & 1;
    // lsf_chunk[lich_cnt][47] = (lsf_dt >> 0) & 1;

    //encode with golay 24,12 and load into m17_l1g
    Golay_24_12_encode (lsf_chunk[lich_cnt]+00, m17_l1g+00);
    Golay_24_12_encode (lsf_chunk[lich_cnt]+12, m17_l1g+24);
    Golay_24_12_encode (lsf_chunk[lich_cnt]+24, m17_l1g+48);
    Golay_24_12_encode (lsf_chunk[lich_cnt]+36, m17_l1g+72);

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

        fprintf (stderr, "\n M17 LSF    (ENCODER): ");
        if (super->opts.monitor_encode_internally == 1)
          demod_lsf(super, m17_lsfs, 1);
        else fprintf (stderr, " To Audio Out;");

        //convert bit array into symbols and RF/Audio
        memset (nil, 0, sizeof(nil));
        encodeM17RF (super,      nil, mem, 11); //Preamble
        encodeM17RF (super, m17_lsfs, mem, 1); //LSF

        //flag off after sending
        new_lsf = 0;

        //flag to indicate to send one eot afterwards
        eot_out = 0;
      }

      fprintf (stderr, "\n M17 Stream (ENCODER): ");
      if (super->opts.monitor_encode_internally == 1)
        demod_str(super, m17_t4s, 1);
      else fprintf (stderr, " To Audio Out;");

      //show UDP if active
      if (use_ip == 1 && lich_cnt != 5)
        fprintf (stderr, " UDP: %s:%d", super->opts.m17_hostname, udpport);

      //debug RMS Value
      if (super->m17e.str_encoder_vox == 1)
      {
        fprintf (stderr, " RMS: %04ld", super->demod.input_rms);
        fprintf (stderr, " SQL: %04ld", super->demod.input_sql);
        fprintf (stderr, " SQL HIT: %d;", sql_hit);
      }

      //debug show pulse input latency
      if (super->opts.use_pa_input == 1 && super->opts.payload_verbosity >= 3)
      {
        unsigned long long int latency = pa_simple_get_latency (super->pa.pa_input_device, NULL);
        fprintf (stderr, " Latency: %05lld;", latency);
      }

      //convert bit array into symbols and RF/Audio
      encodeM17RF (super, m17_t4s, mem, 2);
      
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
        m17_ip_packed[i] = (uint8_t)ConvertBitIntoBytes(&m17_ip_frame[i*8], 8);
      ip_crc = crc16(m17_ip_packed, 52);

      //add CRC value to the ip frame
      for (i = 0; i < 16; i++)
        m17_ip_frame[k++] = (ip_crc >> (15-i))&1;
      
      //pack CRC into the byte array as well
      for (i = 52; i < 54; i++)
        m17_ip_packed[i] = (uint8_t)ConvertBitIntoBytes(&m17_ip_frame[i*8], 8);

      //Send packed IP frame to UDP port if enabled
      if (use_ip == 1)
        udp_return = m17_socket_blaster (super, 54, m17_ip_packed);

      //increment lich_cnt, reset on 6
      lich_cnt++;
      if (lich_cnt == 6) lich_cnt = 0;

      //increment frame sequency number, trunc to maximum value, roll nonce if needed
      fsn++;
      if (fsn > 0x7FFF)
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
        m17_ip_packed[i] = (uint8_t)ConvertBitIntoBytes(&m17_ip_frame[i*8], 8);
      ip_crc = crc16(m17_ip_packed, 52);

      //add CRC value to the ip frame
      for (i = 0; i < 16; i++)
        m17_ip_frame[k++] = (ip_crc >> (15-i))&1;

      //pack CRC into the byte array as well
      for (i = 52; i < 54; i++)
        m17_ip_packed[i] = (uint8_t)ConvertBitIntoBytes(&m17_ip_frame[i*8], 8);

      //reset 
      lich_cnt = 0;
      fsn = 0;
      super->demod.in_sync = 0;

      //update timestamp
      ts = time(NULL);

      //update randomizer seed and SID
      srand(ts); //randomizer seed based on time

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
      if (lsf_et == 2) //disable to allow the 0x69 repeating non-zero fill on RES
      {
        for (i = 0; i < 112; i++) m17_lsf[i+112] = iv[i];
      }

      //repack, new CRC, and update rest of lsf as well
      memset (lsf_packed, 0, sizeof(lsf_packed));
      for (i = 0; i < 28; i++)
        lsf_packed[i] = (uint8_t)ConvertBitIntoBytes(&m17_lsf[i*8], 8);
      crc_cmp = crc16(lsf_packed, 28);

      //attach the crc16 bits to the end of the LSF data
      for (i = 0; i < 16; i++) m17_lsf[224+i] = (crc_cmp >> (15-i)) & 1;

      //repack the CRC
      for (i = 28; i < 30; i++)
          lsf_packed[i] = (uint8_t)ConvertBitIntoBytes(&m17_lsf[i*8], 8);

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

      //flush the last frame with the eot bit on
      if (eot && !eot_out)
      {
        fprintf (stderr, "\n M17 Stream (ENCODER): ");
        if (super->opts.monitor_encode_internally == 1)
          demod_str(super, m17_t4s, 1);
        else fprintf (stderr, " To Audio Out;");

        //show UDP if active
        if (use_ip == 1 && lich_cnt != 5)
          fprintf (stderr, " UDP: %s:%d", super->opts.m17_hostname, udpport);

        //debug RMS Value
        if (super->m17e.str_encoder_vox == 1)
        {
          fprintf (stderr, " RMS: %04ld", super->demod.input_rms);
          fprintf (stderr, " SQL: %04ld", super->demod.input_sql);
          fprintf (stderr, " SQL HIT: %d;", sql_hit);
        }

        //convert bit array into symbols and RF/Audio
        encodeM17RF (super, m17_t4s, mem, 2); //Last Stream Frame
        memset (nil, 0, sizeof(nil));
        encodeM17RF (super, nil, mem, 55);    //EOT Marker

        //send dead air with type 99
        memset (nil, 0, sizeof(nil));
        for (i = 0; i < 25; i++)
          encodeM17RF (super, nil, mem, 99);

        //send IP Frame with EOT bit
        if (use_ip == 1)
          udp_return = m17_socket_blaster (super, 54, m17_ip_packed);

        //SEND EOTX to reflector
        if (use_ip == 1)
          udp_return = m17_socket_blaster (super, 10, eotx);

        //reset indicators
        eot = 0;
        eot_out = 1;
        super->m17e.str_encoder_eot = 0;
      }

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
    
  }

  //SEND EOTX to reflector
  // if (use_ip == 1)
  //   udp_return = m17_socket_blaster (super, 10, eotx);

  //SEND DISC to reflector
  if (use_ip == 1)
    udp_return = m17_socket_blaster (super, 10, disc);
  
  //free allocated memory
  free(samp1);
  free(samp2);

}