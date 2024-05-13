/*-------------------------------------------------------------------------------
 * m17_lsf_decoder.c
 * Project M17 - Link Setup Frame Contents Decoder
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void decode_lsf_contents(Super * super)
{
  int i;
  unsigned long long int lsf_dst = (unsigned long long int)convert_bits_into_output(&super->m17d.lsf[0], 48);
  unsigned long long int lsf_src = (unsigned long long int)convert_bits_into_output(&super->m17d.lsf[48], 48);
  uint16_t lsf_type = (uint16_t)convert_bits_into_output(&super->m17d.lsf[96], 16);

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

  //packet or stream
  if (lsf_ps == 0) fprintf (stderr, " Packet");
  if (lsf_ps == 1) fprintf (stderr, " Stream");

  if (lsf_rs != 0)
  { 
    if (lsf_rs == 0x10)
      fprintf (stderr, " OTAKD Data Packet;");
    else if (lsf_rs == 0x11)
    {
      fprintf (stderr, " OTAKD Embedded LSF;\n");
      goto LSF_END;
    }
    else
     fprintf (stderr, " RES: %02X;", lsf_rs);
  }

  if (lsf_et != 0) fprintf (stderr, "\n ENC:");
  if (lsf_et == 1)
  {
    fprintf (stderr, " Scrambler Type: %d;", lsf_es);
    if (super->enc.scrambler_key != 0)
      fprintf (stderr, " Key: %X;", super->enc.scrambler_key);
  }
  if (lsf_et == 2)
    fprintf (stderr, " AES-CTR;");
  
  super->m17d.enc_et = lsf_et;
  super->m17d.enc_st = lsf_es;

  //use lli and llabs instead (disabled due to m17-tools using truly random non-spec IV values)
  // long long int tsn = (super->demod.current_time & 0xFFFFFFFF); //current LSB 32-bit value
  // long long int tsi = (uint32_t)convert_bits_into_output(&super->m17d.lsf[112], 32); //OTA LSB 32-bit value
  // long long int dif = llabs(tsn-tsi);
  // if (lsf_et == 2 && dif > 3600) fprintf (stderr, " \n Warning! Time Difference > %lld secs; Potential NONCE/IV Replay!\n", dif);

  //debug
  // fprintf (stderr, "TSN: %ld; TSI: %ld; DIF: %lld;", tsn, tsi, dif);

  //pack meta bits into 14 bytes
  for (i = 0; i < 14; i++)
    super->m17d.meta[i] = (uint8_t)convert_bits_into_output(&super->m17d.lsf[(i*8)+112], 8);

  //Decode Meta Data when not AES IV (if available)
  if (lsf_et != 2 && super->m17d.meta[0] != 0)
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

    if (super->enc.aes_key_is_loaded)
    {
      fprintf (stderr, "\n AES Key:");
      for (int i = 0; i < 32; i++)
      {
        if (i == 16) fprintf (stderr, "\n         ");
        fprintf (stderr, " %02X", super->enc.aes_key[i]);
      }
    }
  }

  //open a per call wav file here if stream voice, if enabled, if not already opened
  if (lsf_ps && super->opts.use_wav_out_pc && super->wav.wav_out_pc == NULL)
    setup_percall_filename(super);

  LSF_END:
  if (lsf_rs == 0x11)
  {
    uint8_t otakd[16];
    pack_bit_array_into_byte_array(super->m17d.lsf+112, otakd, 16);
    decode_pkt_contents (super, otakd, 16);
  }
  
}