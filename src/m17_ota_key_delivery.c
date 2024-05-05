/*-------------------------------------------------------------------------------
 * m17_ota_key_delivery.c
 * Project M17 - Over the Air Key Delivery Format Encoding
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

//NOTE: This format is not M17 Specification, this is a new specification for M17-FME.
//The idea with this format is that while we can encrypt and decrypt stream and data packets
//M17 is meant to be a HAM radio tool, and M17-FME is meant to be a learning tool, so while
//we can encrypt and decrypt, I feel like we should do it as a technical tool for learning,
//but at the same time, we should also freely share that key for others to be able to use, this
//also works with some laws that require the publishing of enc if used on HAM bands since the
//source code of HOW it is encrypted is plainly available AND the key is transmitted openly OTA.

void encode_ota_key_delivery(Super * super)
{

  //quell defined but not used warnings from m17.h
  stfu ();

  int start = 0;

  float mem[81];

  //initialize RRC memory buffer
  memset (mem, 0, 81*sizeof(float));

  uint8_t nil[368]; //empty array
  memset (nil, 0, sizeof(nil));

  int i, j, k, x; //basic utility counters

  //OTA Key Delivery Protocol
  uint8_t protocol = 9;

  //User Defined Variables
  uint8_t can = 7; //channel access number
  //numerical representation of dst and src after b40 encoding, or special/reserved value
  unsigned long long int dst = 0;
  unsigned long long int src = 0;
  //DST and SRC Callsign Data (pick up to 9 characters from the b40 char array)
  char d40[50] = "M17-FME  "; //DST
  char s40[50] = "M17-FME  "; //SRC

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

  //end CLI Configuration

  uint8_t m17_lsf[240];
  memset (m17_lsf, 0, sizeof(m17_lsf));

  //Setup LSF Variables, these are not sent in chunks like with voice
  //but only once at start of PKT TX
  uint16_t lsf_ps   = 0; //packet or stream indicator bit
  uint16_t lsf_dt   = 1; //Data
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

  //load dst and src values into the LSF
  for (i = 0; i < 48; i++) m17_lsf[i] = (dst >> (47ULL-(unsigned long long int)i)) & 1;
  for (i = 0; i < 48; i++) m17_lsf[i+48] = (src >> (47ULL-(unsigned long long int)i)) & 1;

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

  //load zero fill, enc type, and send sequence number
  uint8_t enc_type = super->enc.enc_type;
  uint8_t ssn = 2; //sending over a packet, so this will always be 2

  for (i = 0; i < 4; i++)
    m17_p1_full[k++] = 0;

  m17_p1_full[k++] = (enc_type >> 1) & 1;
  m17_p1_full[k++] = (enc_type >> 0) & 1;
  m17_p1_full[k++] = (ssn >> 1) & 1;
  m17_p1_full[k++] = (ssn >> 0) & 1;

  //if scrambler key, load 24-bit key now
  if (enc_type == 1)
  {
    for (i = 0; i < 24; i++)
      m17_p1_full[k++] = (super->enc.scrambler_key >> (23-i)) & 1;
  }
  //if AES key, load complete AES key now (if this doesn't work, or even if it does, switch to unpack and load that way)
  else if (enc_type == 2)
  {
    for (i = 0; i < 64; i++)
      m17_p1_full[k++] = (super->enc.A1 >> (63-i)) & 1;
    for (i = 0; i < 64; i++)
      m17_p1_full[k++] = (super->enc.A2 >> (63-i)) & 1;
    for (i = 0; i < 64; i++)
      m17_p1_full[k++] = (super->enc.A3 >> (63-i)) & 1;
    for (i = 0; i < 64; i++)
      m17_p1_full[k++] = (super->enc.A4 >> (63-i)) & 1;
  }

  //zero fill terminating byte (for compatibility)
  for (i = 0; i < 8; i++)
    m17_p1_full[k++] = 0;

  //counter values
  int block = 0; //number of blocks in total
  int ptr = 0;   //ptr to current position of the text
  int pad = 0;   //amount of padding to apply to last frame
  int lst = 0;   //amount of significant octets in the last block
  int stop = 0;  //where to stop packing

  //encode elements
  uint8_t pbc = 0; //packet/octet counter
  uint8_t eot = 0; //end of tx bit

  //since the len of this packet is known precisely, we can hard code these values
  if (enc_type == 1)
  {
    block = 1;
    ptr = k;
    lst = 6;
    pad = 19;
    stop = 5;
    sprintf (super->m17d.sms, "OTAKD Scrambler Key: %X;", super->enc.scrambler_key);
  }
  else if (enc_type == 2)
  {
    block = 2;
    ptr = k;
    lst = 10; //correct, this tells the decoder where to extract the CRC from
    pad = 69; //this has absolutely no effect
    stop = 35;
    sprintf (super->m17d.sms, "OTAKD AES Key: %016llX %016llX %016llX %016llX", super->enc.A1, super->enc.A2, super->enc.A3, super->enc.A4);
  }
  
  //debug position values
  if (super->opts.payload_verbosity > 0)
    fprintf (stderr, "\n BLOCK: %02d; PAD: %02d; LST: %d; K: %04d; PTR: %04d;", block, pad, lst, k, ptr);

  //Calculate the CRC and attach it here
  x = 0;
  uint8_t m17_p1_packed[31*25]; memset (m17_p1_packed, 0, sizeof(m17_p1_packed));
  for (i = 0; i < 25*31; i++)
  {
    m17_p1_packed[x] = (uint8_t)convert_bits_into_output(&m17_p1_full[i*8], 8);
    if (i == stop) break;
    x++;
  }
  //hard set len for CRC16 on type 2, bug fix (investigate reason later)
  if (enc_type == 2) crc_cmp = crc16(m17_p1_packed, 35); //either x, or x+1?
  else crc_cmp = crc16(m17_p1_packed, x+1); //either x, or x+1?

  //debug dump CRC (when pad is literally zero)
  if (super->opts.payload_verbosity > 0)
    fprintf (stderr, "\n X: %d; LAST: %02X; TERM: %02X; CRC: %04X; \n", x, m17_p1_packed[x-1], m17_p1_packed[x], crc_cmp);

  for (i = 0; i < 16; i++) m17_p1_full[k++] = (crc_cmp >> (15-i)) & 1; //this one puts it immediately after the terminating byte

  //debug the full payload
  fprintf (stderr, "\n M17 Packet      FULL: ");
  for (i = 0; i < 25*block; i++)
  {
    if ( (i%25) == 0 && i != 0 ) fprintf (stderr, "\n                       ");
    fprintf (stderr, "%02X", (uint8_t)convert_bits_into_output(&m17_p1_full[i*8], 8));
  }
  fprintf (stderr, "\n");

  //should already be connected earlier, if connected
  int use_ip = super->opts.m17_use_ip;
  uint8_t reflector_module = super->m17e.reflector_module;

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

  //add MPKT header
  k = 0;
  for (j = 0; j < 4; j++)
  {
    for (i = 0; i < 8; i++)
      m17_ip_frame[k++] = (mpkt[j] >> (7-i)) &1;
  }

  //randomize ID
  // srand(time(NULL)); //disabled, investigate any impact this has 
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

  //Send MPKT to reflector
  if (use_ip == 1)
    udp_return = m17_socket_blaster (super, x+34+3, m17_ip_packed);

  //debug
  if (use_ip == 1)
    fprintf (stderr, " UDP IP Frame CRC: %04X; UDP RETURN: %d: X: %d; SENT: %d;", ip_crc, udp_return, x, x+34+3);

  //SEND EOTX to reflector
  if (use_ip == 1)
    udp_return = m17_socket_blaster (super, 10, eotx);

  //flag to determine if we send a new LSF frame for new encode
  //only send once at the appropriate time when encoder is toggled on
  int new_lsf = 1;

  // while (!end)
  for (start = 0; start < block; start++)
  {
    //send LSF frame once, if new encode session
    if (new_lsf == 1)
    {

      fprintf (stderr, "\n M17 LSF    (ENCODER): ");
      demod_lsf(super, m17_lsfs, 1);

      //convert bit array into symbols and RF/Audio
      memset (nil, 0, sizeof(nil));
      encode_rfa (super, nil, mem, 11); //Preamble
      for (i = 0; i < 2; i++)
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

    //debug
    fprintf (stderr, " EOT: %d; PBC: %d; PTR: %d;", eot, pbc, ptr);

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

    //convert bit array into symbols and RF/Audio
    encode_rfa (super, m17_p4s, mem, 4);

    //increment packet / byte counter
    pbc++;

  }
}