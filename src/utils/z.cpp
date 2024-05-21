/*-------------------------------------------------------------------------------
 * z.cpp
 * M17 Project - Test File for CPP Code
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//Note: When using CMAKE, it will not link math as a library for some reason when pure C,
//but you can do this trick to link math (ultimately, will want to see if this can be done without
//having to do this ridiculous step -- 
//this may not be a universal problem, may depend on compiler envioronment
int sample_cpp_func(int input)
{
  int a = sqrt (input);
  return a;
}