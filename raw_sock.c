



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

uint16_t packet_id = 1;

// Data👇🏼
#define PAYLOAD "It's great to connect with you today and dive into the technical world of checksums and data integrity.\n" \
                "Whether you're building a custom networking protocol or just curious about how systems verify data, " \
                "I'm here to help make the process smoother. Technical projects can be quite the puzzle, " \
                "but having the right tools and code snippets makes a world of difference. " \
                "I really appreciate your focus on getting the standard implementations right from the start. " \
                "Let's keep this momentum going as we refine your code or explore new concepts together. " \
                "Please let me know how I can best support your next steps in this project!"

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

int main(void){
    int sock,on = 1;
    char packet[sizeof(struct iphdr)+ sizeof(struct icmphdr) + sizeof(PAYLOAD)];       // created packet of size the struct iphdr(20 bytes) + size of PAYLOAD
    memset(packet,0,sizeof(packet));
    sock = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);             // create a raw socket given parameter (  AF_INET = IPv4 , SOCK_RAW = type , IPPROTO_ICMP =  Protocol used )
    setsockopt(sock,IPPROTO_IP,IP_HDRINCL,&on,sizeof(on));   // Build our own IP header else not set then it is set by default

    struct iphdr *ip = (struct iphdr *)packet;  // ip (pointer variable) set to pointer casting the packet as a struct iphdr
    ip -> version = 4;                                            // IPv4
    ip -> ihl = 5;                                                // 5 rows in IP Header formate each row has 32 bits = 4byte so 5 x 4 = 20
    ip -> tos = 0;                                                // Type of service is set to default
    ip -> frag_off = 0;                                           // if packet exceed the size within MTU then its broken into smaller parts known as fragments
    ip -> ttl = 64;                                               // Time to live (if packet gets stuck into a loop a specific time given to keep it alive)
    ip -> id = htons(packet_id++);                                // and if packet is broken each fragment is given an ID
    ip -> protocol = IPPROTO_ICMP;                                 // Here protocol is set to ICMP
    ip -> check = 0;                                              // check is assigned 0 so no garbage value present in it
    ip -> saddr = inet_addr("127.0.0.1");                         // set the address from where to packet is going
    ip -> daddr = inet_addr("127.0.0.1");                         // set the address where the address is going

    struct icmphdr *icmp = (struct icmphdr*)(packet + sizeof(struct iphdr));
    icmp -> type = ICMP_ECHO;
    icmp -> code = 0;
    icmp->un.echo.id = htons(1);
    icmp->un.echo.sequence = htons(1);

    memcpy(packet + sizeof(struct iphdr) + sizeof(struct icmphdr), PAYLOAD, sizeof(PAYLOAD));// it copies the payload bytes into the packet after the struct iphdr
    ip -> tot_len = htons(sizeof(struct iphdr)+ sizeof(struct icmphdr) + sizeof(PAYLOAD));// Total length is set (the struct iphdr 20 bytes + Payload)
    ip -> check = checksum(packet,sizeof(struct iphdr));          // Checksum function used to check the packet delieverd to dest reached properly or not
    icmp->checksum = 0;
    icmp->checksum = checksum(icmp, sizeof(struct icmphdr) + sizeof(PAYLOAD));


    struct sockaddr_in dest;               // this struct is used to set the destination info
    memset(&dest,0,sizeof(dest));
    dest.sin_family = AF_INET;               // assigned which IPv version is this
    dest.sin_port = htons(80);               // assigned port
    dest.sin_addr.s_addr = inet_addr("127.0.0.1"); // and destination address

    

    if(sendto(sock,packet,ntohs(ip -> tot_len), 0 ,(struct sockaddr *)&dest,sizeof(dest)) == -1){
        printf("\nSent Failed");
        return 1;
    }

    printf("\nPacket Sent");
    close(sock);
    
}
