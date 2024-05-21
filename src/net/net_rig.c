/*-------------------------------------------------------------------------------
 * net_rig.c
 * M17 Project - RIGCTL Remote Functions
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

bool rigctl_set_frequency (int sockfd, long int freq)
{
  char buf[BUFSIZE];

  sprintf (buf, "F %ld\n", freq); 
  tcp_socket_send(sockfd, buf);
  tcp_socket_receive(sockfd, buf);

  if (strcmp(buf, "RPRT 1\n") == 0 )
    return false;

  return true;
}

bool rigctl_set_modulation_nfm (int sockfd, int bandwidth) 
{
  char buf[BUFSIZE];
  //SDR++ has changed the token from FM to NFM, and back again, so just try both
  sprintf (buf, "M NFM %d\n", bandwidth);
  tcp_socket_send(sockfd, buf);
  tcp_socket_receive(sockfd, buf);

  //if it fails the first time, send the other token instead
  if (strcmp(buf, "RPRT 1\n") == 0 )
  {
    sprintf (buf, "M FM %d\n", bandwidth);
    tcp_socket_send(sockfd, buf);
    tcp_socket_receive(sockfd, buf);
  }

  if (strcmp(buf, "RPRT 1\n") == 0 )
    return false;

  return true;
}

bool rigctl_set_modulation_wfm (int sockfd, int bandwidth) 
{
  char buf[BUFSIZE];
  sprintf (buf, "M WFM %d\n", bandwidth);
  tcp_socket_send(sockfd, buf);
  tcp_socket_receive(sockfd, buf);

  if (strcmp(buf, "RPRT 1\n") == 0 )
    return false;

  return true;
}

bool tune_to_frequency (Super * super, long int frequency)
{
  bool err = false;

  //RIGCTL Remote
  if (super->opts.use_rig_remote && super->opts.rig_remote_open)
  {
    err = rigctl_set_modulation_nfm (super->opts.rig_remote_sock, 12000);
    err = rigctl_set_frequency (super->opts.rig_remote_sock, frequency);
  }

  return err;
}