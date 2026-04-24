



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

#define PAYLOAD "Hello Flanoy, TCP WORKING"
uint16_t packetID = 1;

int main(void){
    //  IP HEADER  + TCP HEADER + PAYLOAD
    int sock,check_opt;
    int on = 1;
    char packet[sizeof(struct iphdr) + sizeof(struct tcphdr) + sizeof(PAYLOAD)];
    //uint32_t seq_ID = 0;
    char recv_buffer[1024];
    struct sockaddr_in client_storage;
    socklen_t len = sizeof(client_storage);
    
    

    sock = socket(AF_INET,SOCK_RAW,IPPROTO_TCP);
    if(sock == -1){
        perror("Socket Failed");
        return 1;
    }

    if((check_opt = setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(int))) == -1){
        perror("setsockopt failed");
    }
    

    struct iphdr *ip = (struct iphdr*)packet;
    ip -> version = 4;
    ip -> ihl = 5;
    ip -> tos = 0;
    ip -> frag_off = 0;
    ip -> id = htons(packetID++);
    ip -> ttl = 8;
    ip -> protocol = IPPROTO_TCP;
    ip -> check = 0;
    ip -> saddr = inet_addr("127.0.0.1");
    ip -> daddr = inet_addr("127.0.0.1");
    ip -> check = checksum(packet,sizeof(struct iphdr));

    struct tcphdr *tcp = (struct tcphdr*)(packet + sizeof(struct iphdr));
    tcp -> source = htons(12345);
    tcp -> dest = htons(80);
    tcp -> seq = htonl(0);
    tcp -> ack_seq = htonl(0);
    tcp -> doff = 5;
    tcp -> syn = 1;
    tcp -> ack = 0;
    tcp -> window = htons(65535);
    ip -> tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr) + sizeof(PAYLOAD));
    tcp -> check = 0;
    memcpy(packet + sizeof(struct iphdr) + sizeof(struct tcphdr), PAYLOAD , sizeof(PAYLOAD));

    struct pseudo_header{
        uint32_t src_addr;
        uint32_t dst_addr;
        uint8_t placeholder;
        uint8_t protocol;
        uint16_t tcp_length;
    };
    
    struct pseudo_header psh;
    psh.src_addr   = ip->saddr;
    psh.dst_addr   = ip->daddr;
    psh.placeholder = 0;
    psh.protocol   = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr));
    char pseudo_packet[sizeof(struct pseudo_header) + sizeof(struct tcphdr)];
    memcpy(pseudo_packet, &psh, sizeof(struct pseudo_header));
    memcpy(pseudo_packet + sizeof(struct pseudo_header), tcp, sizeof(struct tcphdr));
    tcp->check = checksum(pseudo_packet, sizeof(pseudo_packet));
    tcp -> check = checksum(packet , sizeof(struct tcphdr));

    struct sockaddr_in dest;
    memset(&dest, 0 , sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(80);
    dest.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(sendto(sock, packet, ntohs(ip -> tot_len),0,(struct sockaddr*)&dest , sizeof(dest)) == -1){
        perror("\nSend Failed");
        return 1;
    }

    if(recvfrom(sock, recv_buffer, ntohs(ip -> tot_len),0, (struct sockaddr*)&client_storage, &len) == -1){
        perror("\nRecv Failed");
        return 1;
    }
    printf("\nSender : %s",recv_buffer + sizeof(struct iphdr) + sizeof(struct tcphdr));

    printf("\nPacket Sent");
    close(sock);

}
