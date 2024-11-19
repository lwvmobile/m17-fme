/*-------------------------------------------------------------------------------
 * m17_str_encoder.c
 * M17 Project - Stream Voice Encoder
 *
 * LWVMOBILE
 * 2024-06 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

//encode and create audio of a M17 Project Stream signal
void encode_str(Super * super)
{
  float mem[81];

  //quell defined but not used warnings from m17.h
  stfu ();

  //initialize RRC memory buffer
  memset (mem, 0, 81*sizeof(float));

  //set stream type value here so we can change 3200 or 1600 accordingly
  uint8_t st = 2; //stream type: 0 = res; 1 = data; 2 = voice(3200); 3 = voice(1600) + data;
  if (super->opts.m17_str_encoder_dt == 3) st = 3; //this is set to 3 IF -S user text string is called at CLI
  else st = 2; //otherwise, just use 3200 voice

  //IP Frame Things and User Variables for Reflectors, etc
  uint8_t nil[368]; //empty array to send to RF during Preamble, EOT Marker, or Dead Air
  memset (nil, 0, sizeof(nil));

  //Enable TX
  super->m17e.str_encoder_tx = 1;

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
  //end CLI Configuration
  
  int i, j, k, x;    //basic utility counters
  short sample = 0;  //individual audio sample from source
  size_t nsam = 160; //number of samples to be read in (default is 160 samples for codec2 3200 bps)
  int dec = super->opts.input_sample_rate / 8000; //number of samples to run before selecting a sample from source input
  uint32_t sql_hit = 26; //squelch hits, hit enough, and deactivate vox
  int eot_out =  1; //if we have already sent the eot out once

  //send dead air with type 99
  for (i = 0; i < 25; i++)
    encode_rfa (super, nil, mem, 99);

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
  time_t epoch = 1577836800L;      //Jan 1, 2020, 00:00:00 UTC
  time_t ts = time(NULL) - epoch;  //timestamp since epoch
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

  //Encode Callsign Data
  encode_callsign_data(super, d40, s40, &dst, &src);

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

  while (!exitflag) //while the software is running
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

    //update CAN
    if (super->m17e.can != -1)
      can = super->m17e.can;
    lsf_cn = can;

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

    //read in RMS value for vox function;
    super->demod.input_rms = raw_rms(voice1, nsam, 1);

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

    //tally consecutive squelch hits based on RMS value, or reset
    if (super->demod.input_rms > super->demod.input_sql) sql_hit = 0;
    else sql_hit++;

    //sanity check to prevent roll over opening vox
    if (sql_hit > 65000) sql_hit = 30000;

    //if vox enabled, toggle tx/eot with sql_hit comparison
    if (super->m17e.str_encoder_vox == 1)
    {
      if (sql_hit > 25 && lich_cnt == 0) //licn_cnt 0 to prevent new LSF popping out
      {
        super->m17e.str_encoder_tx = 0;
        // eot = 1; //TODO: Trace down bug that causes this bit to cause a CRC error on LSF frames
      }
      else
      {
        super->m17e.str_encoder_tx = 1;
        eot = 0;
      }
    }

    //set end of tx bit on the exitflag (sig, results not gauranteed) or toggle eot flag (always triggers)
    if (exitflag) eot = 1; //TODO: Change this to end? and then if no ecdsa, then allow it set to 1?
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
        eot_out = 0;
      }

      fprintf (stderr, "\n M17 Stream (ENCODER): ");
      if (super->opts.internal_loopback_decoder == 1)
        demod_str(super, m17_t4s, 1);
      else fprintf (stderr, " To Audio Out: %s", super->pa.pa_outrf_idx);

      //show UDP if active
      if (use_ip == 1 && lich_cnt != 5)
        fprintf (stderr, " UDP: %s:%d", super->opts.m17_hostname, udpport);

      //debug RMS Value
      if (super->m17e.str_encoder_vox == 1)
      {
        fprintf (stderr, "\n");
        fprintf (stderr, " RMS: %04ld", super->demod.input_rms);
        fprintf (stderr, " SQL: %04ld", super->demod.input_sql);
        fprintf (stderr, " SQL HIT: %d;", sql_hit);
      }

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
        udp_return = m17_socket_blaster (super, 54, m17_ip_packed);

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

      //Encode Callsign Data
      encode_callsign_data(super, d40, s40, &dst, &src);

      //load dst and src values into the LSF
      for (i = 0; i < 48; i++) m17_lsf[i+00] = (dst >> (47ULL-(unsigned long long int)i)) & 1;
      for (i = 0; i < 48; i++) m17_lsf[i+48] = (src >> (47ULL-(unsigned long long int)i)) & 1;

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

        //debug RMS Value
        if (super->m17e.str_encoder_vox == 1)
        {
          fprintf (stderr, "\n");
          fprintf (stderr, " RMS: %04ld", super->demod.input_rms);
          fprintf (stderr, " SQL: %04ld", super->demod.input_sql);
          fprintf (stderr, " SQL HIT: %d;", sql_hit);
        }

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
          udp_return = m17_socket_blaster (super, 54, m17_ip_packed);

        //SEND EOTX to reflector
        if (use_ip == 1)
          udp_return = m17_socket_blaster (super, 10, eotx);

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
          udp_return = m17_socket_blaster (super, 10, eotx);

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

  //SEND EOTX to reflector
  // if (use_ip == 1)
  //   udp_return = m17_socket_blaster (super, 10, eotx);

  //SEND DISC to reflector
  if (use_ip == 1)
    udp_return = m17_socket_blaster (super, 10, disc);
  
  //free allocated memory
  free(samp1);
  free(samp2);

  //get rid of this if it costs too much CPU / skips / lags
  super->demod.current_time = time(NULL);

}