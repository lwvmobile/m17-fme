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

//input is byte array from rolling xor and left shift calculations,
//output is bit array for last stream frame payload
void ecdsa_signature_calculation (Super * super, uint8_t * input, uint8_t * output)
{
  UNUSED(super);
  UNUSED(input); UNUSED(output);

  //do calculation

  //unpack back to bits
  unpack_byte_array_into_bit_array(input, output, 16);
}