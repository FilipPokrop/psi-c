#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define HOSTNAME_LEN 256
#define PORT 42069
#define BUFFERSIZE 2048

void print_help(char *app_name) {
  printf("client usage:\n");
  printf("%s [destination]\n", app_name);
}

int main(int argc, char *argv[]) {
  if (argc > 2) {
    print_help(argv[0]);
    return 1;
  }
  char *destination = "server";
  if (argc == 2) {
    if (strcmp(argv[1], "-h") == 0) {
      print_help(argv[0]);
      return 0;
    }
    destination = argv[1];
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
  if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
    perror("socket");
    return -1;
  }

  struct pollfd pfd;
  pfd.fd = sock;
  pfd.events = POLLOUT;

  host_addr.sin_family = AF_INET;
  host_addr.sin_port = htons(PORT);
  if (connect(sock, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
    if (errno != EINPROGRESS) {
      perror("connect");
      close(sock);
      return -1;
    }
    printf("wait on connect\n");
  }
  int ready = poll(&pfd, 1, -1);
  if (ready < 0) {
    perror("pool\n");
    return -1;
  }

  char buffer[BUFFERSIZE];
  ssize_t buff_size = 0;

  if (pfd.revents & POLLERR) {
    if (send(sock, buffer, sizeof(buffer), 0) < 0) {
      perror("connect");
      return -1;
    }
  }

  printf("succesful connect\n");

  int is_closed = 1;

  fd_set readfds;

  while (is_closed) {
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);
    select(sock + 1, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(sock, &readfds)) {
      if (recv(sock, &buffer, sizeof(buffer), 0) == 0) {
        printf("server colse connection\n");
        is_closed = 0;
      }
    } else if (FD_ISSET(STDIN_FILENO, &readfds)) {
      memset(&buffer, 0, sizeof(buffer));
      buff_size = read(STDIN_FILENO, buffer, sizeof(buffer));
      if (buff_size == 0)
        is_closed = 0;
      else {
        send(sock, buffer, buff_size - 1, 0);
      }
    }
  }
  close(sock);

  return 0;
}
