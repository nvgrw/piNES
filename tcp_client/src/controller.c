#include <pigpio.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define LATCH_DURATION 12                                        // us
#define PULSE_HALF_CYCLE 6                                       // us
#define LATCH_WAIT 1667 - LATCH_DURATION - PULSE_HALF_CYCLE * 8  // us

#define PORT_NO 51717

static volatile bool running = true;

void sigIntHandler(int a) { running = false; }

void pulse() {
  gpioWrite(27, 1);
  gpioDelay(PULSE_HALF_CYCLE);
  gpioWrite(27, 0);
}

static union {
  struct {
    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t select : 1;
    uint8_t start : 1;
    uint8_t up : 1;
    uint8_t down : 1;
    uint8_t left : 1;
    uint8_t right : 1;
  } data;
  uint8_t raw;
} cntrl_1, cntrl_2;

void poll_controllers(void) {
  // Set latch for 12us
  gpioWrite(18, 1);
  gpioDelay(LATCH_DURATION);
  gpioWrite(18, 0);

  // Poll A
  cntrl_1.data.a = !gpioRead(17);
  cntrl_2.data.a = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll B
  pulse();
  cntrl_1.data.b = !gpioRead(17);
  cntrl_2.data.b = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll SELECT
  pulse();
  cntrl_1.data.select = !gpioRead(17);
  cntrl_2.data.select = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll START
  pulse();
  cntrl_1.data.start = !gpioRead(17);
  cntrl_2.data.start = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll UP
  pulse();
  cntrl_1.data.up = !gpioRead(17);
  cntrl_2.data.up = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll DOWN
  pulse();
  cntrl_1.data.down = !gpioRead(17);
  cntrl_2.data.down = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll LEFT
  pulse();
  cntrl_1.data.left = !gpioRead(17);
  cntrl_2.data.left = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll RIGHT
  pulse();
  cntrl_1.data.right = !gpioRead(17);
  cntrl_2.data.right = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);
}

void init_controller(void) {
  if (gpioInitialise() < 0) exit(1);

  // Pin 17: cntrl_1 data
  gpioSetMode(17, PI_INPUT);
  // Pin 22: cntrl_2 data
  gpioSetMode(22, PI_INPUT);

  // Pin 18: latch
  gpioSetMode(18, PI_OUTPUT);
  // Pin 27: pulse
  gpioSetMode(27, PI_OUTPUT);
}

void deinit_controller(void) { gpioTerminate(); }

void error(const char* message) {
  perror(message);
  exit(EXIT_FAILURE);
}

static uint8_t buffer[2] = {0, 0};

int main(int argc, char** argv) {
  init_controller();
  gpioSetSignalFunc(SIGINT, sigIntHandler);

  int sockfd, n;
  struct sockaddr_in serv_addr;
  struct hostent* server;
  if (argc < 2) {
    error("ERROR: must provide address of host");
  }
  // Initialise socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("ERROR on opening socket");
  }
  // Initialise server
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    error("ERROR: no such host");
  }
  memset((char*)&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr,
         server->h_length);
  serv_addr.sin_port = htons(PORT_NO);
  if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR connecting");
  }
  // Connection established
  while (running) {
    poll_controllers();
    buffer[0] = cntrl_1.raw;
    buffer[1] = cntrl_2.raw;
    n = write(sockfd, buffer, 2);
    if (n < 0) {
      error("ERROR writing to socket");
    }

    gpioDelay(LATCH_WAIT);
  }

  close(sockfd);
  deinit_controller();
  return EXIT_SUCCESS;
}
