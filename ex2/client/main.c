#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define HOSTNAME_LEN 256
#define PORT 42069
#define BUFFERSIZE 4096
#define WAITTIME 400000

volatile sig_atomic_t is_closed = 0;

useconds_t sec2usec(float sec) { return sec * 1e6; }

void print_help(char *app_name) {
  printf("client usage:\n");
  printf("%s [-h] [-w WAIT] [-s SIZE] [destination]\n\n", app_name);
  printf("positional arguments:\n");
  printf("  destination\n\n");
  printf("options:\n");
  printf("  -h, --help            print help and exit\n");
  printf("  -w WAIT, --wait WAIT  time to wait beetwen sending package\n");
  printf("  -s SIZE, --size SIZE  package size\n");
  printf("  -h, --help\n");
}

void handle_sigint(int signum) { is_closed = 1; }

int main(int argc, char *argv[]) {
  signal(SIGINT, handle_sigint);

  char *destination = "server";
  float wait = 0.4;
  size_t buff_size = 4096;

  struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"wait", required_argument, NULL, 'w'},
      {"size", required_argument, NULL, 's'},
      {NULL, 0, NULL, 0},

  };
  int opt;
  while ((opt = getopt_long(argc, argv, "hw:s:", long_options, NULL)) != -1) {
    char *endptr;
    printf("%c\n", opt);
    switch (opt) {
    case 'h':
      print_help(*argv);
      return EXIT_SUCCESS;
    case 'w':
      wait = strtof(optarg, &endptr);
      if (*endptr != '\0') {
        fprintf(stderr, "argument -w/--wait invalid float value: %s\n", optarg);
        return EXIT_FAILURE;
      }
      break;
    case 's':
      buff_size = strtoul(optarg, &endptr, 10);
      if (*endptr != '\0') {
        fprintf(stderr, "argument -s/--size invalid int value: %s\n", optarg);
        return EXIT_FAILURE;
      }
      break;
    }
  }

  if (optind < argc) {
    destination = argv[optind];
  }
  printf("destination: %s\n", destination);

  char hostname[HOSTNAME_LEN];
  if (gethostname(hostname, HOSTNAME_LEN) != 0) {
    perror("gethostname\n");
    return errno;
  }
  printf("%s\n", hostname);
  struct addrinfo hints;
  struct addrinfo *result = NULL;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = 0;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;
  int error = 0;
  if ((error = getaddrinfo(hostname, NULL, &hints, &result)) != 0) {
    perror("getaddrinfo");
    return error;
  }
  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    if (rp->ai_addr->sa_family == AF_INET) {
      struct sockaddr_in *host_addr = (struct sockaddr_in *)rp->ai_addr;
      char host_addr_str[INET_ADDRSTRLEN];
      if (inet_ntop(AF_INET, &host_addr->sin_addr, host_addr_str,
                    sizeof(host_addr_str)) == NULL) {
        perror("inet_ntop\n");
        return errno;
      }
      printf("%s\n", host_addr_str);
      break;
    }
  }
  freeaddrinfo(result);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  if ((error = getaddrinfo(destination, NULL, &hints, &result)) != 0) {
    perror("getaddrinfo");
    return error;
  }
  struct sockaddr_in host_addr;
  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    if (rp->ai_addr->sa_family == AF_INET) {
      memcpy(&host_addr, rp->ai_addr, sizeof(host_addr));
      // host_addr = (struct sockaddr_in)rp->ai_addr;
      char host_addr_str[INET_ADDRSTRLEN];
      if (inet_ntop(AF_INET, &host_addr.sin_addr, host_addr_str,
                    sizeof(host_addr_str)) == NULL) {
        perror("inet_ntop\n");
        return errno;
      }
      printf("%s\n", host_addr_str);
      // break;
    }
  }
  freeaddrinfo(result);
  int sock = 0;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  host_addr.sin_family = AF_INET;
  host_addr.sin_port = htons(PORT);
  if (connect(sock, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
    perror("connect");
    close(sock);
    return -1;
  }
  printf("succesful connect\n");

  char *buffer = malloc(buff_size * sizeof(char));
  if (buffer == NULL) {
    perror("malloc");
    close(sock);
    return EXIT_FAILURE;
  }
  memset(buffer, 'a', buff_size);

  size_t data_size = 0;
  while (!is_closed) {
    send(sock, buffer, buff_size, 0);
    data_size += buff_size;
    printf("total sent: %zukb\n", data_size / 1024);
    usleep(sec2usec(wait));
  }
  printf("End sending\n");
  char data = 0;
  int s = send(sock, &data, sizeof(data), MSG_OOB);
  printf("%d\n", s);
  free(buffer);
  close(sock);

  return EXIT_SUCCESS;
}
