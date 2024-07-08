/*-------------------------------------------------------------------------------
 * m17_pkt_demodulator.c
 * M17 Project - Packet Frame Demodulation and Debug
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

void demod_pkt(Super * super, uint8_t * input, int debug)
{
  //quell defined but not used warnings from m17.h
  stfu ();

  int i, x;

  uint8_t dbuf[384];           //384-bit frame - 16-bit (8 symbol) sync pattern (184 dibits)
  uint8_t m17_int_bits[368];  //368 bits that are still interleaved
  uint8_t m17_rnd_bits[368]; //368 bits that are still scrambled (randomized)
  uint8_t m17_bits[368];    //368 bits that have been de-interleaved and de-scramble
  uint8_t m17_depunc[500]; //488 bits after depuncturing

  memset (dbuf, 0, sizeof(dbuf));
  memset (m17_int_bits, 0, sizeof(m17_int_bits));
  memset (m17_rnd_bits, 0, sizeof(m17_rnd_bits));
  memset (m17_bits, 0, sizeof(m17_bits));
  memset (m17_depunc, 0, sizeof(m17_depunc));

  //if not running in debug / encoder mode, then perform dibit collection
  if (debug == 0)
  {
    //load dibits into dibit buffer
    for (i = 0; i < 184; i++)
      dbuf[i] = get_dibit(super);

    //convert dbuf into a bit array
    for (i = 0; i < 184; i++)
    {
      m17_rnd_bits[i*2+0] = (dbuf[i] >> 1) & 1;
      m17_rnd_bits[i*2+1] = (dbuf[i] >> 0) & 1;
    }
  }
  else //we are debugging, and copy input to m17_rnd_bits
    memcpy (m17_rnd_bits, input, 368);

  //descramble the frame
  for (i = 0; i < 368; i++)
    m17_int_bits[i] = (m17_rnd_bits[i] ^ m17_scramble[i]) & 1;

  //deinterleave the bit array using Quadratic Permutation Polynomial
  //function π(x) = (45x + 92x^2 ) mod 368
  for (i = 0; i < 368; i++)
  {
    x = ((45*i)+(92*i*i)) % 368;
    m17_bits[i] = m17_int_bits[x];
  }

  //P3 Depuncture
  x = 0;
  for (i = 0; i < 420; i++)
  {
    if (p3[i%8] == 1)
      m17_depunc[i] = m17_bits[x++];
    else m17_depunc[i] = 0;
  }

  //setup the convolutional decoder
  uint8_t temp[500];
  uint8_t s0;
  uint8_t s1;
  uint8_t m_data[27];
  uint8_t trellis_buf[260]; //30*8 = 240
  memset (trellis_buf, 0, sizeof(trellis_buf));
  memset (temp, 0, sizeof (temp));
  memset (m_data, 0, sizeof (m_data));
  uint16_t metric = 0; UNUSED(metric);

  //test viterbi with all zeroes and all ones
  // memset (m17_depunc, 0, sizeof(m17_depunc));
  // memset (m17_depunc, 1, sizeof(m17_depunc));

  for (i = 0; i < 420; i++) //double check and test value here
    temp[i] = m17_depunc[i] << 1; 

  convolution_start();
  for (i = 0; i < 210; i++) //double check and test value here
  {
    s0 = temp[(2*i)];
    s1 = temp[(2*i)+1];

    metric += convolution_decode(s0, s1);
  }

  convolution_chainback(m_data, 206); //double check and test value here

  unpack_byte_array_into_bit_array(m_data, trellis_buf, 26);

  //test running other viterbi / trellis decoder
  // memset (trellis_buf, 0, sizeof(trellis_buf));
  // memset (m_data, 0, sizeof (m_data));
  // trellis_decode(trellis_buf, m17_depunc, 206);
  // pack_bit_array_into_byte_array_asym(trellis_buf, m_data, 206);

  uint8_t pkt_packed[26];
  memset (pkt_packed, 0, sizeof(pkt_packed));

  //pack to local
  memcpy (pkt_packed, m_data, 26);

  //local variables
  uint8_t counter = (pkt_packed[25] >> 2) & 0x1F;
  uint8_t eot = (pkt_packed[25] >> 7) & 1;

  int ptr = super->m17d.pbc_ptr*25;

  //sanity check to we don't go out of bounds on memcpy and total (core dump)
  if (ptr > 825) ptr = 825;
  if (ptr < 0)   ptr = 0;

  int total = ptr + counter - 3; //-3 if changes to M17_Implementations are made

  //sanity check on total
  if (total < 0 && eot == 1) total = 0; //this is from a bad decode, and caused a core dump on total being a negative value
  
  int end = ptr + 25;

  //debug counter and eot value
  if (!eot) fprintf (stderr, " CNT: %02d; PBC: %02d; EOT: %d;", super->m17d.pbc_ptr, counter, eot);
  else fprintf (stderr, " CNT: %02d; LST: %02d; EOT: %d;", super->m17d.pbc_ptr, counter, eot);
  // fprintf (stderr, " PTR: %d; Total: %d; ", ptr, total);

  //debug view metric out of convolutional decoder
  // fprintf (stderr, " pkt metric: %05d; ", metric); //126 and 13020

  //put packet into storage
  memcpy (super->m17d.pkt+ptr, pkt_packed, 25);

  //individual frame packet
  if (super->opts.payload_verbosity >= 1)
  {
    fprintf (stderr, "\n pkt: ");
    for (i = 0; i < 26; i++)
      fprintf (stderr, "%02X", pkt_packed[i]);
  }

  //evaluate completed packet if eot bit is signalled in current packet
  if (eot)
  {
    //do a CRC check
    uint16_t crc_cmp = crc16(super->m17d.pkt, total+1);
    uint16_t crc_ext = (super->m17d.pkt[total+1] << 8) + super->m17d.pkt[total+2];

    // optimal location?
    if (crc_cmp != crc_ext)
      fprintf (stderr, " (CRC ERR) ");

    //error tracking
    if (crc_cmp != crc_ext) super->error.pkt_crc_err++;

    //if encrypted (lsf indicated, and key available, decrypt the packet now)
    if (super->m17d.enc_et == 1 && super->enc.scrambler_key)
    {
      uint8_t unpacked_pkt[6200]; memset (unpacked_pkt, 0, 6200*sizeof(uint8_t));
      unpack_byte_array_into_bit_array(super->m17d.pkt, unpacked_pkt, total);

      //new method
      super->enc.scrambler_seed_d = super->enc.scrambler_key; //reset seed to key value
      super->enc.scrambler_seed_d = scrambler_sequence_generator(super, 0);
      int z = 0;
      for (i = 8; i < (total*8); i++)
      {
        unpacked_pkt[i] ^= super->enc.scrambler_pn[z++];
        if (z == 128)
        {
          super->enc.scrambler_seed_d = scrambler_sequence_generator(super, 0);
          z = 0;
        }
      }

      //old method
      // for (i = 8; i < (total*8); i++)
      //   unpacked_pkt[i] ^= super->enc.scrambler_pn[i%768];
      
      pack_bit_array_into_byte_array(unpacked_pkt, super->m17d.pkt, total);
    }

    else if (super->m17d.enc_et == 2 && super->enc.aes_key_is_loaded)
    {
      int klen = ((total*8))/128; //NOTE: This will fall short by % value octets
      int kmod = ((total*8))%128; //This is how many bits we are short, so we need to account with a partial ks application

      //debug
      // fprintf (stderr, " AES KLEN: %d; KMOD: %d;", klen, kmod);

      //NOTE: Its pretty redundant to pack and unpack here and in the crypt function,
      //but this is still quicker than writing a new function for only one use case
      
      uint8_t unpacked_pkt[6200]; memset (unpacked_pkt, 0, 6200*sizeof(uint8_t));
      unpack_byte_array_into_bit_array(super->m17d.pkt, unpacked_pkt, total);
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
        
      pack_bit_array_into_byte_array(unpacked_pkt, super->m17d.pkt, total);
    }

    //decode completed packet
    if (crc_cmp == crc_ext)
    {
      if (super->opts.use_m17_textgame_mode)
        decode_game_sms_gate(super, super->m17d.pkt, total);
      else decode_pkt_contents(super, super->m17d.pkt, total);
    }
      
    else if (super->opts.allow_crc_failure == 1)
    {
      if (super->opts.use_m17_textgame_mode)
        decode_game_sms_gate(super, super->m17d.pkt, total);
      else decode_pkt_contents(super, super->m17d.pkt, total);
    }

    // if (crc_cmp != crc_ext)
    //   fprintf (stderr, " (CRC ERR) ");

    if (super->opts.payload_verbosity == 1)
    {
      fprintf (stderr, "\n PKT:");
      for (i = 0; i < end; i++)
      {
        if ( (i%25) == 0 && i != 0)
          fprintf (stderr, "\n     ");
        fprintf (stderr, " %02X", super->m17d.pkt[i]);
      }
      fprintf (stderr, "\n      (CRC CHK) E: %04X; C: %04X;", crc_ext, crc_cmp);
    }

    //reset after processing
    memset (super->m17d.pkt, 0, sizeof(super->m17d.pkt));
    super->m17d.pbc_ptr = 0;
    // super->m17d.dt = 15; //reset here, on OTAKD frames, if the subsequent LSF has an error, then it leaves this as DATA
  }

  //increment pbc counter last
  if (!eot) super->m17d.pbc_ptr++;

  //get rid of this if it costs too much CPU / skips / lags
  super->demod.sync_time = super->demod.current_time = time(NULL);

  //refresh ncurses printer, if enabled
  #ifdef USE_CURSES
  if (super->opts.use_ncurses_terminal == 1)
    print_ncurses_terminal(super);
  #endif

}