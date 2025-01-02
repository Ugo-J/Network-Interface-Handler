#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>

#define BUFFER_SIZE 8192

struct route_info {
    char destination[32];
    char gateway[32];
    char source[32];
    char interface[IF_NAMESIZE];
    unsigned int metric;
    unsigned char protocol;
    unsigned char scope;
    unsigned char type;
};

// Helper function to convert protocol number to string
const char* get_protocol_name(unsigned char protocol) {
    switch (protocol) {
        case RTPROT_KERNEL:   return "kernel";
        case RTPROT_BOOT:     return "boot";
        case RTPROT_STATIC:   return "static";
        case RTPROT_DHCP:     return "dhcp";
        default:              return "unknown";
    }
}

// Helper function to convert scope to string
const char* get_scope_name(unsigned char scope) {
    switch (scope) {
        case RT_SCOPE_UNIVERSE: return "global";
        case RT_SCOPE_SITE:     return "site";
        case RT_SCOPE_LINK:     return "link";
        case RT_SCOPE_HOST:     return "host";
        case RT_SCOPE_NOWHERE:  return "nowhere";
        default:                return "unknown";
    }
}

// Helper function to convert type to string
const char* get_type_name(unsigned char type) {
    switch (type) {
        case RTN_UNSPEC:      return "unspec";
        case RTN_UNICAST:     return "unicast";
        case RTN_LOCAL:       return "local";
        case RTN_BROADCAST:   return "broadcast";
        case RTN_ANYCAST:     return "anycast";
        case RTN_MULTICAST:   return "multicast";
        case RTN_BLACKHOLE:   return "blackhole";
        case RTN_UNREACHABLE: return "unreachable";
        case RTN_PROHIBIT:    return "prohibit";
        case RTN_THROW:       return "throw";
        case RTN_NAT:         return "nat";
        case RTN_XRESOLVE:    return "xresolve";
        default:              return "unknown";
    }
}

// Function to create and initialize netlink socket
int create_netlink_socket() {
    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0) {
        perror("Failed to create socket");
        return -1;
    }

    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind socket");
        close(sock);
        return -1;
    }

    return sock;
}

// Function to send route request
int send_route_request(int sock) {
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
        return -1;
    }

    return 0;
}

// Function to parse a single route from response
void parse_route(struct nlmsghdr *nlh, struct route_info *rt_info) {
    struct rtmsg *rtm;
    struct rtattr *rta;
    int len;

    rtm = (struct rtmsg *)NLMSG_DATA(nlh);
    rta = RTM_RTA(rtm);
    len = RTM_PAYLOAD(nlh);

    memset(rt_info, 0, sizeof(struct route_info));

    // Store protocol, scope and type from rtm
    rt_info->protocol = rtm->rtm_protocol;
    rt_info->scope = rtm->rtm_scope;
    rt_info->type = rtm->rtm_type;

    // Iterate through all attributes
    for (; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
        switch (rta->rta_type) {
            case RTA_DST: {
                struct in_addr addr;
                addr.s_addr = *(unsigned int *)RTA_DATA(rta);
                inet_ntop(AF_INET, &addr, rt_info->destination, sizeof(rt_info->destination));
                break;
            }
            case RTA_GATEWAY: {
                struct in_addr addr;
                addr.s_addr = *(unsigned int *)RTA_DATA(rta);
                inet_ntop(AF_INET, &addr, rt_info->gateway, sizeof(rt_info->gateway));
                break;
            }
            case RTA_PREFSRC: {
                struct in_addr addr;
                addr.s_addr = *(unsigned int *)RTA_DATA(rta);
                inet_ntop(AF_INET, &addr, rt_info->source, sizeof(rt_info->source));
                break;
            }
            case RTA_OIF:
                if_indextoname(*(int *)RTA_DATA(rta), rt_info->interface);
                break;
            case RTA_PRIORITY:
                rt_info->metric = *(unsigned int *)RTA_DATA(rta);
                break;
        }
    }

    // If no destination was set, it's a default route
    if (strlen(rt_info->destination) == 0) {
        strcpy(rt_info->destination, "0.0.0.0");
    }
}

// Function to receive and parse response
int receive_route_response(int sock) {
    char buffer[BUFFER_SIZE];
    struct nlmsghdr *nlh;
    int received_bytes;
    struct route_info rt_info;

    while (1) {
        received_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (received_bytes < 0) {
            perror("Failed to receive response");
            return -1;
        }

        // Process all messages in the received data
        for (nlh = (struct nlmsghdr *)buffer; NLMSG_OK(nlh, received_bytes);
             nlh = NLMSG_NEXT(nlh, received_bytes)) {

            if (nlh->nlmsg_type == NLMSG_DONE) {
                return 0;
            }

            if (nlh->nlmsg_type == NLMSG_ERROR) {
                fprintf(stderr, "Error in netlink message\n");
                return -1;
            }

            parse_route(nlh, &rt_info);
            printf("Destination: %s\n", rt_info.destination);
            if (strlen(rt_info.gateway) > 0) {
                printf("Gateway: %s\n", rt_info.gateway);
            }
            if (strlen(rt_info.source) > 0) {
                printf("Source: %s\n", rt_info.source);
            }
            printf("Interface: %s\n", rt_info.interface);
            printf("Protocol: %s\n", get_protocol_name(rt_info.protocol));
            printf("Scope: %s\n", get_scope_name(rt_info.scope));
            printf("Type: %s\n", get_type_name(rt_info.type));
            if (rt_info.metric > 0) {
                printf("Metric: %u\n", rt_info.metric);
            }
            printf("\n");
        }
    }
}

int main() {
    int sock = create_netlink_socket();
    if (sock < 0) {
        return 1;
    }

    if (send_route_request(sock) < 0) {
        close(sock);
        return 1;
    }

    if (receive_route_response(sock) < 0) {
        close(sock);
        return 1;
    }

    close(sock);
    return 0;
}