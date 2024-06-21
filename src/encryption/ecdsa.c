/*-------------------------------------------------------------------------------
 * ecdsa.c
 * M17 Project - Elliptic Curve Digital Signature Algorithm for Voice Stream
 *
 * LWVMOBILE
 * 2024-06 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//load public and private keys from user arg, or file
void ecdsa_key_loader (Super * super)
{
  UNUSED(super);
}

void ecdsa_curve_init(Super * super)
{
  #ifdef USE_UECC
  super->m17d.ecdsa.curve = uECC_secp256r1();
  super->m17e.ecdsa.curve = uECC_secp256r1();
  #else
  UNUSED(super);
  #endif
}

//decoder side
void ecdsa_signature_verification (Super * super)
{

  #ifdef USE_UECC

  //pointers
  uint8_t * pub_key;
  uint8_t * digest;
  uint8_t * sig;
  uECC_Curve curve;

  //set pointers to correct items
  pub_key = super->m17d.ecdsa.public_key;
  digest  = super->m17d.ecdsa.last_stream_pyl;
  sig     = super->m17d.ecdsa.signature;
  curve   = super->m17d.ecdsa.curve;

  //run verification
  int valid = 0;
  valid = uECC_verify(pub_key, digest, 16*sizeof(uint8_t), sig, curve);

  if (valid) fprintf (stderr, " Signature Valid;");
  else fprintf (stderr, " Signature Invalid;");

  #else
  UNUSED(super);
  #endif
}

//encoder side
void ecdsa_signature_signing (Super * super)
{

  #ifdef USE_UECC

  //pointers
  uint8_t * priv_key;
  uint8_t * digest;
  uint8_t * sig;
  uECC_Curve curve;

  //set pointers to correct items
  priv_key = super->m17e.ecdsa.public_key;
  digest   = super->m17e.ecdsa.last_stream_pyl;
  sig      = super->m17e.ecdsa.signature;
  curve    = super->m17e.ecdsa.curve;

  //run signing
  int valid = 0;
  valid = uECC_sign(priv_key, digest, 16*sizeof(uint8_t), sig, curve);

  if (valid) fprintf (stderr, " Signature Success; \n");
  else fprintf (stderr, " Signature Failure; \n");

  fprintf (stderr, " Signature:");
  for (int i = 0; i < 64; i++)
  {
    if ( (i != 0) && ((i%16) == 0) )
      fprintf (stderr, "\n           ");
    fprintf (stderr, " %02X", sig[i]);
  }

  #else
  UNUSED(super);
  #endif
}
