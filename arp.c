#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <unistd.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <time.h>


#define src_MAC {0x00,0x00,0x00,0x00,0x00,0x0}
#define src_IP "192.168.x.x"
#define target_IP "192.168.x.x"
#define target_MAC {0xff,0xff,0xff,0xff,0xff,0xff}



struct arp_header{
    uint16_t hardware_type;     // MAC address
    uint16_t protocol_type;     // IP address
    uint8_t hardware_len;       // MAC address = 6bytes
    uint8_t protocol_len;       // IPv4 address = 4bytes
    uint16_t operation;         // ARP reply  |  ARP request
    uint8_t sender_mac[6];      // Sender's MAC (My computer's MAC address)
    uint8_t sender_ip[4];       // Sender's IP  (My computer's private IP address)
    uint8_t target_mac[6];      // Target's MAC address
    uint8_t target_ip[4];       // Target's IP address
};

struct ethernet_header{
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ether_type;
};

#define PAYLOAD " ARP DATA : hello user"

int main(void){

    uint8_t mac_address[] = src_MAC;
    uint8_t dst_mac[] = target_MAC;

    int sock_fd = socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ARP));
    char packet[sizeof(struct arp_header) + sizeof(struct ethernet_header) + sizeof(PAYLOAD)];

    struct ethernet_header *eth = (struct ethernet_header*)packet;
    memcpy(eth ->src_mac,mac_address,6);
    memcpy(eth->dst_mac, dst_mac, 6);
    eth ->ether_type = htons(0x0806);    // ARP
    
    
    struct arp_header *arp = (struct arp_header*)(sizeof(struct ethernet_header) + packet);
    arp -> hardware_type = htons(1);  // ETHERNET
    arp -> protocol_type = htons(0x0800);  // IPv4
    arp -> hardware_len = 6;
    arp -> protocol_len = 4;
    arp -> operation = htons(1);
    memcpy(arp -> sender_mac,mac_address,6);
    inet_pton(AF_INET,src_IP,arp ->sender_ip);
    memcpy(arp -> target_mac,dst_mac,6);
    inet_pton(AF_INET,target_IP,arp ->target_ip);
    


    struct sockaddr_ll device = {0};
    device.sll_ifindex = if_nametoindex("eth0");
    device.sll_family = AF_PACKET;
    device.sll_halen = 6;
    memset(device.sll_addr,0xff,6);

        if(sendto(sock_fd,packet,sizeof(packet),0,(struct sockaddr*)&device,sizeof(device)) == -1){
            perror("\nSend Failed");
            close(sock_fd);
            return 1;
        }
        printf("\nPacket Sent Successfully ");
    
    close(sock_fd);

}