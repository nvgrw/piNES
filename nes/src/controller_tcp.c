#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/controller.h"
#include "../include/controller_tcp.h"
#include "error.h"

#define PORT_NO 51717

/**
 * controller_tcp.c
 */

static union {
  controller_pressed_t ctrl1_state;
  uint8_t raw;
} ctrl_1;

static union {
  controller_pressed_t ctrl2_state;
  uint8_t raw;
} ctrl_2;

static struct {
  int sockfd;        // socket file descriptor
  int newsockfd;     // socket file descriptor upon succesful client/sever
                     // connection
  int portno;        // port number on which server accepts connections from
                     // client(s)
  socklen_t clilen;  // size of address of client
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
} tcp;

int controller_tcp_init(void) {
  // Initialise socket
  tcp.sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (tcp.sockfd < 0) {
    perror("ERROR on opening socket");
    return 1;
  }
  // Initialise server address
  memset((char*)&tcp.serv_addr, '\0', sizeof(tcp.serv_addr));
  tcp.serv_addr.sin_family = AF_INET;
  tcp.serv_addr.sin_addr.s_addr = INADDR_ANY;
  tcp.serv_addr.sin_port = htons(PORT_NO);
  // Bind socket to address
  if (bind(tcp.sockfd, (struct sockaddr*)&tcp.serv_addr,
           sizeof(tcp.serv_addr)) < 0) {
    perror("ERROR on binding");
    return 1;
  }
  listen(tcp.sockfd, 5);
  tcp.clilen = sizeof(tcp.cli_addr);
  tcp.newsockfd =
      accept(tcp.sockfd, (struct sockaddr*)&tcp.cli_addr, &tcp.clilen);
  if (tcp.newsockfd < 0) {
    perror("ERROR on accept");
    return 1;
  }
  // Connection established with client
  return 0;
}

static uint8_t buffer[2] = {0, 0};

void controller_tcp_poll(controller_t* ctrl) {
  int n;  // return value for read() and write()
  n = read(tcp.newsockfd, buffer, 2);
  if (n == 2) {
    ctrl_1.raw = buffer[0];
    ctrl_2.raw = buffer[1];
  }
  n = write(newsockfd, "Status received", 15);
  ctrl->pressed1.a |= ctrl_1.ctrl1_state.a;
  ctrl->pressed1.b |= ctrl_1.ctrl1_state.b;
  ctrl->pressed1.select |= ctrl_1.ctrl1_state.select;
  ctrl->pressed1.start |= ctrl_1.ctrl1_state.start;
  ctrl->pressed1.up |= ctrl_1.ctrl1_state.up;
  ctrl->pressed1.down |= ctrl_1.ctrl1_state.down;
  ctrl->pressed1.left |= ctrl_1.ctrl1_state.left;
  ctrl->pressed1.right |= ctrl_1.ctrl1_state.right;
  ctrl->pressed2.a |= ctrl_2.ctrl2_state.a;
  ctrl->pressed2.b |= ctrl_2.ctrl2_state.b;
  ctrl->pressed2.select |= ctrl_2.ctrl2_state.select;
  ctrl->pressed2.start |= ctrl_2.ctrl2_state.start;
  ctrl->pressed2.up |= ctrl_2.ctrl2_state.up;
  ctrl->pressed2.down |= ctrl_2.ctrl2_state.down;
  ctrl->pressed2.left |= ctrl_2.ctrl2_state.left;
  ctrl->pressed2.right |= ctrl_2.ctrl2_state.right;
}

void controller_tcp_deinit(void) {
  close(tcp.newsockfd);
  close(tcp.sockfd);
}
