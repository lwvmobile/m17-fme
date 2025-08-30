/*-------------------------------------------------------------------------------
 * m17_ota_key_delivery.c
 * M17 Project - Over the Air Key Delivery Format Encoding
 *
 * LWVMOBILE
 * 2024-06 M17 Project - Florida Man Edition
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

//NOTE: This format can also now be used to deliver ECDSA Signature Public Keys if desired.

//this version will send a full LSF, then send a full Data Packet Set with the Complete Key
void encode_ota_key_delivery_pkt (Super * super, int use_ip, uint8_t * sid, uint8_t enc_type, uint8_t enc_stype)
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
  uint8_t protocol = 0x09;

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
  //end CLI Configuration

  uint8_t m17_lsf[244]; //the complete LSF + 4 trailing bits
  memset (m17_lsf, 0, sizeof(m17_lsf));

  //Setup LSF Variables, these are not sent in chunks like with voice
  //but only once at start of PKT TX
  uint16_t lsf_ps    = 0; //packet or stream indicator bit
  uint16_t lsf_dt    = 1; //Data
  uint16_t lsf_et    = 0; //encryption type
  uint16_t lsf_es    = 0; //encryption sub-type
  uint16_t lsf_cn  = can; //can value
  uint16_t lsf_rs = 0x04; //reserved 0x04 is LSF for the PKT, not embedded LSF

  //compose the 16-bit frame information from the above sub elements
  uint16_t lsf_fi = 0;
  lsf_fi = (lsf_ps & 1) + (lsf_dt << 1) + (lsf_et << 3) + (lsf_es << 5) + (lsf_cn << 7) + (lsf_rs << 11);
  for (i = 0; i < 16; i++) m17_lsf[96+i] = (lsf_fi >> (15-i)) & 1;

  //Encode Callsign Data
  encode_callsign_data(super, d40, s40, &dst, &src);

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

  //load enc type, and send sequence number
  uint8_t ssn = 4; //sending over a packet, so this will always be 4 (full message)

  //enc_type and ssn bits
  m17_p1_full[k++] = ( enc_type >> 1) & 1;
  m17_p1_full[k++] = ( enc_type >> 0) & 1;
  m17_p1_full[k++] = (enc_stype >> 1) & 1;
  m17_p1_full[k++] = (enc_stype >> 0) & 1;
  m17_p1_full[k++] = (ssn >> 3) & 1;
  m17_p1_full[k++] = (ssn >> 2) & 1;
  m17_p1_full[k++] = (ssn >> 1) & 1;
  m17_p1_full[k++] = (ssn >> 0) & 1;

  //if scrambler key, load 24-bit key now
  if (enc_type == 1)
  {
    for (i = 0; i < 24; i++)
      m17_p1_full[k++] = (super->enc.scrambler_key >> (23-i)) & 1;
  }

  //if AES key, load complete AES key now
  else if (enc_type == 2)
  {
    unpack_byte_array_into_bit_array(super->enc.aes_key, m17_p1_full+16, 32);
    k += 32*8;
  }

  //Signature Public Key
  else if (enc_type == 3)
  {
    unpack_byte_array_into_bit_array(super->m17d.ecdsa.public_key, m17_p1_full+16, 64);
    k += 64*8;
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
    block =  1;
    lst   =  6+2;
    pad   = 19-2;
    stop  =  5; //this is okay (still have a 00 byte, was still set up as a terminator? or just extra byte? extra 00 byte doesn't matter)
    sprintf (super->m17d.sms, "OTAKD Scrambler Key: %X;", super->enc.scrambler_key);
  }
  else if (enc_type == 2)
  {
    block =  2;
    lst   = 10+2;
    pad   = 15-2;
    stop  = 34; //was 35 (still have a 00 byte, was still set up as a terminator? or just extra byte? extra 00 byte doesn't matter)
    sprintf (super->m17d.sms, "OTAKD AES Key Delivery;");
  }
  else if (enc_type == 3)
  {
    block =  3;
    lst   = 19;
    pad   = 6;
    stop  = 66; //was 67 (still have a 00 byte, was still set up as a terminator? or just extra byte? extra 00 byte doesn't matter)
    sprintf (super->m17d.sms, "OTASK Signature Public Key Delivery;");
  }
  
  //debug position values
  if (super->opts.payload_verbosity > 0)
    fprintf (stderr, "\n BLOCK: %02d; PAD: %02d; LST: %d; K: %04d;", block, pad, lst, k);

  //Calculate the CRC and attach it here
  x = 0;
  uint8_t m17_p1_packed[31*25]; memset (m17_p1_packed, 0, sizeof(m17_p1_packed));
  for (i = 0; i < 25*31; i++)
  {
    m17_p1_packed[x] = (uint8_t)convert_bits_into_output(&m17_p1_full[i*8], 8);
    if (i == stop) break;
    x++;
  }

  //hard set len for CRC16 on type
  if      (enc_type == 1) crc_cmp = crc16(m17_p1_packed, 6);
  else if (enc_type == 2) crc_cmp = crc16(m17_p1_packed, 35);
  else if (enc_type == 3) crc_cmp = crc16(m17_p1_packed, 67);

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

  //Standard IP Framing (stripped down)
  uint8_t m17p[4]  = {0x4D, 0x31, 0x37, 0x50}; //https://github.com/M17-Project/M17_inet/tree/main Current "Standard"
  int udp_return = 0;
  uint8_t  m17_ip_frame[8000]; memset (m17_ip_frame, 0, sizeof(m17_ip_frame));
  uint8_t m17_ip_packed[25*40]; memset (m17_ip_packed, 0, sizeof(m17_ip_packed));
  uint16_t ip_crc = 0;

  //add M17P header
  k = 0;
  for (j = 0; j < 4; j++)
  {
    for (i = 0; i < 8; i++)
      m17_ip_frame[k++] = (m17p[j] >> (7-i)) &1;
  }

  UNUSED(sid); //remove SID from this function later on

  //add the current LSF, sans CRC
  for (i = 0; i < 224; i++) //28 bytes
    m17_ip_frame[k++] = m17_lsf[i];

  //Add checksum for LSF (Byte 1)
  for (i = 0; i < 8; i++)
    m17_ip_frame[k++] = (lsf_packed[28] >> (7-i)) & 1;

  //Add checksum for LSF (Byte 2)
  for (i = 0; i < 8; i++)
    m17_ip_frame[k++] = (lsf_packed[29] >> (7-i)) & 1;

  //pack current bit array to current
  for (i = 0; i < 34; i++)
    m17_ip_packed[i] = (uint8_t)convert_bits_into_output(&m17_ip_frame[i*8], 8);

  //pack the entire PKT payload (plus terminator, sans CRC)
  for (i = 0; i < x+1; i++)
    m17_ip_packed[i+34] = (uint8_t)convert_bits_into_output(&m17_p1_full[i*8], 8);
  
  //Calculate CRC over payload only (double check this)
  ip_crc = crc16(m17_ip_packed+34, 1+x);

  //add CRC value to the ip frame
  uint8_t crc_bits[16]; memset (crc_bits, 0, sizeof(crc_bits));
  for (i = 0; i < 16; i++)
    crc_bits[i] = (ip_crc >> (15-i))&1;

  //pack CRC into the byte array as well
  for (i = x+34+1, j = 0; i < (x+34+3); i++, j++) //double check this
    m17_ip_packed[i] = (uint8_t)convert_bits_into_output(&crc_bits[j*8], 8);

  //debug print what is in m17_ip_packed right now
  // fprintf (stderr, "\n UDP: ");
  // for (i = 0; i < (x+34+3); i++)
  //   fprintf (stderr, "%02X ", m17_ip_packed[i]);
  // fprintf (stderr, "\n");

  //Send M17P to reflector
  if (use_ip == 1)
    udp_return = m17_socket_blaster (super, x+34+3, m17_ip_packed);

  //debug
  if (use_ip == 1)
    fprintf (stderr, " UDP IP Frame CRC: %04X; UDP RETURN: %d: X: %d; SENT: %d;", ip_crc, udp_return, x, x+34+3);

  //flag to determine if we send a new LSF frame for new encode
  //only send once at the appropriate time when encoder is toggled on
  int new_lsf = 1;

  //
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
      for (i = 0; i < 2; i++) //send LSF multiple times on PKT data to ensure it syncs properly
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

    //debug dump
    // if (super->opts.payload_verbosity > 0)
    //   fprintf (stderr, "\n EOT: %d; PBC: %d; PTR: %d;", eot, pbc, ptr);

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

//this version is used to craft embedded OTAKD LSF that can be swapped in during STR or IPF encoding
void encode_ota_key_delivery_emb(Super * super, uint8_t * m17_lsf, uint8_t * lsf_count)
{
  int i, k;

  if (super->enc.aes_key_is_loaded || super->enc.scrambler_key)
  {
    //clear out META field / IV NONCE
    for (i = 112; i < 224; i++)
      m17_lsf[i] = 0;

    //only manipulate the reserved bits in this portion, ignore the rest
    uint16_t lsf_rs = 0x06; //reserved bits (0x06 signals embedded LSF for OTAKD)
    if (super->m17e.ecdsa.keys_loaded)
      lsf_rs = lsf_rs | (uint8_t)0x1; //OR 0x01 for ECDSA
    uint16_t lsf_fi = 0;
    lsf_fi = (lsf_rs << 11);
    for (i = 0; i < 5; i++)
      m17_lsf[96+i] = (lsf_fi >> (15-i)) & 1;

    //load protocol, enc type, and send sequence number
    uint8_t protocol  = 0x09; //OTA Key Delivery Protocol
    uint8_t enc_type  = super->enc.enc_type;
    uint8_t enc_stype = super->enc.enc_subtype;
    uint8_t ssn = (*lsf_count%5) - 1; //mod 5

    //start manipulating at index 112
    if (super->enc.aes_key_is_loaded)
    {
      k = 112;
      for (i = 0; i < 8; i++)
        m17_lsf[k++] = (protocol >> (7-i)) & 1;
      //enc_type and ssn bits
      m17_lsf[k++] = ( enc_type >> 1) & 1;
      m17_lsf[k++] = ( enc_type >> 0) & 1;
      m17_lsf[k++] = (enc_stype >> 1) & 1;
      m17_lsf[k++] = (enc_stype >> 0) & 1;
      m17_lsf[k++] = (ssn >> 3) & 1;
      m17_lsf[k++] = (ssn >> 2) & 1;
      m17_lsf[k++] = (ssn >> 1) & 1;
      m17_lsf[k++] = (ssn >> 0) & 1;

      //load aes key segment, determined by the current ssn value
      if      (ssn == 0)
      {
        for (i = 0; i < 64; i++)
          m17_lsf[k++] = (super->enc.A1 >> (63-i)) & 1;
      }
      else if (ssn == 1)
      {
        for (i = 0; i < 64; i++)
          m17_lsf[k++] = (super->enc.A2 >> (63-i)) & 1;
      }
      else if (ssn == 2)
      {
        for (i = 0; i < 64; i++)
          m17_lsf[k++] = (super->enc.A3 >> (63-i)) & 1;
      }
      else if (ssn == 3)
      {
        for (i = 0; i < 64; i++)
          m17_lsf[k++] = (super->enc.A4 >> (63-i)) & 1;
      }
    }

    else if (super->enc.scrambler_key)
    {
      ssn = 4; //always 4, since we only need one LSF for this
      *lsf_count = 4; //advance it to 4 so we can skip over sending multiple repeats
      k = 112;
      for (i = 0; i < 8; i++)
        m17_lsf[k++] = (protocol >> (7-i)) & 1;
      //enc_type and ssn bits
      m17_lsf[k++] = ( enc_type >> 1) & 1;
      m17_lsf[k++] = ( enc_type >> 0) & 1;
      m17_lsf[k++] = (enc_stype >> 1) & 1;
      m17_lsf[k++] = (enc_stype >> 0) & 1;
      m17_lsf[k++] = (ssn >> 3) & 1;
      m17_lsf[k++] = (ssn >> 2) & 1;
      m17_lsf[k++] = (ssn >> 1) & 1;
      m17_lsf[k++] = (ssn >> 0) & 1;

      //load up to 24-bit scrambler key
      for (i = 0; i < 24; i++)
        m17_lsf[k++] = (super->enc.scrambler_key >> (23-i)) & 1;
    }

    //pack and compute the CRC16 for substitution LSF
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

  }
  //else if no ENC, passthough only

}