#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define HOSTNAME_LEN 256
#define PORT 42069
// #define BUFFERSIZE 512
#define TERMINATE "signal\n"

volatile sig_atomic_t msg_oob = 0;
float wait = 0.4;
size_t buff_size = 512;

void handle_sigint(int signum) {
  // write(2, TERMINATE, sizeof(TERMINATE));
  printf("\nsignal\n");
  _exit(EXIT_SUCCESS);
}

void handle_sigurg(int signum) { msg_oob = 1; }

useconds_t sec2usec(float sec) { return sec * 1e6; }

void print_help(char *app_name) {
  printf("server usage:\n");
  printf("%s [-h] [-w WAIT] [-s SIZE]\n\n", app_name);
  // printf("positional arguments:\n");
  // printf("  destination\n\n");
  printf("options:\n");
  printf("  -h, --help            print help and exit\n");
  printf("  -w WAIT, --wait WAIT  time to wait beetwen sending package\n");
  printf("  -s SIZE, --size SIZE  package size\n");
  printf("  -h, --help\n");
}

void handle_connection(int fd, struct sockaddr_in *addr) {
  char addr_str[INET_ADDRSTRLEN];
  // char buffer[BUFFERSIZE];
  char *buffer = malloc(buff_size * sizeof(char));
  if (buffer == NULL) {
    perror("malloc");
    close(fd);
    exit(EXIT_FAILURE);
  }
  size_t client_data = 0;
  if (inet_ntop(AF_INET, &addr->sin_addr, addr_str, sizeof(addr_str)) == NULL) {
    perror("inet_ntop");
  } else {
    printf("connect with: %s\n", addr_str);
  }
  while (!msg_oob) {
    memset(buffer, 0, buff_size);
    int readsize = recv(fd, buffer, buff_size, 0);
    if (readsize < 0) {
      perror("recv");
    } else if (readsize == 0) {
      break;
    }

    client_data += readsize;
    printf("%s: Total recive: %f kb\n", addr_str, (float)client_data / 1024.f);
    // printf("%s: \n", addr_str);
    usleep(sec2usec(wait));
  }
  free(buffer);
  printf("close connectin: %s\n", addr_str);
  close(fd);
}

int main(int argc, char *argv[]) {

  signal(SIGINT, handle_sigint);
  signal(SIGURG, handle_sigurg);

  char *port_str = getenv("PORT");
  unsigned int port = PORT;
  if (port_str == NULL) {
    printf("WARNING: Environment variable PORT is not defined; used default "
           "value.\n");
  } else {
    char *endptr;
    port = strtoul(port_str, &endptr, 10);
    if (*endptr != '\0') {
      fprintf(stderr, "variable PORT invalid int value %s\n", port_str);
      return EXIT_FAILURE;
    }
  }

  static struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"wait", required_argument, NULL, 'w'},
      {"size", required_argument, NULL, 's'},
      {NULL, 0, NULL, 0},

  };

  int opt;
  while ((opt = getopt_long(argc, argv, "hw:s:", long_options, NULL)) != -1) {
    char *endptr;
    // printf("%c\n", opt);
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

  char hostname[HOSTNAME_LEN];
  if (gethostname(hostname, HOSTNAME_LEN) != 0) {
    perror("gethostname\n");
    return errno;
  }

  printf("hostname: %s\n", hostname);
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
      printf("adders: %s\n", host_addr_str);
      break;
    }
  }
  freeaddrinfo(result);

  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(-1);
  }
  // set socket not block
  // int flags = fcntl(sock, F_GETFL);
  // if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
  //   perror("fcntl set O_NONBLOCK flag");
  //   close(sock);
  //   exit(-1);
  // }

  struct sockaddr_in sock_addr;
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = INADDR_ANY;
  sock_addr.sin_port = htons(port);
  if (bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) {
    perror("bind");
    close(sock);
    exit(-1);
  }
  if (listen(sock, 10) < 0) {
    perror("listen");
    close(sock);
    exit(-1);
  }

  while (1) {
    struct sockaddr_in conn_addr;
    conn_addr.sin_family = AF_INET;
    int conn_addr_size = sizeof(struct sockaddr_in);
    int conn = accept(sock, (struct sockaddr *)&conn_addr,
                      (socklen_t *)&conn_addr_size);
    char addr_str[INET_ADDRSTRLEN];
    if (conn < 0) {
      perror("accept");
      continue;
    }
    if (inet_ntop(AF_INET, &conn_addr.sin_addr, addr_str, sizeof(addr_str)) ==
        NULL) {
      perror("inet_ntop");
    } else {
      printf("connect with: %s\n", addr_str);
    }

    pid_t cpid = fork();
    if (cpid < 0) {
      perror("fork");
      close(conn);
    } else if (cpid == 0) {
      close(sock);
      fcntl(conn, F_SETOWN, getpid());
      handle_connection(conn, &conn_addr);
      break;
    } else {
      close(conn);
    }
  }

  return 0;
}
