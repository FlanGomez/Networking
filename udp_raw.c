



// ==== RAW SOCKET ==== //
// ==================== //
// [      IP HEADER      ]
// [   OPTIONAL PROTOCOL    ] (TCP / UDP / ICMP)
// [      PAYLOAD        ]   (data)
// [   OPTIONAL FOOTER     ]

// ==================== //





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>


// Standard Checksum Function (RFC 1071)
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;
    for (sum = 0; len > 1; len -= 2) sum += *buf++;
    if (len == 1) sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
};

struct pseudo_header{
    uint32_t source;
    uint32_t dest;
    uint8_t  placeholder;
    uint8_t  protocol;
    uint16_t udp_len;
};

struct tos_bit{
    uint8_t precedence : 3;
    uint8_t delay : 1;
    uint8_t throughput : 1;
    uint8_t reliability : 1;
    uint8_t reserved : 2;
};
#define PAYLOAD "Hello UDP working ! PACKET RECV"

int main(){

    int sock,packet_ID = 0;
    char packet[ sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(PAYLOAD)];
    struct sockaddr_storage client_detail;
    socklen_t len = sizeof(client_detail);

    sock = socket(AF_INET,SOCK_RAW,IPPROTO_UDP);
    if(sock == -1){
        perror("\nSocket failed");
        return 1;
    }

    int on = 1;
    setsockopt(sock,IPPROTO_IP,IP_HDRINCL,&on , sizeof(int));
    struct tos_bit typeof_service;
    typeof_service.precedence = 7; // Networking
    typeof_service.delay = 1;
    typeof_service.throughput = 0;
    typeof_service.reliability = 1;
    typeof_service.reserved = 0;

    struct iphdr *ip = (struct iphdr*)packet;
    ip -> version = 4;
    ip -> ihl = 5;
    ip -> tos = *(uint8_t*)&typeof_service;
    ip -> id = htons(packet_ID++);
    ip -> frag_off = 0;
    ip -> ttl = 64;
    ip -> protocol = IPPROTO_UDP;
    ip -> check = 0;
    ip -> saddr = inet_addr("127.0.0.1");
    ip -> daddr = inet_addr("127.0.0.1");
    ip -> check = checksum(ip,sizeof(struct iphdr));

    struct udphdr *udp = (struct udphdr*)(packet + sizeof(struct iphdr));
    udp -> source = htons(12345);
    udp -> dest = htons(80);
    udp -> len = htons(sizeof(struct udphdr) + sizeof(PAYLOAD));
    ip -> tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(PAYLOAD));
    udp -> check = 0;
    memcpy(packet + sizeof(struct iphdr) + sizeof(struct udphdr),PAYLOAD , sizeof(PAYLOAD));

    struct pseudo_header pse;
    pse.source = inet_addr("127.0.0.1");
    pse.dest = inet_addr("127.0.0.1");
    pse.placeholder = 0;
    pse.protocol = IPPROTO_UDP;
    pse.udp_len = htons(sizeof(struct udphdr) + sizeof(PAYLOAD));
    char pseudo_packet[sizeof(struct pseudo_header) + sizeof(struct udphdr) + sizeof(PAYLOAD)];
    memcpy(pseudo_packet, &pse, sizeof(struct pseudo_header));
    memcpy(pseudo_packet + sizeof(struct pseudo_header), udp, sizeof(struct udphdr) + sizeof(PAYLOAD));
    udp->check = checksum(pseudo_packet, sizeof(pseudo_packet));

    struct sockaddr_in dest;
    memset(&dest,0,sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(80);
    dest.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(sendto(sock,packet,sizeof(packet),0,(struct sockaddr*)&dest, sizeof(dest)) == -1){
        perror("\nSEND FAILED");
    }
    printf("\nPACKET SENT");

    char buffer[1024];
    recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_detail, &len);
    printf("\nRECV : %s", buffer + sizeof(struct udphdr) + sizeof(struct iphdr));
    

    close(sock);
}
