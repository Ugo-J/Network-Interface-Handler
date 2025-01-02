#include "network_interface_header.hpp"

void net_interface_handler::print_route_info(struct route_info *route) {
    char dst[INET_ADDRSTRLEN];
    char gw[INET_ADDRSTRLEN];
    char src[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &route->dst_addr, dst, sizeof(dst));
    inet_ntop(AF_INET, &route->gateway, gw, sizeof(gw));
    inet_ntop(AF_INET, &route->src_addr, src, sizeof(src));

    printf("Destination: %s\n", dst);
    if (route->gateway.s_addr != 0)
        printf("Gateway: %s\n", gw);
    if (route->src_addr.s_addr != 0)
        printf("Source: %s\n", src);
    
    printf("Protocol: ");
    switch(route->protocol) {
        case RTPROT_BOOT: printf("boot\n"); break;
        case RTPROT_KERNEL: printf("kernel\n"); break;
        case RTPROT_STATIC: printf("static\n"); break;
        case RTPROT_DHCP: printf("dhcp\n"); break;
        default: printf("unknown(%d)\n", route->protocol);
    }

    printf("Scope: ");
    switch(route->scope) {
        case RT_SCOPE_NOWHERE: printf("nowhere\n"); break;
        case RT_SCOPE_HOST: printf("host\n"); break;
        case RT_SCOPE_LINK: printf("link\n"); break;
        case RT_SCOPE_SITE: printf("site\n"); break;
        case RT_SCOPE_UNIVERSE: printf("global\n"); break;
        default: printf("unknown(%d)\n", route->scope);
    }

    printf("Type: ");
    switch(route->type) {
        case RTN_UNSPEC: printf("unspec\n"); break;
        case RTN_UNICAST: printf("unicast\n"); break;
        case RTN_LOCAL: printf("local\n"); break;
        case RTN_BROADCAST: printf("broadcast\n"); break;
        case RTN_ANYCAST: printf("anycast\n"); break;
        case RTN_MULTICAST: printf("multicast\n"); break;
        case RTN_BLACKHOLE: printf("blackhole\n"); break;
        case RTN_UNREACHABLE: printf("unreachable\n"); break;
        case RTN_PROHIBIT: printf("prohibit\n"); break;
        case RTN_THROW: printf("throw\n"); break;
        case RTN_NAT: printf("nat\n"); break;
        default: printf("unknown(%d)\n", route->type);
    }

    if (route->metric)
        printf("Metric: %u\n", route->metric);
    printf("\n");
}

bool net_interface_handler::send_netlink_request(int sock, int type, int seq) {
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

// Function to send route request
bool net_interface_handler::send_route_request(int sock) {
    char msg_buffer[BUFFER_SIZE];
    struct nlmsghdr *nlh = (struct nlmsghdr *)msg_buffer;
    struct rtmsg *rtm;

    memset(msg_buffer, 0, BUFFER_SIZE);

    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = getpid();

    rtm = (rtmsg*)NLMSG_DATA(nlh);
    rtm->rtm_family = AF_INET;
    rtm->rtm_table = RT_TABLE_MAIN;

    if (send(sock, nlh, nlh->nlmsg_len, 0) < 0) {
        perror("Failed to send request");
        return false;
    }

    return true;
}

void net_interface_handler::process_link_info(struct nlmsghdr *nlh) {
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

void net_interface_handler::process_addr_info(struct nlmsghdr *nlh) {
    struct ifaddrmsg *ifa = static_cast<struct ifaddrmsg *>(NLMSG_DATA(nlh));
    struct rtattr *rta = IFA_RTA(ifa);
    int rta_len = IFA_PAYLOAD(nlh);
    int index = -1;

    // we loop through the entered network interfaces to find the index of this network interface in the interface array
    for(int i = 0; i<num_of_network_interfaces; i++){
        if(interface_array[i].index == ifa->ifa_index){
            index = i;
            break;
        }
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

void net_interface_handler::process_route_info(struct nlmsghdr *nlh) {

    struct rtmsg *rtm;
    rtm = (struct rtmsg *)NLMSG_DATA(nlh);
    struct rtattr *tb[RTA_MAX + 1];
    parse_rtattr(tb, RTA_MAX, RTM_RTA(rtm), RTM_PAYLOAD(nlh));

    // we first fetch the interace index
    int index = -1;

    // now we check if the index with this route is in the interface array
    for(int i = 0; i<num_of_network_interfaces; i++){
        if(interface_array[i].index == *(int *)RTA_DATA(tb[RTA_OIF])){
            index = i;
            break;
        }
    }

    // this part only runs if the interface with this route was found in the interface array
    if(index > -1){

        // we create a local reference to the next route entry in the route array at this index of the interface array
        struct route_info& route_info = interface_array[index].route_array[interface_array[index].num_of_routes];
        memset(&route_info, 0, sizeof(route_info));

        // Get route attributes
        if (tb[RTA_DST])
            memcpy(&route_info.dst_addr, RTA_DATA(tb[RTA_DST]), sizeof(route_info.dst_addr));

        if (tb[RTA_GATEWAY])
            memcpy(&route_info.gateway, RTA_DATA(tb[RTA_GATEWAY]), sizeof(route_info.gateway));

        if (tb[RTA_PREFSRC])
            memcpy(&route_info.src_addr, RTA_DATA(tb[RTA_PREFSRC]), sizeof(route_info.src_addr));

        if (tb[RTA_OIF])
            route_info.ifindex = *(int *)RTA_DATA(tb[RTA_OIF]);

        if (tb[RTA_METRICS]){
            route_info.metric = *(unsigned int *)RTA_DATA(tb[RTA_METRICS]);
        }

        // this RTA_PRIORITY is used to collect the route metric if it is not returned in the tb[RTA_METRICS] pointer
        if (tb[RTA_PRIORITY]){
            route_info.metric = *(unsigned int *)RTA_DATA(tb[RTA_PRIORITY]);
        }

        route_info.protocol = rtm->rtm_protocol;
        route_info.scope = rtm->rtm_scope;
        route_info.type = rtm->rtm_type;
        route_info.flags = rtm->rtm_flags;

        // we increment the num of routes for this interface
        interface_array[index].num_of_routes++;

    }

}

int net_interface_handler::get_network_interfaces() {

    // we first set the number of network interfaces back to 0
    num_of_network_interfaces = 0;

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
    if (!send_route_request(sock)) {
        std::cout<<"Error Sending Route Request\n";
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

    // print the routes for this interface
    for(int i = 0; i<num_of_network_interfaces; i++)
        for(int j = 0; j<interface_array[i].num_of_routes; j++){
            printf("Interface: %s\n", interface_array[i].name);
            print_route_info(&interface_array[i].route_array[j]);
        }

    return 0;
}