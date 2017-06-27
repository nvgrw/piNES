/**
 * MIT License
 *
 * Copyright (c) 2017
 * Aurel Bily, Alexis I. Marinoiu, Andrei V. Serbanescu, Niklas Vangerow
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "controller.h"
#include "controller_tcp.h"
#include "error.h"

#define PORT_NO 51717

/**
 * controller_tcp.c
 */

#define READLEN 1000

static union {
  controller_pressed_t state;
  uint8_t raw;
} ctrl_1, ctrl_2;

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

static bool has_client = false;
static uint8_t buffer[READLEN];  // Zeroed out as part of BSS.

int controller_tcp_init(void) {
  // Initialise socket
  tcp.sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (tcp.sockfd < 0) {
    perror("ERROR on opening socket");
    return 1;
  }
  // Set socket to non-blocking
  int status =
      fcntl(tcp.sockfd, F_SETFL, fcntl(tcp.sockfd, F_GETFL, 0) | O_NONBLOCK);
  if (status < 0) {
    perror("ERROR on fcntl");
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
  return 0;
}

void controller_tcp_poll(controller_t* ctrl) {
  int n;  // return value for read() and write()
  if (!has_client) {
    tcp.newsockfd =
        accept(tcp.sockfd, (struct sockaddr*)&tcp.cli_addr, &tcp.clilen);
    if (tcp.newsockfd < 0) {
      //  perror("ERROR on accept");
      return;
    }
    // Connection established with client
    has_client = true;
  }
  n = read(tcp.newsockfd, buffer, READLEN);
  if (n >= 2) {
    ctrl_1.raw = buffer[0];
    ctrl_2.raw = buffer[1];
  }

  ctrl->pressed1.a |= ctrl_1.state.a;
  ctrl->pressed1.b |= ctrl_1.state.b;
  ctrl->pressed1.select |= ctrl_1.state.select;
  ctrl->pressed1.start |= ctrl_1.state.start;
  ctrl->pressed1.up |= ctrl_1.state.up;
  ctrl->pressed1.down |= ctrl_1.state.down;
  ctrl->pressed1.left |= ctrl_1.state.left;
  ctrl->pressed1.right |= ctrl_1.state.right;
  ctrl->pressed2.a |= ctrl_2.state.a;
  ctrl->pressed2.b |= ctrl_2.state.b;
  ctrl->pressed2.select |= ctrl_2.state.select;
  ctrl->pressed2.start |= ctrl_2.state.start;
  ctrl->pressed2.up |= ctrl_2.state.up;
  ctrl->pressed2.down |= ctrl_2.state.down;
  ctrl->pressed2.left |= ctrl_2.state.left;
  ctrl->pressed2.right |= ctrl_2.state.right;
}

void controller_tcp_deinit(void) {
  close(tcp.newsockfd);
  close(tcp.sockfd);
}
