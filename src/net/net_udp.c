/*-------------------------------------------------------------------------------
 * net_udp.c
 * M17 Project - Network Functions for UDP/IP Frame Input and Output
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

struct sockaddr_in addressM17;
struct sockaddr_in addressM17duplex;

int udp_socket_bind(char *hostname, int portno)
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

int m17_socket_blaster(Super * super, size_t nsam, void * data)
{
  int err = 0;
  err = sendto(super->opts.m17_udp_sock, data, nsam, 0, (const struct sockaddr * ) & addressM17, sizeof(struct sockaddr_in));
  return (err);
}

int udp_socket_connectM17(Super * super)
{
  long int err = 0;
  err = super->opts.m17_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (err < 0)
  {
    fprintf (stderr, " UDP Socket Error %ld\n", err);
    return (err);
  }

  // Don't think this is needed, but doesn't seem to hurt to keep it here either
  int broadcastEnable = 1;
  err = setsockopt(super->opts.m17_udp_sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

  if (err < 0)
  {
    fprintf (stderr, " UDP Broadcast Set Error %ld\n", err);
    return (err);
  }

  //set these for non blocking when no samples to read (or speed up responsiveness to ncurses)
  struct timeval read_timeout;
  read_timeout.tv_sec = 0;
  read_timeout.tv_usec = 10;
  err = setsockopt(super->opts.m17_udp_sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  if (err < 0)
  {
    fprintf (stderr, " UDP Read Timeout Set Error %ld\n", err);
    return (err);
  }

  memset((char * ) & addressM17, 0, sizeof(addressM17));
  addressM17.sin_family = AF_INET;
  // err = addressM17.sin_addr.s_addr = inet_addr(super->opts.m17_hostname); //old method, fallback if issues arise
  err = inet_aton(super->opts.m17_hostname, &addressM17.sin_addr); //inet_aton handles broadcast .255 addresses correctly in some environments (Raspbian GNU/Linux 11 (bullseye) armv7l)
  if (err < 0)
  {
    fprintf (stderr, " UDP inet_addr Error %ld\n", err);
    return (err);
  }

  addressM17.sin_port = htons(super->opts.m17_portno);
  if (err < 0)
  {
    fprintf (stderr, " UDP htons Error %ld\n", err);
    return (err);
  }

  return (0); //no error
}

int m17_socket_receiver(Super * super, void * data)
{
  size_t err = 0;
  struct sockaddr_in cliaddr; 
  socklen_t len = sizeof(cliaddr); 

  //receive data from socket
  err = recvfrom(super->opts.m17_udp_sock, data, 1000, 0, (struct sockaddr * ) & addressM17, &len);

  return err;
}

int m17_socket_receiver_duplex(int m17_udp_socket_duplex, void * data)
{
  size_t err = 0;
  struct sockaddr_in cliaddr; 
  socklen_t len = sizeof(cliaddr); 

  //receive data from socket
  err = recvfrom(m17_udp_socket_duplex, data, 1000, 0, (struct sockaddr * ) & addressM17duplex, &len);

  return err;
}

void error(char *msg)
{
  perror(msg);
  exit(0);
}