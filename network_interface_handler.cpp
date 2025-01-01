#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <iostream>

// Structure to store route attributes
struct route_info {
    struct in_addr dst_addr;
    struct in_addr src_addr;
    struct in_addr gateway;
    char ifname[IF_NAMESIZE];
    unsigned int ifindex;
    unsigned char protocol;
    unsigned char scope;
    unsigned char type;
    unsigned int flags;
    unsigned int metric;
};

struct network_interface {
    char name[IF_NAMESIZE];
    int index;
    unsigned int flags;
    struct in_addr ip_addr;
    struct in_addr netmask;

    // rtattr structure for the interface IPV4 address
    struct rtattr *tb[IFA_MAX + 1];
    // rtattr structure for the interface IPV6 address
    struct rtattr *tb_ipv6[IFA_MAX + 1];
    // this char array is used to store the ipv4 address as a c-string
    char addr_str[INET_ADDRSTRLEN];
    // this char array is used to store the ipv6 address as a c-string
    char addr_str_ipv6[INET_ADDRSTRLEN];
    // interface address IPV4 prefix length
    int ifa_prefixlen;
    // interface address IPV6 prefix length
    int ifa_prefixlen_ipv6;

    // route array
};

// size of the internal buffer used for the network interface API calls
const int BUFFER_SIZE = 32 * 1024; // 32KB

// this variable holds the size of the interface array in which the network interface details are stored
const int INTERFACE_ARRAY_SIZE = 128;

// the system uses this variable to keep track of the number of interfaces that have been entered into the interface array
int num_of_network_interfaces = 0;

// this is the interface array that holds the details of the network interfaces on the system
network_interface interface_array[INTERFACE_ARRAY_SIZE];

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
    memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
    
    while (RTA_OK(rta, len)) {
        if (rta->rta_type <= max) {
            tb[rta->rta_type] = rta;
        }
        rta = RTA_NEXT(rta, len);
    }
}

/* void print_interface_status(unsigned int flags) {
    printf("Status: ");
    if (flags & IFF_UP) printf("UP ");
    if (flags & IFF_RUNNING) printf("RUNNING ");
    if (flags & IFF_LOOPBACK) printf("LOOPBACK ");
    if (flags & IFF_BROADCAST) printf("BROADCAST ");
    if (flags & IFF_MULTICAST) printf("MULTICAST ");
    if (flags & IFF_PROMISC) printf("PROMISCUOUS ");
    printf("\n");
} */

bool send_netlink_request(int sock, int type, int seq) {
    char msg_buffer[BUFFER_SIZE];
    struct nlmsghdr *nlh = (struct nlmsghdr *)msg_buffer;

    // Prepare request message
    memset(msg_buffer, 0, BUFFER_SIZE);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    nlh->nlmsg_type = type;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = seq;
    nlh->nlmsg_pid = 0;

    if (send(sock, nlh, nlh->nlmsg_len, 0) < 0) {
        perror("Failed to send request");
        return false;
    }
    return true;
}

void process_link_info(struct nlmsghdr *nlh) {
    struct ifinfomsg *ifi = static_cast<struct ifinfomsg *>(NLMSG_DATA(nlh));
    struct rtattr *rta = IFLA_RTA(ifi);
    int rta_len = IFLA_PAYLOAD(nlh);
    // we make the local network interface structure be a reference to the current network interface entry in the interface array
    network_interface& current_if = interface_array[num_of_network_interfaces];

    memset(&current_if, 0, sizeof(current_if));
    current_if.index = ifi->ifi_index;
    current_if.flags = ifi->ifi_flags;
    strncpy(current_if.name, static_cast<const char *>(RTA_DATA(rta)), IF_NAMESIZE - 1);

    // we check if this interface is UP and RUNNING because this system should store the details of only up and running interfaces, we also check that the interface is not the loopback interface, so we only increment the num_of_network_interfaces if these conditions are met
    if((ifi->ifi_flags & IFF_UP) && (ifi->ifi_flags & IFF_RUNNING) && !(ifi->ifi_flags & IFF_LOOPBACK)){
    // with this condition we filter only interfaces that are UP, RUNNING and not the loopback interface

        // we increment the num_of_network_interfaces
        num_of_network_interfaces++;
    
    }

}

void process_addr_info(struct nlmsghdr *nlh) {
    struct ifaddrmsg *ifa = static_cast<struct ifaddrmsg *>(NLMSG_DATA(nlh));
    struct rtattr *rta = IFA_RTA(ifa);
    int rta_len = IFA_PAYLOAD(nlh);
    int index = -1;

    // we loop through the entered network interfaces to find the index of this network interface in the interface array
    for(int i = 0; i<num_of_network_interfaces; i++){
        if(interface_array[i].index == ifa->ifa_index)
            index = i;
    }

    if(index > -1){
    // this only runs if a valid index was found

        if(ifa->ifa_family == AF_INET){
        // this is an IPV4 address

            // we store the interface address prefix length
            interface_array[index].ifa_prefixlen = ifa->ifa_prefixlen;

            // we parse the rtattr structure
            parse_rtattr(interface_array[index].tb, IFA_MAX, rta, rta_len);

            // convert the address structure to a c string and store in the interface structure
            if (interface_array[index].tb[IFA_ADDRESS]) {
                
                inet_ntop(ifa->ifa_family, RTA_DATA(interface_array[index].tb[IFA_ADDRESS]), interface_array[index].addr_str, sizeof(interface_array[index].addr_str));

            }
            
        }
        else{
        // this is an IPV6 address

            // we store the interface address prefix length
            interface_array[index].ifa_prefixlen_ipv6 = ifa->ifa_prefixlen;

            // we parse the rtattr structure
            parse_rtattr(interface_array[index].tb_ipv6, IFA_MAX, rta, rta_len);

            // convert the address structure into a c string and store it in the interface array
            if (interface_array[index].tb_ipv6[IFA_ADDRESS]) {
                
                inet_ntop(ifa->ifa_family, RTA_DATA(interface_array[index].tb_ipv6[IFA_ADDRESS]), interface_array[index].addr_str_ipv6, sizeof(interface_array[index].addr_str_ipv6));

            }

        }

    }
}

void process_route_info(struct nlmsghdr *nlh) {

}

int get_network_interfaces() {
    int sock;
    struct sockaddr_nl nl_addr;
    char msg_buffer[BUFFER_SIZE];
    int recv_len;
    
    // Create netlink socket
    sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0) {
        perror("Failed to create socket");
        return -1;
    }

    // Initialize netlink address
    memset(&nl_addr, 0, sizeof(nl_addr));
    nl_addr.nl_family = AF_NETLINK;
    nl_addr.nl_pid = 0;
    
    if (bind(sock, (struct sockaddr *)&nl_addr, sizeof(nl_addr)) < 0) {
        perror("Failed to bind socket");
        close(sock);
        return -1;
    }

    // we now query network interface
    if (!send_netlink_request(sock, RTM_GETLINK, 0)) {
        std::cout<<"Error Sending RTM_GETLINK Netlink Request\n";
        close(sock);
        return -1;
    }

    // we read the response for this request, the response function stores the interface details in the interface array
    while ((recv_len = recv(sock, msg_buffer, BUFFER_SIZE, 0)) > 0) {
        struct nlmsghdr *nlh = (struct nlmsghdr *)msg_buffer;
        
        while (NLMSG_OK(nlh, recv_len)) {
            if (nlh->nlmsg_type == NLMSG_DONE) {
                break;
            }

            if (nlh->nlmsg_type == RTM_NEWLINK) {
                process_link_info(nlh);
            }
            else if (nlh->nlmsg_type == RTM_NEWADDR) {
                process_addr_info(nlh);
            }
            else if (nlh->nlmsg_type == RTM_NEWROUTE) {
                process_route_info(nlh);
            }

            nlh = NLMSG_NEXT(nlh, recv_len);
        }

        if (nlh->nlmsg_type == NLMSG_DONE) {
            break;
        }
    }

    // we query the ip address of interfaces that are up and running
    if (!send_netlink_request(sock, RTM_GETADDR, 0)) {
        std::cout<<"Error Sending RTM_GETADDR Netlink Request\n";
        close(sock);
        return -1;
    }

    // we read the response for this request, the response function stores the interface address details in the interface array
    while ((recv_len = recv(sock, msg_buffer, BUFFER_SIZE, 0)) > 0) {
        struct nlmsghdr *nlh = (struct nlmsghdr *)msg_buffer;
        
        while (NLMSG_OK(nlh, recv_len)) {
            if (nlh->nlmsg_type == NLMSG_DONE) {
                break;
            }

            if (nlh->nlmsg_type == RTM_NEWLINK) {
                process_link_info(nlh);
            }
            else if (nlh->nlmsg_type == RTM_NEWADDR) {
                process_addr_info(nlh);
            }
            else if (nlh->nlmsg_type == RTM_NEWROUTE) {
                process_route_info(nlh);
            }

            nlh = NLMSG_NEXT(nlh, recv_len);
        }

        if (nlh->nlmsg_type == NLMSG_DONE) {
            break;
        }
    }

    // then we query the routing table
    if (!send_netlink_request(sock, RTM_GETROUTE, 0)) {
        std::cout<<"Error Sending RTM_GETROUTE Netlink Request\n";
        close(sock);
        return -1;
    }

    // we now read the responses from the route request
    while ((recv_len = recv(sock, msg_buffer, BUFFER_SIZE, 0)) > 0) {
        struct nlmsghdr *nlh = (struct nlmsghdr *)msg_buffer;
        
        while (NLMSG_OK(nlh, recv_len)) {
            if (nlh->nlmsg_type == NLMSG_DONE) {
                break;
            }

            if (nlh->nlmsg_type == RTM_NEWLINK) {
                process_link_info(nlh);
            }
            else if (nlh->nlmsg_type == RTM_NEWADDR) {
                process_addr_info(nlh);
            }
            else if (nlh->nlmsg_type == RTM_NEWROUTE) {
                process_route_info(nlh);
            }

            nlh = NLMSG_NEXT(nlh, recv_len);
        }

        if (nlh->nlmsg_type == NLMSG_DONE) {
            break;
        }
    }

    close(sock);
    return 0;
}

int main() {
    get_network_interfaces();

    printf("\n=== Network Interface Details (Netlink) ===\n\n");

    for(int i = 0; i<num_of_network_interfaces; i++){
        network_interface& current_if = interface_array[i];
        printf("Interface: %s\n", current_if.name);
        printf("Index: %d\n", current_if.index);
        std::cout<<current_if.addr_str<<"\n";
    }

    return 0;
}