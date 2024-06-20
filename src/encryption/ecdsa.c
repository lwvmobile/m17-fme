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

/*
The last frame would be generated as follows:
1. At the start of the stream initialize an array of 16 bytes with all 0's
2. After each stream frame (starting with 0) XOR the array with payload and rotate it by 1 byte (let's say left)
3. Repeat until there's no more voice payload.
*/

/*
Use bit 11 of the TYPE field to indicate signed stream (1-signed, 0-unsigned).
The most significant bit of the frame counter of the last speech/data frame must not be set if the stream is signed.*
If the stream is signed, the last 4 frames should have frame numbers equal to 0x7FFC, 0x7FFD, 0x7FFE, 0xFFFF, with the last one having MSB set to 1 (stream end).*
The contents of the last 4 frames is the signature. It is calculated with the stream digest and user's private key over secp256r1 curve - 512-bit long vector.
*/

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
