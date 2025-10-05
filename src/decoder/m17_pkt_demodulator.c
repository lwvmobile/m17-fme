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

  int i;
  uint8_t  dbuf[184];                 //384-bit frame - 16-bit (8 symbol) sync pattern (184 dibits)
  float    sbuf[184];                 //float symbol buffer
  uint16_t soft_bit[2*SYM_PER_PLD];   //raw frame soft bits
  uint16_t d_soft_bit[2*SYM_PER_PLD]; //deinterleaved soft bits
  uint8_t  viterbi_bytes[31];         //packed viterbi return bytes
  uint32_t error = 0;                 //viterbi error

  memset(dbuf, 0, sizeof(dbuf));
  memset(sbuf, 0.0f, sizeof(sbuf));
  memset(soft_bit, 0, sizeof(soft_bit));
  memset(d_soft_bit, 0, sizeof(d_soft_bit));
  memset(viterbi_bytes, 0, sizeof(viterbi_bytes));

  //if not running in debug / encoder mode, then perform dibit collection
  if (debug == 0)
  {
    //load dibits into dibit buffer
    for (i = 0; i < 184; i++)
      dbuf[i] = get_dibit(super);

    //convert dbuf into a symbol array
    for (i = 0; i < 184; i++)
    {
      if      (dbuf[i] == 0) sbuf[i] = +1.0f;
      else if (dbuf[i] == 1) sbuf[i] = +3.0f;
      else if (dbuf[i] == 2) sbuf[i] = -1.0f;
      else if (dbuf[i] == 3) sbuf[i] = -3.0f;
      else                   sbuf[i] = +0.0f;
    }
    
  }
  else //we are debugging, and convert input to symbols
  {
    //load dibits into dibit buffer from input bits
    for (i = 0; i < 184; i++)
      dbuf[i] = (input[(i*2)+0] << 1) | input[(i*2)+1];

    //convert dbuf into a symbol array
    for (i = 0; i < 184; i++)
    {
      if      (dbuf[i] == 0) sbuf[i] = +1.0f;
      else if (dbuf[i] == 1) sbuf[i] = +3.0f;
      else if (dbuf[i] == 2) sbuf[i] = -1.0f;
      else if (dbuf[i] == 3) sbuf[i] = -3.0f;
      else                   sbuf[i] = +0.0f;
    }
  }

  //libm17 magic
  //slice symbols to soft dibits
  slice_symbols(soft_bit, sbuf);

  //derandomize
  randomize_soft_bits(soft_bit);

  //deinterleave
  reorder_soft_bits(d_soft_bit, soft_bit);

  //viterbi
  error = viterbi_decode_punctured(viterbi_bytes, d_soft_bit, p3, 2*SYM_PER_PLD, 8);

  uint8_t pkt_packed[26];
  memset (pkt_packed, 0, sizeof(pkt_packed));

  //pack to local
  memcpy (pkt_packed, viterbi_bytes+1, 26);

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
  if (super->opts.payload_verbosity)
  {
    if (!eot) fprintf (stderr, "CNT: %02d; PBC: %02d; EOT: %d; ", super->m17d.pbc_ptr, counter, eot);
    else fprintf (stderr, "CNT: %02d; LST: %02d; EOT: %d; ", super->m17d.pbc_ptr, counter, eot);
    // fprintf (stderr, "PTR: %d; Total: %d; ", ptr, total); //internal debug only
    fprintf (stderr, "Ve: %1.1f; ", (float)error/(float)0xFFFF);
  }
  else
  {
    if (!eot) fprintf (stderr, "PKT#: %02d;", counter);
    else fprintf (stderr, "PKT#: XX;");
  }

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

    //create and apply encryption keystream to super->m17d.pkt at this 
    //point, after protocol byte, and prior to terminating byte and CRC
    if ((super->m17d.enc_et == 1 && super->enc.scrambler_key) ||
        (super->m17d.enc_et == 2 && super->enc.aes_key_is_loaded) )
    {

      //keystream bit and byte arrays
      uint8_t ks_bits[7680]; memset(ks_bits, 0, sizeof(ks_bits));
      uint8_t ks_bytes[960]; memset(ks_bytes, 0, sizeof(ks_bytes));

      enc_pkt_ks_creation(super, ks_bits, ks_bytes, 0);
      for (i = 1; i < total; i++)
        super->m17d.pkt[i] ^= ks_bytes[i-1];

      //reset meta (iv) after use
      memset(super->m17d.meta, 0, sizeof(super->m17e.meta));

    }

    //decode completed packet
    if (crc_cmp == crc_ext)
      decode_pkt_contents(super, super->m17d.pkt, total);

    else if (super->opts.allow_crc_failure == 1)
      decode_pkt_contents(super, super->m17d.pkt, total);

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