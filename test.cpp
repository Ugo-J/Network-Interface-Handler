#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define BUFFER_SIZE 8192

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

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
    memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
    
    while (RTA_OK(rta, len)) {
        if (rta->rta_type <= max) {
            tb[rta->rta_type] = rta;
        }
        rta = RTA_NEXT(rta, len);
    }
}

void print_route_info(struct route_info *route) {
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
    printf("Interface: %s\n", route->ifname);
    
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

int get_routing_table() {
    int sock;
    struct sockaddr_nl nl_addr;
    struct nlmsghdr *nlh;
    struct rtmsg *rtm;
    struct route_info route_info;
    char msg_buffer[BUFFER_SIZE];
    int len, ret;

    // Create netlink socket
    sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0) {
        perror("Failed to create socket");
        return -1;
    }

    // Initialize netlink address
    memset(&nl_addr, 0, sizeof(nl_addr));
    nl_addr.nl_family = AF_NETLINK;
    nl_addr.nl_pid = getpid();
    
    if (bind(sock, (struct sockaddr *)&nl_addr, sizeof(nl_addr)) < 0) {
        perror("Failed to bind socket");
        close(sock);
        return -1;
    }

    // Prepare request message
    memset(msg_buffer, 0, BUFFER_SIZE);
    nlh = (struct nlmsghdr *)msg_buffer;
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = getpid();

    // Send request
    if (send(sock, nlh, nlh->nlmsg_len, 0) < 0) {
        perror("Failed to send request");
        close(sock);
        return -1;
    }

    printf("\n=== Routing Table Information ===\n\n");

    // Receive and process responses
    while (1) {
        len = recv(sock, msg_buffer, BUFFER_SIZE, 0);
        if (len < 0) {
            perror("Failed to receive response");
            close(sock);
            return -1;
        }

        nlh = (struct nlmsghdr *)msg_buffer;
        while (NLMSG_OK(nlh, len)) {
            if (nlh->nlmsg_type == NLMSG_DONE) {
                close(sock);
                return 0;
            }

            if (nlh->nlmsg_type == NLMSG_ERROR) {
                fprintf(stderr, "Error in netlink message\n");
                close(sock);
                return -1;
            }

            if (nlh->nlmsg_type == RTM_NEWROUTE) {
                memset(&route_info, 0, sizeof(route_info));
                rtm = (struct rtmsg *)NLMSG_DATA(nlh);
                struct rtattr *tb[RTA_MAX + 1];
                parse_rtattr(tb, RTA_MAX, RTM_RTA(rtm), RTM_PAYLOAD(nlh));

                // Get route attributes
                if (tb[RTA_DST])
                    memcpy(&route_info.dst_addr, RTA_DATA(tb[RTA_DST]), sizeof(route_info.dst_addr));

                if (tb[RTA_GATEWAY])
                    memcpy(&route_info.gateway, RTA_DATA(tb[RTA_GATEWAY]), sizeof(route_info.gateway));

                if (tb[RTA_PREFSRC])
                    memcpy(&route_info.src_addr, RTA_DATA(tb[RTA_PREFSRC]), sizeof(route_info.src_addr));

                if (tb[RTA_OIF])
                    route_info.ifindex = *(int *)RTA_DATA(tb[RTA_OIF]);

                if (tb[RTA_METRICS])
                    route_info.metric = *(unsigned int *)RTA_DATA(tb[RTA_METRICS]);

                route_info.protocol = rtm->rtm_protocol;
                route_info.scope = rtm->rtm_scope;
                route_info.type = rtm->rtm_type;
                route_info.flags = rtm->rtm_flags;

                // Get interface name from index
                if_indextoname(route_info.ifindex, route_info.ifname);

                print_route_info(&route_info);
            }
            nlh = NLMSG_NEXT(nlh, len);
        }
    }

    close(sock);
    return 0;
}

int main() {
    return get_routing_table();
}