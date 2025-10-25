/*-------------------------------------------------------------------------------
 * m17_ecdsa_encoder.c
 * M17 Project - Stream Voice Encoder Signature
 *
 * LWVMOBILE
 * 2024-06 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

//encode and create audio of a M17 Project Stream Frame Signature
void encode_str_ecdsa(Super * super, uint8_t lich_cnt, uint8_t * m17_lsf, float * mem, int use_ip, int udpport, uint8_t * sid)
{

  //BUGFIX: LSD encoding that strattles the signature frames may have bad encoding or CRC,
  //so to just not have the receiving end attempt to decode last LSD, let's reset this
  //to zero at this point
  lich_cnt = 0;

  //quell defined but not used warnings from m17.h
  stfu();
  
  ecdsa_signature_creation(super);

  //unpack signed payload into bits to be sent over payload
  uint8_t sig_bits[512]; memset(sig_bits, 0, 512*sizeof(uint8_t));
  unpack_byte_array_into_bit_array(super->m17e.ecdsa.signature, sig_bits, 64);
  
  int i, j, k, x, y, z;    //basic utility counters

  //Standard IP Framing
  uint8_t magic[4] = {0x4D, 0x31, 0x37, 0x20};
  int udp_return = 0; UNUSED(udp_return);
  uint8_t m17_ip_frame[432]; memset (m17_ip_frame, 0, sizeof(m17_ip_frame));
  uint8_t m17_ip_packed[54]; memset (m17_ip_packed, 0, sizeof(m17_ip_packed));
  uint16_t ip_crc = 0;

  //frame sequence number and eot bit
  uint16_t fsn = 0x7FFC; //starting value for ECDSA Signature
  uint8_t eot = 0;

  uint8_t lsf_chunk[6][48]; //40 bit chunks of link information spread across 6 frames
  memset (lsf_chunk, 0, sizeof(lsf_chunk));


  y = 0; //counter for sig_bits
  for (z = 0; z < 4; z++) //loop to send ECDSA Signature in 4 Frame Payload
  {

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

    //Load in Signature Bits here
    for (i = 0; i < 128; i++)
      m17_v1[i+16] = sig_bits[y++];

    if (z == 3) //set EOT bit on 4th pass
      m17_v1[0] = 1;
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

    fprintf (stderr, "\n M17 Stream (ENCODER): ");
    if (super->opts.internal_loopback_decoder == 1)
      demod_str(super, m17_t4s, 1);
    else fprintf (stderr, " To Audio Out: %s", super->pa.pa_outrf_idx);

    //show UDP if active
    if (use_ip == 1 && lich_cnt != 5 && fsn != 0x7FFF)
      fprintf (stderr, " UDP: %s:%d", super->opts.m17_hostname, udpport);

    fprintf (stderr, " Sending Signature (secp256r1);");

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

    if (z == 3) //set EOT bit on 4th pass
      eot = 1;

    //add eot bit flag
    m17_ip_frame[k++] = eot&1;

    //add current fsn value
    for (i = 0; i < 15; i++)
      m17_ip_frame[k++] = (fsn >> (14-i))&1;

    //add signature bits
    for (i = 0; i < 128; i++)
      m17_ip_frame[k++] = m17_v1[i+16];

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
      lich_cnt = 0;

    //increment frame sequency number
    fsn++;

  }

  //reset ECSDA payloads
  memset (super->m17e.ecdsa.curr_stream_pyl, 0, 16*sizeof(uint8_t));
  memset (super->m17e.ecdsa.last_stream_pyl, 0, 16*sizeof(uint8_t));
  memset (super->m17e.ecdsa.signature, 0, 64*sizeof(uint8_t));

  //reset decoder side as well for loopback decoder
  memset (super->m17d.ecdsa.curr_stream_pyl, 0, 16*sizeof(uint8_t));
  memset (super->m17d.ecdsa.last_stream_pyl, 0, 16*sizeof(uint8_t));
  memset (super->m17d.ecdsa.signature, 0, 64*sizeof(uint8_t));

}