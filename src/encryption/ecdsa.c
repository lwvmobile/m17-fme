/*-------------------------------------------------------------------------------
 * ecdsa.c
 * M17 Project - Elliptic Curve Digital Signature Algorithm for Voice Stream
 *
 * LWVMOBILE
 * 2024-06 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//ECDSA Debug Signature Test
void ecdsa_signature_debug_test()
{

  uint8_t priv_key[32] = {0x73, 0xd5, 0x45, 0xd4, 0xa9, 0xde, 0x94, 0xba, 0x4e, 0x22, 0x51, 0x5f, 0x6a, 0xc4, 0xcc, 0x03, 0x2a, 0x09, 0xe6, 0xc8, 0x47, 0xc8, 0x62, 0x97, 0x07, 0x51, 0xb0, 0x35, 0xcb, 0xb4, 0xfa, 0x70};
	uint8_t pub_key[64]  = {0xf9, 0x9e, 0x9a, 0xdc, 0xf7, 0xe5, 0xc1, 0x09, 0x56, 0xf0, 0x9d, 0x07, 0x84, 0x89, 0xb1, 0x70, 0x53, 0x37, 0x15, 0x11, 0x5c, 0xa0, 0x53, 0x5a, 0xb0, 0xa9, 0x62, 0x65, 0x34, 0xcb, 0x9e, 0x96, 0x5b, 0x43, 0x9f, 0x32, 0x1b, 0x62, 0xfc, 0xb6, 0xd1, 0x31, 0xe1, 0xb8, 0x72, 0xe8, 0xd8, 0x30, 0x4f, 0x45, 0xd9, 0xf6, 0xfb, 0x02, 0xb4, 0x1a, 0x33, 0xf6, 0xd8, 0x26, 0x65, 0xd9, 0xd9, 0xdb};
	uint8_t digest[16]   = {0xde, 0xad, 0xbe, 0xef};
	uint8_t sig[64]      = {0};
	
	const struct uECC_Curve_t* curve = uECC_secp256r1();

  printf("digest=");
	for(uint8_t i=0; i<sizeof(digest); i++) printf("%02X", digest[i]);
	printf("\n");
	
	if(uECC_sign(priv_key, digest, sizeof(digest), sig, curve))
		printf("Signing OK\n");
	else
		printf("Signing ERR\n");
	
	printf("sig=");
	for(uint8_t i=0; i<sizeof(sig); i++) printf("%02X", sig[i]);
	printf("\n");
	
	//sig[0]^=0x80; //bit flip to test error
	if(uECC_verify(pub_key, digest, sizeof(digest), sig, curve))
		printf("Signature OK\n");
	else
		printf("Invalid signature\n");

}

//debug values for ECDSA keys
void ecdsa_signature_debug_keys (Super * super)
{
  #ifdef USE_UECC
  int i = 0;

  //test keys
  uint8_t priv_key[32] = {0x73, 0xd5, 0x45, 0xd4, 0xa9, 0xde, 0x94, 0xba, 0x4e, 0x22, 0x51, 0x5f, 0x6a, 0xc4, 0xcc, 0x03, 0x2a, 0x09, 0xe6, 0xc8, 0x47, 0xc8, 0x62, 0x97, 0x07, 0x51, 0xb0, 0x35, 0xcb, 0xb4, 0xfa, 0x70};
	uint8_t pub_key[64]  = {0xf9, 0x9e, 0x9a, 0xdc, 0xf7, 0xe5, 0xc1, 0x09, 0x56, 0xf0, 0x9d, 0x07, 0x84, 0x89, 0xb1, 0x70, 0x53, 0x37, 0x15, 0x11, 0x5c, 0xa0, 0x53, 0x5a, 0xb0, 0xa9, 0x62, 0x65, 0x34, 0xcb, 0x9e, 0x96, 0x5b, 0x43, 0x9f, 0x32, 0x1b, 0x62, 0xfc, 0xb6, 0xd1, 0x31, 0xe1, 0xb8, 0x72, 0xe8, 0xd8, 0x30, 0x4f, 0x45, 0xd9, 0xf6, 0xfb, 0x02, 0xb4, 0x1a, 0x33, 0xf6, 0xd8, 0x26, 0x65, 0xd9, 0xd9, 0xdb};

  fprintf (stderr, "\n");
  fprintf (stderr, "PUB Key:");
  for (i = 0; i < 64; i++)
  {
    super->m17d.ecdsa.public_key[i] = pub_key[i];
    if (i == 16 || i == 32 || i == 48)
      fprintf (stderr, "\n        ");
    fprintf (stderr, " %02X", super->m17d.ecdsa.public_key[i]);
  }

  fprintf (stderr, "\n");
  fprintf (stderr, "PRI Key:");
  for (i = 0; i < 32; i++)
  {
    super->m17e.ecdsa.private_key[i] = priv_key[i];
    if (i == 16) fprintf (stderr, "\n        ");
    fprintf (stderr, " %02X", super->m17e.ecdsa.private_key[i]);
  }

  super->m17e.ecdsa.keys_loaded = 1; //private key available
  super->m17d.ecdsa.keys_loaded = 1; //public key available

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
  const struct uECC_Curve_t * curve = uECC_secp256r1();

  //set pointers to correct items
  pub_key = super->m17d.ecdsa.public_key;
  digest  = super->m17d.ecdsa.last_stream_pyl;
  sig     = super->m17d.ecdsa.signature;

  //debug
  // fprintf (stderr, "\n");
  // fprintf (stderr, "Digest:");
  // for (int i = 0; i < 16; i++)
  //   fprintf (stderr, " %02X", digest[i]);
  // fprintf (stderr, "\n");

  // fprintf (stderr, "\n");
  // fprintf (stderr, "Signature:");
  // for (int i = 0; i < 64; i++)
  // {
  //   if (i == 16 || i == 32 || i == 48)
  //     fprintf (stderr, "\n          ");
  //   fprintf (stderr, " %02X", sig[i]);
  // }


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
void ecdsa_signature_creation (Super * super)
{

  #ifdef USE_UECC

  //pointers
  uint8_t * priv_key;
  uint8_t * digest;
  uint8_t * sig;
  const struct uECC_Curve_t * curve = uECC_secp256r1();

  //set pointers to correct items
  priv_key = super->m17e.ecdsa.private_key;
  digest   = super->m17e.ecdsa.last_stream_pyl;
  sig      = super->m17e.ecdsa.signature;

  //debug
  // fprintf (stderr, "\n");
  // fprintf (stderr, "Digest:");
  // for (int i = 0; i < 16; i++)
  //   fprintf (stderr, " %02X", digest[i]);
  // fprintf (stderr, "\n");

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
