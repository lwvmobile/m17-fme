/*-------------------------------------------------------------------------------
 * m17_lsf_decoder.c
 * Project M17 - Link Setup Frame Contents Decoder
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

// void decode_lsf_contents(m17_decoder_state * m17d)
void decode_lsf_contents(Super * super)
{
  int i;
  unsigned long long int lsf_dst = (unsigned long long int)ConvertBitIntoBytes(&super->m17d.lsf[0], 48);
  unsigned long long int lsf_src = (unsigned long long int)ConvertBitIntoBytes(&super->m17d.lsf[48], 48);
  uint16_t lsf_type = (uint16_t)ConvertBitIntoBytes(&super->m17d.lsf[96], 16);

  //this is the way the spec/code expects you to read these bits
  uint8_t lsf_ps = (lsf_type >> 0) & 0x1;
  uint8_t lsf_dt = (lsf_type >> 1) & 0x3;
  uint8_t lsf_et = (lsf_type >> 3) & 0x3;
  uint8_t lsf_es = (lsf_type >> 5) & 0x3;
  uint8_t lsf_cn = (lsf_type >> 7) & 0xF;
  uint8_t lsf_rs = (lsf_type >> 11) & 0x1F;

  //store this so we can reference it for playing voice and/or decoding data, dst/src etc
  super->m17d.dt  = lsf_dt;
  super->m17d.dst = lsf_dst;
  super->m17d.src = lsf_src;
  super->m17d.can = lsf_cn;

  fprintf (stderr, "\n");
  decode_callsign_data(super, lsf_dst, lsf_src);
  fprintf (stderr, " CAN: %d;", lsf_cn);
  
  if (lsf_dt == 0) fprintf (stderr, " Reserved");
  if (lsf_dt == 1) fprintf (stderr, " Data");
  if (lsf_dt == 2) fprintf (stderr, " Voice (3200bps)");
  if (lsf_dt == 3) fprintf (stderr, " Voice (1600bps)");

  if (lsf_rs != 0) fprintf (stderr, " RS: %02X", lsf_rs);

  if (lsf_et != 0) fprintf (stderr, " ENC:");
  if (lsf_et == 2) fprintf (stderr, " AES-CTR");
  if (lsf_et == 1) fprintf (stderr, " Scrambler - %d", lsf_es);

  //packet or stream
  if (lsf_ps == 0) fprintf (stderr, " Packet");
  if (lsf_ps == 1) fprintf (stderr, " Stream");

  super->m17d.enc_et = lsf_et;
  super->m17d.enc_st = lsf_es;

  //compare incoming META/IV value on AES, if timestamp 32-bits are not within a time 5 minute window, then throw a warning
  uint32_t tsn = (time(NULL) & 0xFFFFFFFF); //current LSB 32-bit value
  uint32_t tsi = (uint32_t)ConvertBitIntoBytes(&super->m17d.lsf[112], 32); //OTA LSB 32-bit value
  uint32_t dif = abs(tsn-tsi);
  if (lsf_et == 2 && dif > 3600) fprintf (stderr, " \n Warning! Time Difference > %d secs; Potential NONCE/IV Replay!\n", dif);

  //debug
  // fprintf (stderr, "TSN: %ld; TSI: %ld; DIF: %ld;", tsn, tsi, dif);

  //pack meta bits into 14 bytes
  for (i = 0; i < 14; i++)
    super->m17d.meta[i] = (uint8_t)ConvertBitIntoBytes(&super->m17d.lsf[(i*8)+112], 8);

  //Decode Meta Data
  if (lsf_et == 0 && super->m17d.meta[0] != 0) //not sure if this applies universally, or just to text data byte 0 for null data
  {
    uint8_t meta[15]; meta[0] = lsf_es + 90; //add identifier for pkt decoder
    for (i = 0; i < 14; i++) meta[i+1] = super->m17d.lsf[i];
    fprintf (stderr, "\n ");
    decode_pkt_contents (super, meta, 15); //decode META
  }
  
  if (lsf_et == 2)
  {
    fprintf (stderr, " IV: ");
    for (i = 0; i < 16; i++)
      fprintf (stderr, "%02X", super->m17d.meta[i]);
  }
  
}