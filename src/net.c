/*-------------------------------------------------------------------------------
 * net.c
 * Project M17 - Network Functions for UDP/IP Frame Input and Output
 *
 * LWVMOBILE
 * 2024-04 Project M17 - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

//UDP Specific
#include <arpa/inet.h>

struct sockaddr_in addressM17;

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int UDPBind (char *hostname, int portno)
{
  UNUSED(hostname);

  int sockfd;
  struct sockaddr_in serveraddr;

  /* socket: create the socket */
  //UDP socket
  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (sockfd < 0)
  {
    fprintf(stderr,"ERROR opening UDP socket\n");
    error("ERROR opening UDP socket");
  }

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY
  serveraddr.sin_port = htons(portno);

  //Bind socket to listening
  if (bind(sockfd, (struct sockaddr *) &serveraddr,  sizeof(serveraddr)) < 0)
  { 
    perror("ERROR on binding UDP Port");
  }

  //set these for non blocking when no samples to read
  struct timeval read_timeout;
  read_timeout.tv_sec = 0;
  read_timeout.tv_usec = 10;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  return sockfd;
}

int m17_socket_blaster(config_opts * opts, size_t nsam, void * data)
{
  int err = 0;
  err = sendto(opts->m17_udp_sock, data, nsam, 0, (const struct sockaddr * ) & addressM17, sizeof(struct sockaddr_in));
  return (err);
}

int udp_socket_connectM17(config_opts * opts)
{
  long int err = 0;
  err = opts->m17_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (err < 0)
  {
    fprintf (stderr, " UDP Socket Error %ld\n", err);
    return (err);
  }

  // Don't think this is needed, but doesn't seem to hurt to keep it here either
  int broadcastEnable = 1;
  err = setsockopt(opts->m17_udp_sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
  if (err < 0)
  {
    fprintf (stderr, " UDP Broadcast Set Error %ld\n", err);
    return (err);
  }

  memset((char * ) & addressM17, 0, sizeof(addressM17));
  addressM17.sin_family = AF_INET;
  err = addressM17.sin_addr.s_addr = inet_addr(opts->m17_hostname);
  if (err < 0)
    fprintf (stderr, " UDP inet_addr Error %ld\n", err);

  addressM17.sin_port = htons(opts->m17_portno);
  if (err < 0)
    fprintf (stderr, " UDP htons Error %ld\n", err);

  return (err);
}

int m17_socket_receiver(config_opts * opts, void * data)
{
  size_t err = 0;
  struct sockaddr_in cliaddr; 
  socklen_t len = sizeof(cliaddr); 

  //receive data from socket
  err = recvfrom(opts->m17_udp_sock, data, 1000, 0, (struct sockaddr * ) & addressM17, &len);

  return err;
}