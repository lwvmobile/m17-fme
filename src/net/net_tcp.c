/*-------------------------------------------------------------------------------
 * net_tcp.c
 * M17 Project - Network Functions for TCP
 *
 * LWVMOBILE
 * 2024-05 M17 Project - Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "main.h"

struct sockaddr_in address;

int tcp_socket_connect (char *hostname, int portno)
{
  int sockfd;
  struct sockaddr_in serveraddr;
  struct hostent *server;


  /* socket: create the socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    fprintf(stderr,"ERROR opening socket\n");
    error("ERROR opening socket");
  }
      

  /* gethostbyname: get the server's DNS entry */
  server = gethostbyname(hostname);
  if (server == NULL)
  {
      fprintf(stderr,"ERROR, no such host as %s\n", hostname);
      //exit(0);
      return (0); //return 0, check on other end and configure pulse input 
  }

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portno);

  /* connect: create a connection with the server */
  if (connect(sockfd, (const struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
  {
      fprintf(stderr,"ERROR opening socket\n");
      return (0);
  }      

  return sockfd;
}

bool tcp_socket_send(int sockfd, char *buf)
{
    int n;

    n = write(sockfd, buf, strlen(buf));
    if (n < 0)
      error("ERROR writing to socket");
    return true;
}

bool tcp_socket_receive(int sockfd, char *buf)
{
    int n;

    n = read(sockfd, buf, BUFSIZE);
    if (n < 0)
      error("ERROR reading from socket");
    buf[n]= '\0';
    return true;
}

bool tcp_snd_audio_source_open (Super * super)
{
  bool err = false;
  super->snd_src_in.audio_in_file = sf_open_fd(super->opts.tcp_input_sock, SFM_READ, super->snd_src_in.audio_in_file_info, 0);
  if (super->snd_src_in.audio_in_file != NULL)
    err = true;

  return err;
}