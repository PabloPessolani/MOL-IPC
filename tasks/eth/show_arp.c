#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <asm/types.h>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define PROTO_ARP 0x0806
#define ETH2_HEADER_LEN 14
#define HW_TYPE 1
#define PROTOCOL_TYPE 0x800
#define MAC_LENGTH 6
#define IPV4_LENGTH 4
#define ARP_REQUEST 0x01
#define ARP_REPLY 0x02
#define BUF_SIZE 60

struct arp_header
{
        unsigned short hardware_type;
        unsigned short protocol_type;
        unsigned char hardware_len;
        unsigned char  protocol_len;
        unsigned short opcode;
        unsigned char sender_mac[MAC_LENGTH];
        unsigned char sender_ip[IPV4_LENGTH];
        unsigned char target_mac[MAC_LENGTH];
        unsigned char target_ip[IPV4_LENGTH];
};

int main()
{
        int sd;
        unsigned char buffer[BUF_SIZE];
        unsigned char source_ip[4] = {10,222,190,160};
        unsigned char target_ip[4] = {10,222,190,139};
        struct ifreq ifr;
        struct ethhdr *send_req = (struct ethhdr *)buffer;
        struct ethhdr *rcv_resp= (struct ethhdr *)buffer;
        struct arp_header *arp_req = (struct arp_header *)(buffer+ETH2_HEADER_LEN);
        struct arp_header *arp_resp = (struct arp_header *)(buffer+ETH2_HEADER_LEN);
        struct sockaddr_ll socket_address;
        int index,ret,length=0,ifindex;

memset(buffer,0x00,60);
        /*open socket*/
        sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (sd == -1) {
                perror("socket():");
                exit(1);
        }
        strcpy(ifr.ifr_name,"eth1.30");
    /*retrieve ethernet interface index*/
    if (ioctl(sd, SIOCGIFINDEX, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        exit(1);
    }
    ifindex = ifr.ifr_ifindex;
printf("interface index is %d\n",ifindex);

        /*retrieve corresponding MAC*/
        if (ioctl(sd, SIOCGIFHWADDR, &ifr) == -1) {
                perror("SIOCGIFINDEX");
                exit(1);
        }
close (sd);

        for (index = 0; index < 6; index++)
        {

                send_req->h_dest[index] = (unsigned char)0xff;
                arp_req->target_mac[index] = (unsigned char)0x00;
                /* Filling the source  mac address in the header*/
                send_req->h_source[index] = (unsigned char)ifr.ifr_hwaddr.sa_data[index];
                arp_req->sender_mac[index] = (unsigned char)ifr.ifr_hwaddr.sa_data[index];
                socket_address.sll_addr[index] = (unsigned char)ifr.ifr_hwaddr.sa_data[index];
        }
        printf("Successfully got eth1 MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                        send_req->h_source[0],send_req->h_source[1],send_req->h_source[2],
                        send_req->h_source[3],send_req->h_source[4],send_req->h_source[5]);
        printf(" arp_reqMAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                        arp_req->sender_mac[0],arp_req->sender_mac[1],arp_req->sender_mac[2],
                        arp_req->sender_mac[3],arp_req->sender_mac[4],arp_req->sender_mac[5]);
        printf("socket_address MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                        socket_address.sll_addr[0],socket_address.sll_addr[1],socket_address.sll_addr[2],
                        socket_address.sll_addr[3],socket_address.sll_addr[4],socket_address.sll_addr[5]);

        /*prepare sockaddr_ll*/
        socket_address.sll_family = AF_PACKET;
        socket_address.sll_protocol = htons(ETH_P_ARP);
        socket_address.sll_ifindex = ifindex;
        socket_address.sll_hatype = htons(ARPHRD_ETHER);
        socket_address.sll_pkttype = (PACKET_BROADCAST);
        socket_address.sll_halen = MAC_LENGTH;
        socket_address.sll_addr[6] = 0x00;
        socket_address.sll_addr[7] = 0x00;

        /* Setting protocol of the packet */
        send_req->h_proto = htons(ETH_P_ARP);

        /* Creating ARP request */
        arp_req->hardware_type = htons(HW_TYPE);
        arp_req->protocol_type = htons(ETH_P_IP);
        arp_req->hardware_len = MAC_LENGTH;
        arp_req->protocol_len =IPV4_LENGTH;
        arp_req->opcode = htons(ARP_REQUEST);
        for(index=0;index<5;index++)
        {
                arp_req->sender_ip[index]=(unsigned char)source_ip[index];
                arp_req->target_ip[index]=(unsigned char)target_ip[index];
        }
  // Submit request for a raw socket descriptor.
  if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

buffer[32]=0x00;
        ret = sendto(sd, buffer, 42, 0, (struct  sockaddr*)&socket_address, sizeof(socket_address));
        if (ret == -1)
        {
                perror("sendto():");
                exit(1);
        }
        else
        {
                printf(" Sent the ARP REQ \n\t");
                for(index=0;index<42;index++)
                {
                        printf("%02X ",buffer[index]);
                        if(index % 16 ==0 && index !=0)
                        {printf("\n\t");}
                }
        }
printf("\n\t");
        memset(buffer,0x00,60);
        while(1)
        {
                length = recvfrom(sd, buffer, BUF_SIZE, 0, NULL, NULL);
                if (length == -1)
                {
                        perror("recvfrom():");
                        exit(1);
                }
                if(htons(rcv_resp->h_proto) == PROTO_ARP)
                {
                        //if( arp_resp->opcode == ARP_REPLY )
                        printf(" RECEIVED ARP RESP len=%d \n",length);
                        printf(" Sender IP :");
                        for(index=0;index<4;index++)
                                printf("%u.",(unsigned int)arp_resp->sender_ip[index]);

                        printf("\n Sender MAC :");
                        for(index=0;index<6;index++)
                                printf(" %02X:",arp_resp->sender_mac[index]);

                        printf("\nReceiver  IP :");
                        for(index=0;index<4;index++)
                                printf(" %u.",arp_resp->target_ip[index]);

                        printf("\n Self MAC :");
                        for(index=0;index<6;index++)
                                printf(" %02X:",arp_resp->target_mac[index]);

                        printf("\n  :");

                        break;
                }
        }

        return 0;
}
Thanks a lot once more Arun Kumar P

shareimprove this answer
answered May 17 at 5:52

user6343961
111
  	 	
Thanks for the code, I used it as a base for mine – Mikko Korkalo Sep 2 at 8:11
add a comment
up vote
0
down vote
I took user6343961's code, did some cleaning and splicing and implemented support for automatically getting interface IP address. Also the parameters come from CLI instead of hardcoding. bind() is also used to get only ARP from the interface we want. Have fun. This code works for me.

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>  //htons etc

#define PROTO_ARP 0x0806
#define ETH2_HEADER_LEN 14
#define HW_TYPE 1
#define MAC_LENGTH 6
#define IPV4_LENGTH 4
#define ARP_REQUEST 0x01
#define ARP_REPLY 0x02
#define BUF_SIZE 60

#define debug(x...) printf(x);printf("\n");
#define info(x...) printf(x);printf("\n");
#define warn(x...) printf(x);printf("\n");
#define err(x...) printf(x);printf("\n");

struct arp_header {
    unsigned short hardware_type;
    unsigned short protocol_type;
    unsigned char hardware_len;
    unsigned char protocol_len;
    unsigned short opcode;
    unsigned char sender_mac[MAC_LENGTH];
    unsigned char sender_ip[IPV4_LENGTH];
    unsigned char target_mac[MAC_LENGTH];
    unsigned char target_ip[IPV4_LENGTH];
};

/*
 * Converts struct sockaddr with an IPv4 address to network byte order uin32_t.
 * Returns 0 on success.
 */
int int_ip4(struct sockaddr *addr, uint32_t *ip)
{
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *i = (struct sockaddr_in *) addr;
        *ip = i->sin_addr.s_addr;
        return 0;
    } else {
        err("Not AF_INET");
        return 1;
    }
}

/*
 * Formats sockaddr containing IPv4 address as human readable string.
 * Returns 0 on success.
 */
int format_ip4(struct sockaddr *addr, char *out)
{
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *i = (struct sockaddr_in *) addr;
        const char *ip = inet_ntoa(i->sin_addr);
        if (!ip) {
            return -2;
        } else {
            strcpy(out, ip);
            return 0;
        }
    } else {
        return -1;
    }
}

/*
 * Writes interface IPv4 address as network byte order to ip.
 * Returns 0 on success.
 */
int get_if_ip4(int fd, const char *ifname, uint32_t *ip) {
    int err = -1;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    if (strlen(ifname) > (IFNAMSIZ - 1)) {
        err("Too long interface name");
        goto out;
    }

    strcpy(ifr.ifr_name, ifname);
    if (ioctl(fd, SIOCGIFADDR, &ifr) == -1) {
        perror("SIOCGIFADDR");
        goto out;
    }

    if (int_ip4(&ifr.ifr_addr, ip)) {
        goto out;
    }
    err = 0;
out:
    return err;
}

/*
 * Sends an ARP who-has request to dst_ip
 * on interface ifindex, using source mac src_mac and source ip src_ip.
 */
int send_arp(int fd, int ifindex, const unsigned char *src_mac, uint32_t src_ip, uint32_t dst_ip)
{
    int err = -1;
    unsigned char buffer[BUF_SIZE];
    memset(buffer, 0, sizeof(buffer));

    struct sockaddr_ll socket_address;
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_ARP);
    socket_address.sll_ifindex = ifindex;
    socket_address.sll_hatype = htons(ARPHRD_ETHER);
    socket_address.sll_pkttype = (PACKET_BROADCAST);
    socket_address.sll_halen = MAC_LENGTH;
    socket_address.sll_addr[6] = 0x00;
    socket_address.sll_addr[7] = 0x00;

    struct ethhdr *send_req = (struct ethhdr *) buffer;
    struct arp_header *arp_req = (struct arp_header *) (buffer + ETH2_HEADER_LEN);
    int index;
    ssize_t ret, length = 0;

    //Broadcast
    memset(send_req->h_dest, 0xff, MAC_LENGTH);

    //Target MAC zero
    memset(arp_req->target_mac, 0x00, MAC_LENGTH);

    //Set source mac to our MAC address
    memcpy(send_req->h_source, src_mac, MAC_LENGTH);
    memcpy(arp_req->sender_mac, src_mac, MAC_LENGTH);
    memcpy(socket_address.sll_addr, src_mac, MAC_LENGTH);

    /* Setting protocol of the packet */
    send_req->h_proto = htons(ETH_P_ARP);

    /* Creating ARP request */
    arp_req->hardware_type = htons(HW_TYPE);
    arp_req->protocol_type = htons(ETH_P_IP);
    arp_req->hardware_len = MAC_LENGTH;
    arp_req->protocol_len = IPV4_LENGTH;
    arp_req->opcode = htons(ARP_REQUEST);

    debug("Copy IP address to arp_req");
    memcpy(arp_req->sender_ip, &src_ip, sizeof(uint32_t));
    memcpy(arp_req->target_ip, &dst_ip, sizeof(uint32_t));

    ret = sendto(fd, buffer, 42, 0, (struct sockaddr *) &socket_address, sizeof(socket_address));
    if (ret == -1) {
        perror("sendto():");
        goto out;
    }
    err = 0;
out:
    return err;
}

/*
 * Gets interface information by name:
 * IPv4
 * MAC
 * ifindex
 */
int get_if_info(const char *ifname, uint32_t *ip, char *mac, int *ifindex)
{
    debug("get_if_info for %s", ifname);
    int err = -1;
    struct ifreq ifr;
    int sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sd <= 0) {
        perror("socket()");
        goto out;
    }
    if (strlen(ifname) > (IFNAMSIZ - 1)) {
        printf("Too long interface name, MAX=%i\n", IFNAMSIZ - 1);
        goto out;
    }

    strcpy(ifr.ifr_name, ifname);

    //Get interface index using name
    if (ioctl(sd, SIOCGIFINDEX, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        goto out;
    }
    *ifindex = ifr.ifr_ifindex;
    printf("interface index is %d\n", *ifindex);

    //Get MAC address of the interface
    if (ioctl(sd, SIOCGIFHWADDR, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        goto out;
    }

    //Copy mac address to output
    memcpy(mac, ifr.ifr_hwaddr.sa_data, MAC_LENGTH);

    if (get_if_ip4(sd, ifname, ip)) {
        goto out;
    }
    debug("get_if_info OK");

    err = 0;
out:
    if (sd > 0) {
        debug("Clean up temporary socket");
        close(sd);
    }
    return err;
}

/*
 * Creates a raw socket that listens for ARP traffic on specific ifindex.
 * Writes out the socket's FD.
 * Return 0 on success.
 */
int bind_arp(int ifindex, int *fd)
{
    debug("bind_arp: ifindex=%i", ifindex);
    int ret = -1;

    // Submit request for a raw socket descriptor.
    *fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (*fd < 1) {
        perror("socket()");
        goto out;
    }

    debug("Binding to ifindex %i", ifindex);
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(struct sockaddr_ll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifindex;
    if (bind(*fd, (struct sockaddr*) &sll, sizeof(struct sockaddr_ll)) < 0) {
        perror("bind");
        goto out;
    }

    ret = 0;
out:
    if (ret && *fd > 0) {
        debug("Cleanup socket");
        close(*fd);
    }
    return ret;
}

/*
 * Reads a single ARP reply from fd.
 * Return 0 on success.
 */
int read_arp(int fd)
{
    debug("read_arp");
    int ret = -1;
    unsigned char buffer[BUF_SIZE];
    ssize_t length = recvfrom(fd, buffer, BUF_SIZE, 0, NULL, NULL);
    int index;
    if (length == -1) {
        perror("recvfrom()");
        goto out;
    }
    struct ethhdr *rcv_resp = (struct ethhdr *) buffer;
    struct arp_header *arp_resp = (struct arp_header *) (buffer + ETH2_HEADER_LEN);
    if (ntohs(rcv_resp->h_proto) != PROTO_ARP) {
        debug("Not an ARP packet");
        goto out;
    }
    if (ntohs(arp_resp->opcode) != ARP_REPLY) {
        debug("Not an ARP reply");
        goto out;
    }
    debug("received ARP len=%ld", length);
    struct in_addr sender_a;
    memset(&sender_a, 0, sizeof(struct in_addr));
    memcpy(&sender_a.s_addr, arp_resp->sender_ip, sizeof(uint32_t));
    debug("Sender IP: %s", inet_ntoa(sender_a));

    debug("Sender MAC: %02X:%02X:%02X:%02X:%02X:%02X",
          arp_resp->sender_mac[0],
          arp_resp->sender_mac[1],
          arp_resp->sender_mac[2],
          arp_resp->sender_mac[3],
          arp_resp->sender_mac[4],
          arp_resp->sender_mac[5]);

    ret = 0;

out:
    return ret;
}

/*
 *
 * Sample code that sends an ARP who-has request on
 * interface <ifname> to IPv4 address <ip>.
 * Returns 0 on success.
 */
int test_arping(const char *ifname, const char *ip) {
    int ret = -1;
    uint32_t dst = inet_addr(ip);
    if (dst == 0 || dst == 0xffffffff) {
        printf("Invalid source IP\n");
        return 1;
    }

    int src;
    int ifindex;
    char mac[MAC_LENGTH];
    if (get_if_info(ifname, &src, mac, &ifindex)) {
        err("get_if_info failed, interface %s not found or no IP set?", ifname);
        goto out;
    }
    int arp_fd;
    if (bind_arp(ifindex, &arp_fd)) {
        err("Failed to bind_arp()");
        goto out;
    }

    if (send_arp(arp_fd, ifindex, mac, src, dst)) {
        err("Failed to send_arp");
        goto out;
    }

    while(1) {
        int r = read_arp(arp_fd);
        if (r == 0) {
            info("Got reply, break out");
            break;
        }
    }

    ret = 0;
out:
    if (arp_fd) {
        close(arp_fd);
        arp_fd = 0;
    }
    return ret;
}

int main(int argc, const char **argv) {
    int ret = -1;
    if (argc != 3) {
        printf("Usage: %s <INTERFACE> <DEST_IP>\n", argv[0]);
        return 1;
    }
    const char *ifname = argv[1];
    const char *ip = argv[2];
    return test_arping(ifname, ip);
}