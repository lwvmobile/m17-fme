/*-------------------------------------------------------------------------------
 * m17_lsf_decoder.c
 * M17 Project - Link Setup Frame Contents Decoder
 *
 * LWVMOBILE
 * 2025-10 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void decode_lsf_v2_contents(Super * super)
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

  //decode behavior debug
  // lsf_rs |= 0x10;

  //ECDSA signature included
  uint8_t is_signed = 0;

  //Seperate the Signed bit from the reserved field, shift right once
  if (lsf_rs & 1)
  {
    is_signed = 1;
    lsf_rs >>= 1;
  }

  //if this field is not zero, then this is not a standard V2.0 or older spec'd LSF type,
  //if proposed LSF TYPE field changes occur, this region will be 0 for V2.0, and not 0 for newer
  if (lsf_rs != 0)
  {
    fprintf (stderr, " Unknown LSF TYPE;"); //V3.0 in future spec?
    goto LSF_END;
  }

  //store this so we can reference it for playing voice and/or decoding data, dst/src etc
  super->m17d.dt  = lsf_dt;
  super->m17d.dst = lsf_dst;
  super->m17d.src = lsf_src;
  super->m17d.can = lsf_cn;

  fprintf (stderr, "\n");
  decode_callsign_data(super, lsf_dst, lsf_src);

  for (i = 0; i < super->m17d.lockout_index; i++)
  {
    if (strncmp(super->m17d.src_csd_lockout[i], "         ", 9) != 0)
    {
      if (strncmp(super->m17d.src_csd_str, super->m17d.src_csd_lockout[i], 9) == 0)
      {
        fprintf (stderr, " [LOCKOUT]");
        break;
      }
    }
  }

  fprintf (stderr, " CAN: %d;", lsf_cn);
  
  //only valid on Stream mode
  if (lsf_ps == 1)
  {
    if (lsf_dt == 0) fprintf (stderr, " Reserved");
    if (lsf_dt == 1) fprintf (stderr, " Data");
    if (lsf_dt == 2) fprintf (stderr, " Voice (3200bps)");
    if (lsf_dt == 3) fprintf (stderr, " Voice (1600bps)");

    fprintf (stderr, " Stream");

    //debug type, et, es on misc things from other sources
    if (super->opts.payload_verbosity >= 1)
    {
      fprintf (stderr, "\n");
      fprintf (stderr, " FT: %04X;", lsf_type);
      fprintf (stderr, " ET: %0X;", lsf_et);
      fprintf (stderr, " ES: %0X;", lsf_es);
      fprintf (stderr, " RES: %02X;", lsf_rs);
    }
  }

  //Packet Mode
  if (lsf_ps == 0)
  {
    fprintf (stderr, " Data PDU");
    if (super->opts.payload_verbosity >= 1)
      fprintf (stderr, " FT: %04X;", lsf_type);
    super->m17d.dt = 20;

    //zero out aes_iv, particularly FN portion, will be filled in below if needed
    memset(super->m17d.lsf3.aes_iv, 0, sizeof(super->m17d.lsf3.aes_iv));
  }

  if (is_signed)
    fprintf (stderr, " Signed (secp256r1);");

  if (lsf_et != 0) fprintf (stderr, "\n ENC:");
  if (lsf_et == 1)
  {
    fprintf (stderr, " Scrambler; Subtype: %d;", lsf_es);
    if (lsf_es == 0)
      fprintf (stderr, " (8-bit);");
    else if (lsf_es == 1)
      fprintf (stderr, " (16-bit);");
    else if (lsf_es == 2)
      fprintf (stderr, " (24-bit);");
    if (super->enc.scrambler_key != 0)
      fprintf (stderr, " Key: %X;", super->enc.scrambler_key);
  }

  int keylen = 32;
  if (lsf_et == 2)
  {
    fprintf (stderr, " AES");
    if (lsf_es == 0)
    { 
      keylen = 16;
      fprintf (stderr, " 128;");
    }
      
    else if (lsf_es == 1)
    {
      keylen = 24;
      fprintf (stderr, " 192;");
    }
      
    else if (lsf_es == 2)
    {
      keylen = 32;
      fprintf (stderr, " 256;");
    }
      
  }
  
  super->m17d.enc_et = lsf_et;
  super->m17d.enc_st = lsf_es;

  //use lli and llabs instead (disabled due to m17-tools using truly random non-spec IV values)
  // long long int epoch = 1577836800LL;                                     //Jan 1, 2020, 00:00:00 UTC
  // long long int tsn = ( (super->demod.current_time-epoch) & 0xFFFFFFFF); //current LSB 32-bit value
  // long long int tsi = (uint32_t)convert_bits_into_output(&super->m17d.lsf[112], 32); //OTA LSB 32-bit value
  // long long int dif = llabs(tsn-tsi);
  // if (lsf_et == 2 && dif > 3600) fprintf (stderr, " \n Warning! Time Difference > %lld secs; Potential NONCE/IV Replay!\n", dif);

  //debug
  // fprintf (stderr, "TSN: %ld; TSI: %ld; DIF: %lld;", tsn, tsi, dif);

  //pack meta bits into 14 bytes
  for (i = 0; i < 14; i++)
    super->m17d.meta[i] = (uint8_t)convert_bits_into_output(&super->m17d.lsf[(i*8)+112], 8);

  //if AES IV, then copy meta portion to aes_iv (FN is loaded seperately)
  if (lsf_et == 0x2)
  {
    for (int i = 0; i < 14; i++)
      super->m17d.lsf3.aes_iv[i] = super->m17d.meta[i];
  }
  //else, copy to meta
  else
  {
    for (int i = 0; i < 14; i++)
      super->m17d.lsf3.meta[i] = super->m17d.meta[i];
  }

  //using meta_sum in case some byte fields, particularly meta[0], are zero
  uint32_t meta_sum = 0;
  for (i = 0; i < 14; i++)
    meta_sum += super->m17d.meta[i];

  //Decode Meta Data when not ENC (if meta field is populated with something)
  if (lsf_et == 0 && meta_sum != 0)
  {
    uint8_t meta[15]; meta[0] = lsf_es + 0x80; //add identifier for pkt decoder

    //re-map older values to newer internal values
    //Extended CSD currently removed from 3.0, so just map it to depreciated new value of 0x98
    if (meta[0] == 0x82)
      meta[0] = 0x98;
    //Meta Text to V3.0 Value <-- seperated two different decodes for this, depending on version used, older is still 0x80
    // if (meta[0] == 0x80)
    //   meta[0] = 0x82;
    //end re-map

    for (i = 0; i < 14; i++)
      meta[i+1] = super->m17d.meta[i];
    fprintf (stderr, "\n ");
    decode_pkt_contents (super, meta, 15); //decode META
  }

  //if no Meta (debug)
  // if (lsf_et == 0 && meta_sum == 0)
  //   fprintf (stderr, " Meta Null; ");

  //reset potential stale FN portion of meta field if ES enc PKT 
  //comes in immediately after a Stream without a nocarrier reset
  if (lsf_ps == 0) //packet data indicator
  {
    super->m17d.meta[14] = 0;
    super->m17d.meta[15] = 0;
  }
  
  if (lsf_et == 2)
  {
    fprintf (stderr, " IV: ");
    for (i = 0; i < 16; i++)
      fprintf (stderr, "%02X", super->m17d.meta[i]);

    if (super->enc.aes_key_is_loaded)
    {
      fprintf (stderr, "\n Key: ");
      for (i = 0; i < keylen; i++)
        fprintf (stderr, "%02X", super->enc.aes_key[i]);

    }
  }

  //open a per call wav file here if stream voice, if enabled, if not already opened
  if (lsf_ps && super->opts.use_wav_out_pc && super->wav.wav_out_pc == NULL)
    setup_percall_filename(super);

  LSF_END: {} //
  
}

void decode_lsf_v3_contents(Super * super)
{

  //hexidicimal values of destination and source
  unsigned long long int lsf_dst = (unsigned long long int)convert_bits_into_output(&super->m17d.lsf[0], 48);
  unsigned long long int lsf_src = (unsigned long long int)convert_bits_into_output(&super->m17d.lsf[48], 48);

  //LSF Verison 3.0 Type Field
  uint16_t lsf_type = (uint16_t)convert_bits_into_output(&super->m17d.lsf[96], 16);

  //LSF Type Field broken into its sub fields
  uint16_t payload_contents = (lsf_type >> 12) & 0xF;
  uint16_t encryption_type  = (lsf_type >> 9)  & 0x7;
  uint16_t signature        = (lsf_type >> 8)  & 0x1;
  uint16_t meta_contents    = (lsf_type >> 4)  & 0xF;
  uint16_t can              = (lsf_type >> 0)  & 0xF;

  //assign local to super
  super->m17d.lsf3.full_type_field = lsf_type;
  super->m17d.lsf3.dst_hex_value = lsf_dst;
  super->m17d.lsf3.src_hex_value = lsf_src;
  super->m17d.lsf3.payload_contents = payload_contents;
  super->m17d.lsf3.encryption_type = encryption_type;
  super->m17d.lsf3.signature = signature;
  super->m17d.lsf3.meta_contents = meta_contents;
  super->m17d.lsf3.can = can;

  //fix ncurses display and history / event items to show can and not -1
  super->m17d.can = can;

  fprintf (stderr, "\n");
  decode_callsign_data(super, lsf_dst, lsf_src);

  //TODO: Storage for CSD String? Or leave as-is?
  for (int i = 0; i < super->m17d.lockout_index; i++)
  {
    if (strncmp(super->m17d.src_csd_lockout[i], "         ", 9) != 0)
    {
      if (strncmp(super->m17d.src_csd_str, super->m17d.src_csd_lockout[i], 9) == 0)
      {
        fprintf (stderr, " [LOCKOUT]");
        break;
      }
    }
  }

  fprintf (stderr, " CAN: %d;", can);

  if      (payload_contents == 0x1) fprintf (stderr, " Stream Data");
  else if (payload_contents == 0x2) fprintf (stderr, " Voice (3200bps)");
  else if (payload_contents == 0x3) fprintf (stderr, " Voice (1600bps)");
  else if (payload_contents == 0xF) fprintf (stderr, " Packet Data");
  else fprintf (stderr, " Reserved: %X", payload_contents);

  if (signature) fprintf (stderr, " Signed (secp256r1);");

  //compatibility shim for current code and call history / event log (remove later if redone)
  if (payload_contents != 0 && payload_contents < 4)
    super->m17d.dt = payload_contents;
  else if (payload_contents == 0xF)
    super->m17d.dt = 20;

  //pack meta bits into 14 bytes
  for (int i = 0; i < 14; i++)
    super->m17d.meta[i] = (uint8_t)convert_bits_into_output(&super->m17d.lsf[(i*8)+112], 8);

  //zero out aes_iv, if Packet Data, particularly any stale FN values loaded
  if (payload_contents == 0xF)
    memset(super->m17d.lsf3.aes_iv, 0, sizeof(super->m17d.lsf3.aes_iv));

  //if AES IV, then copy meta portion to aes_iv (FN is loaded seperately)
  if (meta_contents == 0xF)
  {
    for (int i = 0; i < 14; i++)
      super->m17d.lsf3.aes_iv[i] = super->m17d.meta[i];
  }
  //else, copy to meta
  else
  {
    for (int i = 0; i < 14; i++)
      super->m17d.lsf3.meta[i] = super->m17d.meta[i];
  }

  //decode encryption_type (to maintain a level of backwards compatibility,
  //the old encryption type and subtype values will be used only for encryption)
  if (encryption_type != 0)
  {
    fprintf (stderr, "\n ENC:");

    if (encryption_type == 0x1)
    {
      fprintf (stderr, " Scrambler (8-bit);");
      super->m17d.enc_et = 1;
      super->m17d.enc_st = 0;
    }
    else if (encryption_type == 0x2)
    {
      fprintf (stderr, " Scrambler (16-bit);");
      super->m17d.enc_et = 1;
      super->m17d.enc_st = 1;
    }
    else if (encryption_type == 0x3)
    {
      fprintf (stderr, " Scrambler (24-bit);");
      super->m17d.enc_et = 1;
      super->m17d.enc_st = 2;
    }

    else if (encryption_type == 0x4)
    {
      fprintf (stderr, " AES-CTR (128-bit);");
      super->m17d.enc_et = 2;
      super->m17d.enc_st = 0;
    }
    else if (encryption_type == 0x5)
    {
      fprintf (stderr, " AES-CTR (192-bit);");
      super->m17d.enc_et = 2;
      super->m17d.enc_st = 1;
    }
    else if (encryption_type == 0x6)
    {
      fprintf (stderr, " AES-CTR (256-bit);");
      super->m17d.enc_et = 2;
      super->m17d.enc_st = 2;
    }

    else if (encryption_type == 0x7)
    {
      fprintf (stderr, " Reserved Enc (0x7);");
      super->m17d.enc_et = 3;
      super->m17d.enc_st = 3;
    }

    if (encryption_type <= 0x3 && super->enc.scrambler_key != 0)
      fprintf (stderr, " Key: %X;", super->enc.scrambler_key);
    else if (encryption_type > 0x3 && encryption_type <= 0x6 && super->enc.aes_key_is_loaded == 1)
    {

      fprintf (stderr, " IV: ");
      for (int i = 0; i < 16; i++)
        fprintf (stderr, "%02X", super->m17d.lsf3.aes_iv[i]);

      int keylen = 32;
      if (encryption_type == 0x4)
        keylen = 16;
      else if (encryption_type == 0x5)
        keylen = 24;
      else keylen = 32;

      fprintf (stderr, "\n Key: ");
      for (int i = 0; i < keylen; i++)
        fprintf (stderr, "%02X", super->enc.aes_key[i]);
    }

  }
  else
  {
    super->m17d.enc_et = 0;
    super->m17d.enc_st = 0;
  }

  //handle any decodable meta contents, not including AES IV
  if (meta_contents != 0 && meta_contents != 0xF)
  {
    uint8_t meta[15];
    meta[0] = meta_contents + 0x80;
    for (int i = 0; i < 14; i++)
      meta[i+1] = super->m17d.lsf3.meta[i];
    fprintf (stderr, "\n ");
    decode_pkt_contents (super, meta, 15); //decode META
  }

  //Payload Dump on LSF Type
  if (super->opts.payload_verbosity >= 1)
  {
    fprintf (stderr, "\n");
    fprintf (stderr, " FT: %04X;", lsf_type);
    fprintf (stderr, " PAY: %X;", payload_contents);
    fprintf (stderr, " ENC: %X;", encryption_type);
    fprintf (stderr, " SIG: %X;", signature);
    fprintf (stderr, " META: %X;", meta_contents);
  }

}

//gate function to determine if this is v2, or v3, and proceed accordingly
void decode_lsf_contents(Super * super)
{
  uint16_t lsf_type = (uint16_t)convert_bits_into_output(&super->m17d.lsf[96], 16);
  uint16_t version_check = lsf_type >> 12;

  if (version_check == 0)
    decode_lsf_v2_contents(super);
  else decode_lsf_v3_contents(super);
}