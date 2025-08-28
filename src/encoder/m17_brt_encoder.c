/*-------------------------------------------------------------------------------
 * m17_str_encoder.c
 * M17 Project - Stream Voice Encoder
 *
 * LWVMOBILE
 * 2024-06 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"
#include "m17.h"

//encode and create audio of a M17 Project BERT signal
void encode_brt(Super * super)
{

  init_brt();

  float mem[81];

  //quell defined but not used warnings from m17.h
  stfu ();

  //initialize RRC memory buffer
  memset (mem, 0, 81*sizeof(float));

  //Enable TX
  super->m17e.str_encoder_tx = 1;

  //if using the ncurses terminal, disable TX on startup until user toggles it with the '\' key, if not vox enabled
  if (super->opts.use_ncurses_terminal == 1 && super->opts.use_m17_str_encoder == 1 && super->m17e.str_encoder_vox == 0)
    super->m17e.str_encoder_tx = 0;
  
  int i, k, x;    //basic utility counters

  uint8_t nil[368]; //empty array to send to RF during Preamble, EOT Marker, or Dead Air
  memset (nil, 0, sizeof(nil));

  //send dead air with type 99
  for (i = 0; i < 25; i++)
    encode_rfa (super, nil, mem, 99);

  //send Preamble B
  encode_rfa (super, nil, mem, 33);

  //BERT LFSR
  uint16_t bert_lfsr_seed = 1;

  while (!exitflag) //while the software is running
  {

    uint8_t bert[201]; memset (bert, 0, sizeof(bert));
    bert_lfsr_seed = brt_lfsr(bert_lfsr_seed, bert, 197);
    
    //initialize and start assembling the completed frame
    uint8_t bert_c[402]; memset (bert_c, 0, sizeof(bert_c));

    //Use the convolutional encoder to encode the BERT Frame
    simple_conv_encoder (bert, bert_c, 201);

    uint8_t bert_p[368]; memset (bert_p, 0, sizeof(bert_p));

    //use the P2 puncture to...puncture and collapse the BERT Frame
    k = 0; x = 0;
    for (i = 0; i < 34; i++)
    {
      bert_p[k++] = bert_c[x++];
      bert_p[k++] = bert_c[x++];
      bert_p[k++] = bert_c[x++];
      bert_p[k++] = bert_c[x++];
      bert_p[k++] = bert_c[x++];
      //quit early on last set of i when 368 k bits reached 
      //index from 0 to 367,so 368 is breakpoint with k++
      if (k == 368) break;
      bert_p[k++] = bert_c[x++];
      bert_p[k++] = bert_c[x++];
      bert_p[k++] = bert_c[x++];
      bert_p[k++] = bert_c[x++];
      bert_p[k++] = bert_c[x++];
      bert_p[k++] = bert_c[x++];
      x++;
    }

    //debug
    // fprintf (stderr, "CONV: %03d; PUNC: %03d;", x, k);

    uint8_t bert_i[368]; memset (bert_i, 0, sizeof(bert_i));

    //interleave the bit array using Quadratic Permutation Polynomial
    //function π(x) = (45x + 92x^2 ) mod 368
    for (i = 0; i < 368; i++)
    {
      x = ((45*i)+(92*i*i)) % 368;
      bert_i[x] = bert_p[i];
    }

    uint8_t bert_s[368]; memset (bert_s, 0, sizeof(bert_s));

    //scramble/randomize the frame
    for (i = 0; i < 368; i++)
      bert_s[i] = (bert_i[i] ^ m17_scramble[i]) & 1;

    //-----------------------------------------

    if (super->m17e.str_encoder_tx == 1) //when toggled on
    {
      //Enable sync
      super->demod.in_sync = 1;

      fprintf (stderr, "\n M17 BERT (ENCODER) LFSR %03X: ", bert_lfsr_seed);
      // if (super->opts.internal_loopback_decoder == 1)
      //   demod_brt(super, bert_s, 1); //NOTE: This will not work, since we use the same static for both, it'll just keep resetting itself
      fprintf (stderr, " To Audio Out: %s", super->pa.pa_outrf_idx);

      #ifdef USE_PULSEAUDIO
      //debug show pulse input latency
      if (super->opts.use_pa_input == 1 && super->opts.demod_verbosity >= 2)
      {
        unsigned long long int latency = pa_simple_get_latency (super->pa.pa_input_device, NULL);
        fprintf (stderr, " Latency: %05lld;", latency);
      }
      #endif

      //convert bit array into symbols and RF/Audio
      encode_rfa (super, bert_s, mem, 3);

    } //end if (super->m17d.strencoder_tx)
    else bert_lfsr_seed = 1; //reset seed
    
  }

  super->demod.current_time = time(NULL);

}