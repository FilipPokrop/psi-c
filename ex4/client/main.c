#include "icmp_types.h"
#include "ip_protocol_numbers.h"
#include <arpa/inet.h>
#include <complex.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFERSIZE 2048
#define ETHERNET_HEADER_SIZE 14

volatile sig_atomic_t end = 0;

struct ip_packet {
  u_int8_t ihl : 4, version : 4;
  u_int8_t TOS;
  u_int16_t total_length;
  u_int16_t identification;
  u_int8_t flags : 3;
  u_int16_t fragment_offset : 13;
  u_int8_t time_to_live;
  u_int8_t protocol;
  u_int16_t header_checksum;
  u_int32_t source_addres;
  u_int32_t destination_addres;
  char data[];
};

struct tcp_packet {
  u_int16_t source_port;
  u_int16_t destination_port;
  u_int32_t sequence_number;
  u_int32_t acknowlegment_number;
  u_int8_t flags : 6, : 6, data_offset : 4;
  u_int16_t window;
  u_int16_t checksum;
  u_int16_t urgent_pointer;
  char data[];
};

struct udp_packet {
  u_int16_t source_port;
  u_int16_t destination_port;
  u_int16_t length;
  u_int16_t checksum;
  char data[];
};

struct icmp_packet {
  u_int8_t type;
  u_int8_t code;
  u_int16_t checksum;
  char data[];
};

void handle_sigint(int signum) { end = 1; }

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
void *handle_ethernet(void *data, int data_size) {
  if (data_size < ETHERNET_HEADER_SIZE) {
    fprintf(stderr, "ethernet");
    return NULL;
  }
  return data + ETHERNET_HEADER_SIZE;
}

int handle_ip(void **data, int size, struct ip_packet **packet) {

  *packet = *data;

  if ((*packet)->version != 4) {
    printf("wrong ip version %d\n", (*packet)->version);
    return -1;
  }
  if (in_cksum(*data, ((*packet)->ihl * 4)) != 0) {
    printf("wrong checksum\n");
    return -2;
  }
  *data += (*packet)->ihl * 4;
  return size - (*packet)->ihl * 4;
}

int handle_icmp(void **data, int size, struct icmp_packet **packet) {
  *packet = *data;
  if (size < 8) {
    fprintf(stderr, "ICMP header size must be greater or equal 8\n");
    return -1;
  }
  if (in_cksum(*data, size) != 0) {
    fprintf(stderr, "Wrong checksum in ICMP header\n");
    return -2;
  }
  *data = data + 8;
  return size - 8;
}

int handle_tcp(void **data, int size, struct tcp_packet **packet) {
  if (size < 20) {
    fprintf(stderr, "TCP header size must be greater or equal 20\n");
    return -1;
  }
  *packet = *data;
  *data += (*packet)->data_offset;
  return size - (*packet)->data_offset;
}
int handle_udp(void **data, int size, struct udp_packet **packet) {
  if (size < 8) {
    fprintf(stderr, "TCP header size must be greater or equal 20\n");
    return -1;
  }

  *packet = *data;
  *data += 8;
  return size - 8;
}

int main(int argc, char *argv[]) {
  signal(SIGINT, handle_sigint);
  int sock;
  if ((sock = socket(AF_PACKET, SOCK_RAW, htons(0x03))) == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  char buff[BUFFERSIZE];
  while (!end) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    int fds_count;
    fds_count = select(sock + 1, &fds, NULL, NULL, NULL);
    if (end) {
      break;
    }
    if (fds_count < 0) {
      perror("select");
      break;
    }
    if (!FD_ISSET(sock, &fds)) {
      continue;
    }
    memset(buff, 0, sizeof(buff));
    int data_size = recv(sock, buff, sizeof(buff), 0);
    // printf("%d\n", data_size);

    char *processed_data = handle_ethernet(buff, data_size);
    struct ip_packet *ip_header;
    int ip_data_size;
    if ((ip_data_size = handle_ip((void **)&processed_data, data_size - 14,
                                  &ip_header)) < 0) {
      continue;
    }
    char src_addr_str[INET_ADDRSTRLEN];
    char dst_addr_str[INET_ADDRSTRLEN];

    if (inet_ntop(AF_INET, &ip_header->source_addres, src_addr_str,
                  sizeof(src_addr_str)) == NULL ||
        inet_ntop(AF_INET, &ip_header->destination_addres, dst_addr_str,
                  sizeof(dst_addr_str)) == NULL) {
      perror("inet_ntop\n");
      continue;
    }
    if (ip_header->protocol == 1) {
      struct icmp_packet *icmp_header;
      int icmp_data_size;
      if ((icmp_data_size = handle_icmp((void **)&processed_data, ip_data_size,
                                        &icmp_header)) < 0) {
        continue;
      }
      printf("%15s -> %-15s ", src_addr_str, dst_addr_str);
      printf("%-5s", IPProtocols[ip_header->protocol]);
      printf("%6hu ", htons(ip_header->total_length));
      printf("%-23s, ttl=%hhu", ICMPTypes[icmp_header->type],
             ip_header->time_to_live);
      printf("\n");
      continue;
    } else if (ip_header->protocol == 17) {
      struct udp_packet *udp_header;
      int udp_data_size =
          handle_udp((void **)&processed_data, ip_data_size, &udp_header);
      printf("%15s -> %-15s ", src_addr_str, dst_addr_str);
      printf("%-5s", IPProtocols[ip_header->protocol]);
      printf("%6hu ", htons(ip_header->total_length));
      printf("%6hu -> %-6hu", htons(udp_header->source_port),
             htons(udp_header->destination_port));
      printf("Len=%hu", htons(udp_header->length));
      printf("\n");
      continue;
    } else if (ip_header->protocol == 6) {
      struct tcp_packet *tcp_header;
      int tcp_data_size =
          handle_tcp((void **)&processed_data, ip_data_size, &tcp_header);
      printf("%15s -> %-15s ", src_addr_str, dst_addr_str);
      printf("%-5s", IPProtocols[ip_header->protocol]);
      printf("%6hu ", htons(ip_header->total_length));
      printf("%6hu -> %-6hu", htons(tcp_header->source_port),
             htons(tcp_header->destination_port));
      printf("Seq=%u Ack=%u Win=%hu Len=%d", htonl(tcp_header->sequence_number),
             htonl(tcp_header->acknowlegment_number), htons(tcp_header->window),
             tcp_data_size);
      printf("\n");
      continue;
    }

    printf("%15s -> %-15s ", src_addr_str, dst_addr_str);
    printf("%-5s", IPProtocols[ip_header->protocol]);
    printf("%6hu", htons(ip_header->total_length));
    printf("\n");
  }

  printf("end app\n");
  close(sock);
  return 0;
}
