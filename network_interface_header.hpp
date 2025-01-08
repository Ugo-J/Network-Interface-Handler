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
#include <fcntl.h>
// for the unshare system call
#include <sched.h>
// for the syscall to fetch the kernel thread id
#include <sys/syscall.h>

/*  // Network interface statuses
    void print_interface_status(unsigned int flags) {
    printf("Status: ");
    if (flags & IFF_UP) printf("UP ");
    if (flags & IFF_RUNNING) printf("RUNNING ");
    if (flags & IFF_LOOPBACK) printf("LOOPBACK ");
    if (flags & IFF_BROADCAST) printf("BROADCAST ");
    if (flags & IFF_MULTICAST) printf("MULTICAST ");
    if (flags & IFF_PROMISC) printf("PROMISCUOUS ");
    printf("\n");
} */

class net_interface_handler {

public:
    int get_network_interfaces();
    int add_network_interface(int if_index);

    // the system uses this variable to keep track of the number of interfaces that have been entered into the interface array. This variable should be accessible to the program independent of an object
    inline static int num_of_network_interfaces = 0;

public:
    // function to unshare a thread from the global network namespace. This function is made static
    static int net_ns_unshare(){
        int error = unshare(CLONE_NEWNET);

        if(error != 0){
            if(error == ENOMEM){
        
                std::cout<<"Insufficient Memory To Unshare From Network Namespace\n";
                return error;
        
            }
            else if(error == EINVAL){
                
                std::cout<<"Invalid Network Namespace Unsharing Configuration\n";
                return error;
                
            }
            else if(error == EPERM){
                
                std::cout<<"Permission Denied For Unsharing From Global Network Namespace\n";
                return error;
                
            }
            else{
                std::cout<<"Unsharing From Global Network Namespace Failed...Retry With Root Privilege\n";
                return error;
            }
        }

        return error;
    }

private:
    // Structure to store route attributes
    struct route_info {
        struct in_addr dst_addr;
        struct in_addr src_addr;
        struct in_addr gateway;
        unsigned int ifindex;
        unsigned char protocol;
        unsigned char scope;
        unsigned char type;
        unsigned int flags;
        unsigned int metric;
        unsigned int prefixlen;
    };

private:
    struct network_interface {
        char name[IF_NAMESIZE];
        int index;
        unsigned char scope;
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

        static const int ROUTE_ARRAY_SIZE = 128;

        // route array
        route_info route_array[ROUTE_ARRAY_SIZE];

        // route count
        int num_of_routes = 0;
    };

private:
    // size of the internal buffer used for the network interface API calls
    static const int BUFFER_SIZE = 32 * 1024; // 32KB

    // this variable holds the size of the interface array in which the network interface details are stored
    static const int INTERFACE_ARRAY_SIZE = 128;

    // this is the interface array that holds the details of the network interfaces on the system
    network_interface interface_array[INTERFACE_ARRAY_SIZE];

public:
    // we create a static network interface structure to hold the details of the loopback address because this is used to reconfigure the loopback address in new network namespaces - we initialise the num_of_routes to 0 and the loopback interface index to 1 because the loopback interface always has a device index of 1
    inline static network_interface loopback_interface{.index=1, .num_of_routes=0};

    // bool used to test whether the loopback interface details have been stored in the loopback interface structure
    inline static bool loopback_interface_set = false;

private:
    void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
        memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
        
        while (RTA_OK(rta, len)) {
            if (rta->rta_type <= max) {
                tb[rta->rta_type] = rta;
            }
            rta = RTA_NEXT(rta, len);
        }
    }

    static void addAttr(struct nlmsghdr* nh, int type, const void* data, int len) {
        struct rtattr* rta = (struct rtattr*)((char*)nh + nh->nlmsg_len);
        rta->rta_type = type;
        rta->rta_len = RTA_LENGTH(len);
        memcpy(RTA_DATA(rta), data, len);
        nh->nlmsg_len += RTA_LENGTH(len);
    }

private:
    void print_route_info(struct route_info *route);
    bool send_netlink_request(int sock, int type, int seq);
    bool send_route_request(int sock);
    void process_link_info(struct nlmsghdr *nlh);
    void process_addr_info(struct nlmsghdr *nlh);
    void process_route_info(struct nlmsghdr *nlh);
    int move_interface_to_netns(int if_index, pid_t target_tid);
    int bring_up_loopback(); // function for bringing up the loopback interface
    int configure_loopback_address();
    int bring_up_interface(int if_index); // function for bringing up interface
    int configure_interface_address(int if_index);
    // functions to add routes
    int add_interface_routes(int index);
    int add_direct_route(struct in_addr& dst, int if_index, unsigned char protocol, unsigned char scope, unsigned char type, int prefixlen, int metric = 0);
    int add_gateway_route(struct in_addr& dst, struct in_addr& gw, int if_index, unsigned char protocol, unsigned char scope, unsigned char type, int prefixlen, int metric = 0);
};
