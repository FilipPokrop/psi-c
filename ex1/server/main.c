#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
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
#define BUFFERSIZE 2048
#define TERMINATE "signal\n"

void handle_sigint(int signum) {
  // write(2, TERMINATE, sizeof(TERMINATE));
  printf("\nsignal\n");
  _exit(0);
}

void handle_connection(int fd, struct sockaddr_in *addr) {
  char addr_str[INET_ADDRSTRLEN];
  char buffer[BUFFERSIZE];
  if (inet_ntop(AF_INET, &addr->sin_addr, addr_str, sizeof(addr_str)) == NULL) {
    perror("inet_ntop");
  } else {
    printf("connect with: %s\n", addr_str);
  }
  while (1) {
    memset(&buffer, 0, BUFFERSIZE);
    int readsize = recv(fd, buffer, BUFFERSIZE, 0);
    if (readsize < 0) {
      perror("recv");
    } else if (readsize == 0) {
      break;
    }

    printf("%s: %s\n", addr_str, buffer);
  }
  printf("close connectin: %s\n", addr_str);
  close(fd);
}

int main(int argc, char *argv[]) {

  signal(SIGINT, handle_sigint);
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
  sock_addr.sin_port = htons(PORT);
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
    if (conn < 0) {
      perror("accept");
      continue;
    }

    pid_t cpid = fork();
    if (cpid < 0) {
      perror("fork");
      close(conn);
    } else if (cpid == 0) {
      close(sock);
      handle_connection(conn, &conn_addr);
      break;
    } else {
      close(conn);
    }
  }

  return 0;
}
