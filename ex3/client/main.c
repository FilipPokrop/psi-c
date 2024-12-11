#include "stats.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define USEC_IN_SEC 1000000
#define BUFFSIZE 2048
float fwait = 1.f;
// useconds_t uwait = USEC_IN_SEC;
struct timeval wait;
struct statistic stats;
int data_size = 64;
volatile sig_atomic_t count = -1;
pid_t pid;
// char *destination = "google.pl"
struct icmp_packet {
  u_int8_t type;
  u_int8_t code;
  u_int16_t checksum;
  u_int16_t identifier;
  u_int16_t sequence_number;
  char data[];
};

struct ip_packet {
  u_int8_t IHL : 4;
  u_int8_t version : 4;
  u_int8_t TOS;
  u_int16_t total_length;
  u_int16_t identification;
  u_int16_t flags_and_fragment_offset;
  u_int8_t time;
  u_int8_t protocol;
  u_int16_t header_checksum;
  u_int32_t source_addres;
  u_int32_t destination_addres;
  char data[];
};

useconds_t sec2usec(float sec) { return sec * USEC_IN_SEC; }

void handle_sigint(int signum) { count = 0; }

int timeval_subtract(struct timespec *result, const struct timespec *x,
                     const struct timespec *y) {
  /* Perform the carry for the later subtraction by updating y. */
  struct timespec yy = *y;
  if (x->tv_nsec < yy.tv_nsec) {
    int nsec = (yy.tv_nsec - x->tv_nsec) / 1000000000 + 1;
    yy.tv_nsec -= 1000000000 * nsec;
    yy.tv_sec += nsec;
  }
  if (x->tv_nsec - yy.tv_nsec > 1000000000) {
    int nsec = (x->tv_nsec - yy.tv_nsec) / 1000000000;
    yy.tv_nsec += 1000000000 * nsec;
    yy.tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_nsec = x->tv_nsec - y->tv_nsec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

u_short in_cksum(u_short *addr, int len) {
  int nleft, sum;
  u_short *w;
  union {
    u_short us;
    u_char uc[2];
  } last;
  u_short answer;

  nleft = len;
  sum = 0;
  w = addr;

  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  /* mop up an odd byte, if necessary */
  if (nleft == 1) {
    last.uc[0] = *(u_char *)w;
    last.uc[1] = 0;
    sum += last.us;
  }

  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
  sum += (sum >> 16);                 /* add carry */
  answer = ~sum;                      /* truncate to 16 bits */
  return (answer);
}

int handle_ip_header(void *buffer, size_t size, struct sockaddr_in *addr,
                     void **data) {
  struct ip_packet *packet = (struct ip_packet *)buffer;
  /* uint8_t version = packet->version_and_IHL >> 4 & 0xf; */
  /* uint8_t hsize = (packet->version_and_IHL & 0xf) * 4; */
  uint8_t hsize = packet->IHL * 4;
  *data = (char *)buffer + hsize;
  /* printf("%u %u \n", addr->sin_addr.s_addr, packet->source_addres); */

  if (packet->version != 4) {
    printf("wrong ip version");
    return -1;
  }
  if (in_cksum(buffer, hsize) != 0) {
    printf("wrong checksum\n");
    return -2;
  }
  if (addr->sin_addr.s_addr != packet->source_addres) {
    printf("wrong source address\n");
    return -3;
  }

  return size - hsize;
}

int handle_icmp_header(void *buffer, size_t size, int seq_num) {
  struct icmp_packet *packet = (struct icmp_packet *)buffer;
  if (packet->type != 0) {
    return -1;
  }
  if (packet->code != 0) {
    return -2;
  }
  if (in_cksum(buffer, size)) {
    return -3;
  }
  if (packet->sequence_number != seq_num) {
    return -4;
  }
  if (packet->identifier != pid) {
    return -5;
  }
  return 0;
}

int send_ping(int sock, struct sockaddr_in *addr, int size, int seq_num) {
  struct icmp_packet *packet =
      (struct icmp_packet *)malloc(size * sizeof(char));
  if (packet == NULL) {
    perror("malloc");
    return -1;
  }
  char *buffer = (char *)packet;
  packet->type = 8;
  packet->code = 0;
  packet->checksum = 0;
  packet->identifier = pid & 0xffff;
  packet->sequence_number = seq_num;

  memset(packet->data, 'Q', size - sizeof(struct icmp_packet));
  u_short checksum = in_cksum((u_short *)packet, size);
  packet->checksum = checksum;
  sendto(sock, packet, size, 0, (struct sockaddr *)addr,
         sizeof(struct sockaddr_in));
  free(packet);

  return 0;
}

struct timeval recive_ping(int sock, struct sockaddr_in *addr, int seq_num,
                           const struct timeval *start,
                           const struct timeval *timeout) {
  struct timeval tv_stop, tv_result, tv_start;
  struct timeval tv_timeout = *timeout;
  struct timeval triptime;
  tv_start = *start;
  tv_result = *timeout;
  while (tv_timeout.tv_usec > 0 || tv_timeout.tv_sec > 0) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    int fds_count;
    fds_count = select(sock + 1, &fds, NULL, NULL, &tv_timeout);
    gettimeofday(&tv_stop, NULL);
    if (fds_count < 0) {
      perror("select");
    }
    if (fds_count == 0) {
      printf("Timeout\n");
      return tv_result;
    }
    char buffer[BUFFSIZE];
    int readsize = recv(sock, &buffer, BUFFSIZE, 0);
    char *data;
    int datasize = handle_ip_header(buffer, readsize, addr, (void *)&data);
    if (datasize < 0)
      continue;
    if (handle_icmp_header(data, datasize, seq_num) < 0)
      continue;

    char *addr_str = inet_ntoa(addr->sin_addr);
    timersub(&tv_stop, &tv_start, &triptime);
    struct ip_packet *ip_pack = (struct ip_packet *)buffer;

    /* struct icmp_packet *icmp_pack = (struct icmp_packet *)data; */
    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%ld.%03ld\n", datasize,
           addr_str, seq_num, ip_pack->time, triptime.tv_sec,
           triptime.tv_usec / 1000);
    /* printf("") */
    /* print_packet(buffer, data, struct timeval *time) */
    return triptime;
    timersub(timeout, &triptime, &tv_timeout);
  }

  return tv_result;
}

struct timeval ping(int sock, struct sockaddr_in *addr, int size, int seq_num,
                    struct timeval *timeout) {
  struct timeval timeout_cp = *timeout;
  struct timeval start, stop, result;

  send_ping(sock, addr, size, seq_num);
  st_send(&stats);
  gettimeofday(&start, NULL);

  struct timeval triptime = recive_ping(sock, addr, seq_num, &start, timeout);
  st_recive(&stats, &triptime);

  return triptime;
}

int main(int argc, char **argv) {
  pid = getpid();

  signal(SIGINT, handle_sigint);

  st_init(&stats);

  struct option long_options[] = {{"help", no_argument, NULL, 'h'},
                                  {"count", required_argument, NULL, 'c'},
                                  {"wait", required_argument, NULL, 'w'},
                                  {"size", required_argument, NULL, 's'},
                                  {NULL, 0, NULL, 0}};
  int opt;
  while ((opt = getopt_long(argc, argv, "c:s:w:h", long_options, NULL)) != -1) {
    char *endptr;
    float wait;
    switch (opt) {
    case 'h':

      return EXIT_SUCCESS;
    case 'w':
      fwait = strtof(optarg, &endptr);
      // uwait = sec2usec(wait);
      if (*endptr != '\0') {
        fprintf(stderr, "argument -w/--wait invalid float value: %s\n", optarg);
        return EXIT_FAILURE;
      }
      break;
    case 's':
      data_size = strtoul(optarg, &endptr, 10);
      if (*endptr != '\0') {
        fprintf(stderr, "argument -s/--size invalid int value: %s\n", optarg);
        return EXIT_FAILURE;
      }
      break;
    case 'c':
      count = strtoul(optarg, &endptr, 10);
      if (*endptr != '\0') {
        fprintf(stderr, "argument -c/--count invalid int value: %s\n", optarg);
        return EXIT_FAILURE;
      }
      break;
    }
  }

  if (optind == argc) {
    fprintf(stderr, "usage error: Destination addres required\n");
    return EXIT_FAILURE;
  }
  char *destination = argv[optind];

  wait.tv_sec = (time_t)fwait;
  wait.tv_usec = (useconds_t)sec2usec(fwait - wait.tv_sec);
  /* printf("%ld, %ld\n", wait.tv_sec, wait.tv_usec); */

  // memset(&hints, 0, sizeof hints);
  struct addrinfo hints;
  struct addrinfo *result;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  int error;
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
        return EXIT_FAILURE;
      }
      // printf("%s\n", host_addr_str);
      break;
    }
  }
  freeaddrinfo(result);
  int sock;
  if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  uint64_t i = 0;
  while (count != 0) {

    struct timeval timeout = wait;
    struct timeval triptime = ping(sock, &host_addr, data_size, i, &timeout);

    useconds_t sleeptime =
        triptime.tv_sec > 0 ? 0 : USEC_IN_SEC - triptime.tv_usec;
    /* printf("%ld,%ld\n", triptime.tv_sec, triptime.tv_usec); */
    if (usleep(sleeptime) < -1) {
      perror("usleep");
      return EXIT_FAILURE;
    }
    if (count > 0)
      count--;
    i++;
  }
  close(sock);
  st_print(&stats);
  return EXIT_SUCCESS;
}
