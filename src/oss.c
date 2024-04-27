/*-------------------------------------------------------------------------------
 * oss.c
 * Project M17 - Hot Garbage Audio Handling
 *
 * LWVMOBILE
 * 2024-05 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

void open_oss_output (Super * super)
{
  
  int fmt; 
  int speed = super->opts.output_sample_rate ;

  super->opts.oss_output_device = open (super->opts.oss_output_dev_str, O_RDWR);
  if (super->opts.oss_output_device == -1)
  {
    fprintf (stderr, "Error, couldn't open #1 %s\n", super->opts.oss_output_dev_str);
    super->opts.use_oss_output = 0;
    exit(1);
  }

  fmt = 0;
  if (ioctl (super->opts.oss_output_device, SNDCTL_DSP_RESET) < 0)
  {
    fprintf (stderr, "ioctl reset error \n");
  }

  fmt = speed;
  if (ioctl (super->opts.oss_output_device, SNDCTL_DSP_SPEED, &fmt) < 0)
  {
    fprintf (stderr, "ioctl speed error \n");
  }

  fmt = 0;
  if (ioctl (super->opts.oss_output_device, SNDCTL_DSP_STEREO, &fmt) < 0)
  {
    fprintf (stderr, "ioctl stereo error \n");
  }

  fmt = AFMT_S16_LE;
  if (ioctl (super->opts.oss_output_device, SNDCTL_DSP_SETFMT, &fmt) < 0)
  {
    fprintf (stderr, "ioctl setfmt error \n");
  }

}

void open_oss_input (Super * super)
{
  
  int fmt; 
  int speed = super->opts.input_sample_rate ;

  super->opts.oss_input_device = open (super->opts.oss_input_dev_str, O_RDWR);
  if (super->opts.oss_input_device == -1)
  {
    fprintf (stderr, "Error, couldn't open #1 %s\n", super->opts.oss_input_dev_str);
    super->opts.oss_input_device = 0;
    exit(1);
  }

  fmt = 0;
  if (ioctl (super->opts.oss_input_device, SNDCTL_DSP_RESET) < 0)
  {
    fprintf (stderr, "ioctl reset error \n");
  }

  fmt = speed;
  if (ioctl (super->opts.oss_input_device, SNDCTL_DSP_SPEED, &fmt) < 0)
  {
    fprintf (stderr, "ioctl speed error \n");
  }

  fmt = 0;
  if (ioctl (super->opts.oss_input_device, SNDCTL_DSP_STEREO, &fmt) < 0)
  {
    fprintf (stderr, "ioctl stereo error \n");
  }

  fmt = AFMT_S16_LE;
  if (ioctl (super->opts.oss_input_device, SNDCTL_DSP_SETFMT, &fmt) < 0)
  {
    fprintf (stderr, "ioctl setfmt error \n");
  }

}

short oss_input_read (Super * super)
{
  short sample = 0;
  read (super->opts.oss_input_device, &sample, 2);
  return sample;
}

//Note: Won't be able to use both RF and VX output simultaneously over OSS (boo hoo)
void oss_output_write (Super * super, short * out, size_t nsam)
{
  int err = 0; UNUSED(err);
  write (super->opts.oss_output_device, out, nsam*2);
}
