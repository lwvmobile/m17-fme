/*-------------------------------------------------------------------------------
 * m17_pkt_encoder.c
 * M17 Project - Packet Mode Encoder
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void encode_pkt(Super * super)
{

  //quell defined but not used warnings from m17.h
  stfu ();

  float mem[81];

  //initialize RRC memory buffer
  memset (mem, 0, 81*sizeof(float));

  uint8_t nil[368]; //empty array
  memset (nil, 0, sizeof(nil));

  int i, j, k, x; //basic utility counters

  //User Defined Variables
  uint8_t can = 7; //channel access number
  //numerical representation of dst and src after b40 encoding, or special/reserved value
  unsigned long long int dst = 0;
  unsigned long long int src = 0;
  //DST and SRC Callsign Data (pick up to 9 characters from the b40 char array)
  char d40[50] = "M17-FME  "; //DST
  char s40[50] = "M17-FME  "; //SRC

  //Default
  // char text[800] = "This is a simple SMS text message sent over M17 Packet Data.";

  //short
  //NOTE: Working on full payload w/o padding
  // char text[800] = "Lorem";

  //medium
  //NOTE: Fixed w/ the pad < 1, then add a block (not just if 0)
  // char text[800] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";

  //large
  //NOTE: Working on full payload w/o padding
  char text[800] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

  //Preamble of the Declaration of Independence (U.S.A.)
  //NOTE: Fixed again with block != 31 check and manual terminator insertion into text byte 772
  // char text[] = "When in the Course of human events, it becomes necessary for one people to dissolve the political bands which have connected them with another, and to assume among the powers of the earth, the separate and equal station to which the Laws of Nature and of Nature's God entitle them, a decent respect to the opinions of mankind requires that they should declare the causes which impel them to the separation. We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness.--That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, --That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness. Prudence, indeed, will dictate that Governments long established should not be changed for light and transient causes; and accordingly all experience hath shewn, that mankind are more disposed to suffer, while evils are sufferable, than to right themselves by abolishing the forms to which they are accustomed. But when a long train of abuses and usurpations, pursuing invariably the same Object evinces a design to reduce them under absolute Despotism, it is their right, it is their duty, to throw off such Government, and to provide new Guards for their future security.--Such has been the patient sufferance of these Colonies; and such is now the necessity which constrains them to alter their former Systems of Government. The history of the present King of Great Britain is a history of repeated injuries and usurpations, all having in direct object the establishment of an absolute Tyranny over these States. To prove this, let Facts be submitted to a candid world.";

  //end User Defined Variables

  //configure User Defined Variables, if defined at CLI
  if (super->m17e.can != -1) //has a set value
    can = super->m17e.can;

  if (super->m17e.srcs[0] != 0)
    sprintf (s40, "%s", super->m17e.srcs);

  if (super->m17e.dsts[0] != 0)
    sprintf (d40, "%s", super->m17e.dsts);

  //SMS Message OR Raw Encoded Data Format
  uint8_t protocol  = 0x05;
  if (super->m17e.sms[0] != 0)
  {
    protocol = 0x05; //SMS Protocol
    sprintf (text, "%s", super->m17e.sms);
  }
  else if (super->m17e.raw[0] != 0)
    protocol = super->m17e.raw[1];

  //if special values, then assign them
  if (strcmp (d40, "BROADCAST") == 0)
    dst = 0xFFFFFFFFFFFF;
  if (strcmp (d40, "ALL") == 0)
    dst = 0xFFFFFFFFFFFF;

  //end CLI Configuration

  //send dead air with type 99
  for (i = 0; i < 25; i++)
    encode_rfa (super, nil, mem, 99);

  //send preamble_a for the LSF frame
  // encode_rfa (super, nil, mem, 11); //don't need to send twice, had wrong type anyways

  //NOTE: PKT mode does not seem to have an IP format specified by M17 standard,
  //so I will assume that you do not send PKT data over IP to a reflector

  uint8_t m17_lsf[244]; //the complete LSF + 4 trailing bits
  memset (m17_lsf, 0, sizeof(m17_lsf));

  //Setup LSF Variables, these are not sent in chunks like with voice
  //but only once at start of PKT TX
  uint16_t lsf_ps   = 0; //packet or stream indicator bit
  uint16_t lsf_dt   = 1; //Data
  uint16_t lsf_et   = super->enc.enc_type;    //encryption type
  uint16_t lsf_es   = super->enc.enc_subtype; //encryption sub-type
  uint16_t lsf_cn = can; //can value
  uint16_t lsf_rs   = 0; //reserved bits

  if (lsf_et == 1)
  {
    scrambler_key_init (super, 1);
    lsf_es = super->enc.enc_subtype; //encryption sub-type
  }

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

  //load dst and src values into the LSF
  for (i = 0; i < 48; i++) m17_lsf[i] = (dst >> (47ULL-(unsigned long long int)i)) & 1;
  for (i = 0; i < 48; i++) m17_lsf[i+48] = (src >> (47ULL-(unsigned long long int)i)) & 1;

  //TODO: Any extra meta fills (extended callsign, etc?)

  //NONCE
  time_t epoch = 1577836800L;      //Jan 1, 2020, 00:00:00 UTC
  time_t ts = time(NULL) - epoch;  //timestamp since epoch
  srand(ts); //randomizer seed based on timestamp

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

  //leaving disabled, but code is present now if we wish to run and convert ECB to IV mode
  //NOTE: Currently, LSF frame decode on RF Audio Input is 50/50 with an IV present
  //Another thing to consider, if we don't use the CTR (no LSN) then its technically not CTR mode

  //load nonce into the IV field of the m17_lsf and mirror to m17e.meta for aes crypt function
  // if (super->enc.enc_type == 2 && super->enc.aes_key_is_loaded)
  // {
  //   k = 112;
  //   for (j = 0; j < 14; j++)
  //   {
  //     for (i = 0; i < 8; i++)
  //       m17_lsf[k++] = (nonce[j] >> (7-i))&1;
  //   }

  //   //copy nonce to meta for crypt function below
  //   memcpy (super->m17e.meta, nonce, 14);
  // }
  //end load

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

  //a full sized complete packet paylaod to break into smaller frames
  uint8_t m17_p1_full[31*200]; memset (m17_p1_full, 0, sizeof(m17_p1_full));

  //load protocol value into first 8 bits
  k = 0;
  for (i = 0; i < 8; i++)
    m17_p1_full[k++] = (protocol >> (7-i)) & 1; 

  //byte representation of a single string char
  uint8_t cbyte;

  //counter values
  int tlen = strlen((const char*)text);
  int block = 0; //number of blocks in total
  int ptr = 0;  //ptr to current position of the text
  int pad = 0; //amount of padding to apply to last frame
  int lst = 0; //amount of significant octets in the last block

  //encode elements
  uint8_t pbc = 0; //packet/octet counter
  uint8_t eot = 0; //end of tx bit

  //sanity check, if tlen%25 is 23 or 24, need to increment to another block value
  if ( (tlen%25) > 23) tlen += (tlen%23) + 1;

  //sanity check, maximum strlen should not exceed 771 for a full encode
  if (tlen > 771) tlen = 771;

  //insert a zero byte as the terminator
  text[tlen++] = 0x00;

  //insert one at the last available byte position
  text[772] = 0x00;

  //debug tlen value
  // fprintf (stderr, " STRLEN: %d; ", tlen);
  
  //if user passed a raw data string
  if (super->m17e.raw[0] != 0)
  {
    fprintf (stderr, "\n Protocol: %02X;", protocol);
    fprintf (stderr, "\n Octets:");
    tlen = (int)super->m17e.raw_len;
    for (i = 2; i < tlen; i++)
    {
      fprintf (stderr, " %02X", super->m17e.raw[i]);
      for (j = 0; j < 8; j++)
        m17_p1_full[k++] = (super->m17e.raw[i] >> (7-j)) & 1;

      ptr++;

      //add line break to keep it under 80 columns
      if ( (i%71) == 0 && i != 0)
        fprintf (stderr, "\n        ");
    }

    fprintf (stderr, "\n");
  }

  //Convert a string text message into UTF-8 octets and load into full if using SMS protocol
  else //if (protocol == 0x05) //send an SMS message instead (default text or user supplied sms string)
  {
    fprintf (stderr, "\n SMS: ");
    for (i = 0; i < tlen; i++)
    {
      cbyte = (uint8_t)text[ptr];
      fprintf (stderr, "%c", cbyte);

      for (j = 0; j < 8; j++)
        m17_p1_full[k++] = (cbyte >> (7-j)) & 1;

      if (cbyte == 0) break; //if terminator reached

      ptr++; //increment pointer
      

      //add line break to keep it under 80 columns
      // if ( (i%71) == 0 && i != 0)
      //   fprintf (stderr, "\n      ");
    }
    fprintf (stderr, "\n");
  }
  //end UTF-8 Encoding

  //calculate blocks, pad, and last values for pbc
  block = (ptr / 25) + 1;
  pad = (block * 25) - ptr - 4;
  // if (pad == 0 && block != 31) //fallback if issues arise
  if (pad < 1 && block != 31)
  {
    block++;
    pad = (block * 25) - ptr - 4;
  }
  lst = 23-pad+2; //pbc value for last block out
  if (super->m17e.raw[0] != 0) lst--; //trim lacking terminating byte if not SMS

  //sanity check block value
  // if (block > 31) block = 31;
  
  //debug position values
  if (super->opts.payload_verbosity > 0)
    fprintf (stderr, "\n BLOCK: %02d; PAD: %02d; LST: %d; K: %04d; PTR: %04d;", block, pad, lst, k, ptr);

  //apply encryption keystream to m17_p1_full at this point, after protocol byte, and prior to terminating byte and CRC
  if (super->enc.enc_type == 1 && super->enc.scrambler_key)
  {
    //mew method
    super->enc.scrambler_seed_e = scrambler_sequence_generator(super, 1);
    int z = 0;
    for (i = 8; i < (k-8); i++) //k should be at the correct position here //k-9
    {
      m17_p1_full[i] ^= super->enc.scrambler_pn[z++];
      if (z == 128)
      {
        super->enc.scrambler_seed_e = scrambler_sequence_generator(super, 1);
        z = 0;
      }
    }
      

    //old method
    // for (i = 8; i < (k-8); i++) //k should be at the correct position here //k-9
    //   m17_p1_full[i] ^= super->enc.scrambler_pn[i%768]; //I think this could have been an issue previously
  }

  else if (super->enc.enc_type == 2 && super->enc.aes_key_is_loaded)
  {
    int klen = (k-8)/128; //NOTE: This will fall short by % value octets
    int kmod = (k-8)%128; //This is how many bits we are short, so we need to account with a partial ks application

    //NOTE: Without a proper IV, this is effectively turning AES-CTR into AES-ECB mode
    //I should load a proper IV, but if the LSF fails, then we can't recover packets
    //unlike voice, where we have a rolling embedded link data with an IV in it
    //I may consider leaving this as ECB mode (less secure, but more secure than scrambler)

    for (i = 0; i < klen; i++)
      aes_ctr_str_payload_crypt (super->m17e.meta, super->enc.aes_key, m17_p1_full+(128*i)+8, super->enc.enc_subtype+1);

    //if there are leftovers (kmod), then run a keystream and partial application to left over bits
    uint8_t aes_ks_bits[128]; memset(aes_ks_bits, 0, 128*sizeof(uint8_t));
    int kmodstart = klen*128;
    
    //set to 8 IF kmodstart == 0 so we don't encrypt the protocol byte on short single block packets
    if (kmodstart == 0) kmodstart = 8;

    //debug
    // fprintf (stderr, " AES KLEN: %d; KMOD: %d; KMODSTART: %d; ", klen, kmod, kmodstart);

    if (kmod != 0)
    {
      aes_ctr_str_payload_crypt (super->m17e.meta, super->enc.aes_key, aes_ks_bits, super->enc.enc_subtype+1);
      for (i = 0; i < kmod; i++)
        m17_p1_full[i+kmodstart] ^= aes_ks_bits[i];
    }

  }

  //Calculate the CRC and attach it here
  x = 0;
  uint8_t m17_p1_packed[31*25]; memset (m17_p1_packed, 0, sizeof(m17_p1_packed));
  for (i = 0; i < 25*31; i++)
  {
    m17_p1_packed[x] = (uint8_t)convert_bits_into_output(&m17_p1_full[i*8], 8);
    // if (m17_p1_packed[x] == 0) break; //stop at the termination byte (may/will not work correctly on raw data with 00 in it, or enc key if result is 00)
    if (x == ptr+1) break; //stop after full read in based on ptr+1 value
    x++;
  }

  //NOTE to self: Revert changes in this commit, if issues with CRC on PKT, or
  //the CRC really does go on the last 16 bits of the payload

  //debug dump the packed payload up to x, or x+1
  // fprintf (stderr, "\n P1P:");
  // for (i = 0; i < x+1; i++)
  //   fprintf (stderr, "%02X", m17_p1_packed[i]);
  // fprintf (stderr, "\n");

  if (super->m17e.raw[0] != 0)
    crc_cmp = crc16(m17_p1_packed, x); //x, no terminating 0x00 byte on raw
  else
    crc_cmp = crc16(m17_p1_packed, x+1); //x+1 for terminating 0x00 byte inclusion

  //debug dump CRC (when pad is literally zero)
  if (super->opts.payload_verbosity > 0)
    fprintf (stderr, "\n X: %d; LAST: %02X; TERM: %02X; CRC: %04X; \n", x, m17_p1_packed[x-1], m17_p1_packed[x], crc_cmp);

  ptr = (block*25*8) - 16;

  //attach the crc16 bits to the end of the PKT data
  // for (i = 0; i < 16; i++) m17_p1_full[ptr+i] = (crc_cmp >> 15-i) & 1; //this one puts it as the last 16-bits of the full payload

  for (i = 0; i < 16; i++) m17_p1_full[k++] = (crc_cmp >> (15-i)) & 1; //this one puts it immediately after the terminating byte

  //debug the full payload
  fprintf (stderr, "\n M17 Packet      FULL: ");
  for (i = 0; i < 25*block; i++)
  {
    if ( (i%25) == 0 && i != 0 ) fprintf (stderr, "\n                       ");
    fprintf (stderr, "%02X", (uint8_t)convert_bits_into_output(&m17_p1_full[i*8], 8));
  }
  fprintf (stderr, "\n");

  //just lump all the UDP IP Frame stuff together and one-shot it
  int use_ip = 0; //1 to enable IP Frame Broadcast over UDP
  uint8_t reflector_module = super->m17e.reflector_module;

  //Open UDP port to default or user defined values, if enabled
  int sock_err = 0;
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

  //NOTE: IP Framing is not standard on M17 for PKT mode, but
  //I don't see any reason why we can't send them anyways, just
  //need to use a new magic for it: MPKT. The receiver here is capable
  //of decoding them

  //Standard IP Framing
  uint8_t mpkt[4]  = {0x4D, 0x50, 0x4B, 0x54};
  uint8_t ackn[4]  = {0x41, 0x43, 0x4B, 0x4E}; UNUSED(ackn);
  uint8_t nack[4]  = {0x4E, 0x41, 0x43, 0x4B}; UNUSED(nack);
  uint8_t conn[11]; memset (conn, 0, sizeof(conn));
  uint8_t disc[10]; memset (disc, 0, sizeof(disc));
  uint8_t ping[10]; memset (ping, 0, sizeof(ping));
  uint8_t pong[10]; memset (pong, 0, sizeof(pong));
  uint8_t eotx[10]; memset (eotx, 0, sizeof(eotx));
  int udp_return = 0; UNUSED(udp_return);
  uint8_t sid[2];   memset (sid, 0, sizeof(sid));
  uint8_t  m17_ip_frame[8000]; memset (m17_ip_frame, 0, sizeof(m17_ip_frame));
  uint8_t m17_ip_packed[25*40]; memset (m17_ip_packed, 0, sizeof(m17_ip_packed));
  uint16_t ip_crc = 0;

  //Setup conn, disc, eotx, ping, pong values
  conn[0] = 0x43; conn[1] = 0x4F; conn[2] = 0x4E; conn[3] = 0x4E; conn[10] = reflector_module;
  disc[0] = 0x44; disc[1] = 0x49; disc[2] = 0x53; disc[3] = 0x43;
  ping[0] = 0x50; ping[1] = 0x49; ping[2] = 0x4E; ping[3] = 0x47;
  pong[0] = 0x50; pong[1] = 0x4F; pong[2] = 0x4E; pong[3] = 0x47;
  eotx[0] = 0x45; eotx[1] = 0x4F; eotx[2] = 0x54; eotx[3] = 0x58;

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

  //add MPKT header
  k = 0;
  for (j = 0; j < 4; j++)
  {
    for (i = 0; i < 8; i++)
      m17_ip_frame[k++] = (mpkt[j] >> (7-i)) &1;
  }

  //randomize ID
  srand(ts);
  sid[0] = rand() & 0xFF;
  sid[1] = rand() & 0xFF;

  //add StreamID / PKT ID
  for (j = 0; j < 2; j++)
  {
    for (i = 0; i < 8; i++)
      m17_ip_frame[k++] = (sid[j] >> (7-i)) &1;
  }

  //add the current LSF, sans CRC
  for (i = 0; i < 224; i++) //28 bytes
    m17_ip_frame[k++] = m17_lsf[i];

  //pack current bit array to current
  for (i = 0; i < 34; i++)
    m17_ip_packed[i] = (uint8_t)convert_bits_into_output(&m17_ip_frame[i*8], 8);

  //pack the entire PKT payload (plus terminator, sans CRC)
  for (i = 0; i < x+1; i++)
    m17_ip_packed[i+34] = (uint8_t)convert_bits_into_output(&m17_p1_full[i*8], 8);

  //Calculate CRC over everthing packed (including the terminator)
  ip_crc = crc16(m17_ip_packed, 34+1+x);

  //add CRC value to the ip frame
  uint8_t crc_bits[16]; memset (crc_bits, 0, sizeof(crc_bits));
  for (i = 0; i < 16; i++)
    crc_bits[i] = (ip_crc >> (15-i))&1;

  //pack CRC into the byte array as well
  for (i = x+34+1, j = 0; i < (x+34+3); i++, j++) //double check this
    m17_ip_packed[i] = (uint8_t)convert_bits_into_output(&crc_bits[j*8], 8);


  //NOTE: Fixed recvfrom limitation, MSG_WAITALL seems to be 256
  //manually inserted 1000 into recvfrom instead, max MPKT size should be 809.

  //send the OTA key
  if (super->opts.use_otakd == 1)
  {
    if (super->enc.enc_type != 0)
      encode_ota_key_delivery_pkt(super, use_ip, sid);
  }

  //Send MPKT to reflector
  if (use_ip == 1)
    udp_return = m17_socket_blaster (super, x+34+3, m17_ip_packed);

  //debug
  if (use_ip == 1)
    fprintf (stderr, "\n UDP IP Frame CRC: %04X; UDP RETURN: %d: X: %d; SENT: %d;", ip_crc, udp_return, x, x+34+3);

  //SEND EOTX to reflector
  if (use_ip == 1)
    udp_return = m17_socket_blaster (super, 10, eotx);

  //SEND DISC to reflector
  if (use_ip == 1)
    udp_return = m17_socket_blaster (super, 10, disc);

  //flag to determine if we send a new LSF frame for new encode
  //only send once at the appropriate time when encoder is toggled on
  int new_lsf = 1;

  while (!exitflag)
  {
    //send LSF frame once, if new encode session
    if (new_lsf == 1)
    {

      fprintf (stderr, "\n M17 LSF    (ENCODER): ");
      demod_lsf(super, m17_lsfs, 1);

      //convert bit array into symbols and RF/Audio
      memset (nil, 0, sizeof(nil));
      encode_rfa (super, nil, mem, 11); //Preamble
      // for (i = 0; i < 2; i++)
        encode_rfa (super, m17_lsfs, mem, 1); //LSF

      //flag off after sending
      new_lsf = 0;

      //reset ptr value to use during chunk loading
      ptr = 0;
    }

    //PKT - 206 bits of Packet Data + 4 trailing bits
    uint8_t m17_p1[210]; memset (m17_p1, 0, sizeof(m17_p1));

    //PKT - 420 bits of Packet Data (after Convolutional Encode)
    uint8_t m17_p2c[420]; memset (m17_p2c, 0, sizeof(m17_p2c));

    //PKT - 368 bits of Packet Data (after P2 Puncturing)
    uint8_t m17_p3p[368]; memset (m17_p3p, 0, sizeof(m17_p3p));

    //PKT - 368 bits of Packet Data (after Interleaving)
    uint8_t m17_p4i[368]; memset (m17_p4i, 0, sizeof(m17_p4i));

    //PKT - 368 bits of Packet Data (after Scrambling)
    uint8_t m17_p4s[368]; memset (m17_p4s, 0, sizeof(m17_p4s));

    //Break the full payload into 25 octet chunks and load into p1
    for (i = 0; i < 200; i++)
      m17_p1[i] = m17_p1_full[ptr++];

    //Trigger EOT when out of data to encode
    if (ptr/8 >= block*25)
    {
      eot = 1;
      pbc = lst;
    }
    m17_p1[200] = eot;

    //set pbc counter to last 5 bits
    for (i = 0; i < 5; i++)
      m17_p1[201+i] = (pbc >> (4-i)) & 1;

    //Use the convolutional encoder to encode the packet data
    simple_conv_encoder (m17_p1, m17_p2c, 210); //206 + 4 trailing bits

    //P3 puncture
    x = 0;
    for (i = 0; i < 420; i++)
    {
      if (p3[i%8] == 1)
        m17_p3p[x++] = m17_p2c[i];
    }

    //debug X bit positions
    // fprintf (stderr, " X: %d", x);

    //interleave the bit array using Quadratic Permutation Polynomial
    //function π(x) = (45x + 92x^2 ) mod 368
    for (i = 0; i < 368; i++)
    {
      x = ((45*i)+(92*i*i)) % 368;
      m17_p4i[x] = m17_p3p[i];
    }

    //scramble/randomize the frame
    for (i = 0; i < 368; i++)
      m17_p4s[i] = (m17_p4i[i] ^ m17_scramble[i]) & 1;

    //-----------------------------------------


    fprintf (stderr, "\n M17 Packet (ENCODER): ");

    //Dump Output of the current Packet Frame
    for (i = 0; i < 26; i++)
      fprintf (stderr, "%02X", (uint8_t)convert_bits_into_output(&m17_p1[i*8], 8));

    //debug PBC
    // fprintf (stderr, " PBC: %d;", pbc);

    //convert bit array into symbols and RF/Audio
    encode_rfa (super, m17_p4s, mem, 4);

    //send the EOT Marker and some dead air
    if (eot)
    {
      memset (nil, 0, sizeof(nil));
      encode_rfa (super, nil, mem, 55); //EOT Marker

      //send dead air with type 99
      for (i = 0; i < 25; i++)
        encode_rfa (super, nil, mem, 99);

      //shut it down
      exitflag = 1;
    }

    //increment packet / byte counter
    pbc++;

  }
}