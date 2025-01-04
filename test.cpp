#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_link.h>
#include <sys/syscall.h>
#include <pthread.h>

#define BUFFER_SIZE 8192

// Get thread ID (gettid() wrapper)
static pid_t get_thread_id() {
    return syscall(SYS_gettid);
}

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

// Function to move interface to network namespace
int move_interface_to_netns(const char *if_name, pid_t target_tid) {
    int sock = create_netlink_socket();
    if (sock < 0) {
        return -1;
    }

    // Open the target thread's network namespace
    char ns_path[64];
    snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/net", target_tid);
    int target_ns_fd = open(ns_path, O_RDONLY);
    if (target_ns_fd < 0) {
        perror("Failed to open target namespace");
        close(sock);
        return -1;
    }

    // Prepare netlink message
    char msg_buffer[BUFFER_SIZE];
    struct nlmsghdr *nlh = (struct nlmsghdr *)msg_buffer;
    struct ifinfomsg *ifm;
    struct rtattr *rta;
    int ret = -1;

    memset(msg_buffer, 0, BUFFER_SIZE);

    // Setup netlink header
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    nlh->nlmsg_type = RTM_SETLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = getpid();

    // Setup interface info message
    ifm = NLMSG_DATA(nlh);
    ifm->ifi_family = AF_UNSPEC;
    ifm->ifi_index = if_nametoindex(if_name);
    if (ifm->ifi_index == 0) {
        perror("if_nametoindex");
        goto cleanup;
    }

    // Add network namespace FD attribute
    rta = (struct rtattr *)((char *)ifm + NLMSG_ALIGN(sizeof(struct ifinfomsg)));
    rta->rta_type = IFLA_NET_NS_FD;
    rta->rta_len = RTA_LENGTH(sizeof(int));
    memcpy(RTA_DATA(rta), &target_ns_fd, sizeof(int));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + RTA_LENGTH(sizeof(int));

    // Send message
    if (send(sock, nlh, nlh->nlmsg_len, 0) < 0) {
        perror("Failed to send netlink message");
        goto cleanup;
    }

    // Wait for response
    struct nlmsghdr response;
    ret = recv(sock, &response, sizeof(response), 0);
    if (ret < 0) {
        perror("Failed to get netlink response");
        goto cleanup;
    }

    // Check response
    if (response.nlmsg_type == NLMSG_ERROR) {
        struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(&response);
        if (err->error != 0) {
            fprintf(stderr, "Netlink error: %s\n", strerror(-err->error));
            ret = -1;
            goto cleanup;
        }
    }

    ret = 0;

cleanup:
    close(target_ns_fd);
    close(sock);
    return ret;
}

// Create veth pair function remains the same
int create_veth_pair(const char *name1, const char *name2) {
    // ... (same as before)
}

// Thread function that creates its own network namespace
void* network_namespace_thread(void* arg) {
    // Create new network namespace for this thread
    if (unshare(CLONE_NEWNET) < 0) {
        perror("unshare failed");
        return NULL;
    }

    pid_t tid = get_thread_id();
    printf("Thread %d created new network namespace\n", tid);

    // Keep thread running to maintain namespace
    while (1) {
        sleep(1);
    }

    return NULL;
}

// Example usage
int main() {
    pthread_t netns_thread;
    
    // Create thread with new network namespace
    if (pthread_create(&netns_thread, NULL, network_namespace_thread, NULL) != 0) {
        perror("pthread_create failed");
        return 1;
    }

    // Give thread time to start and create namespace
    sleep(1);

    // Get thread ID
    pid_t target_tid = get_thread_id();  // You'll need a way to get the actual thread ID
    
    // Create veth pair in current namespace
    if (create_veth_pair("veth0", "veth1") < 0) {
        fprintf(stderr, "Failed to create veth pair\n");
        return 1;
    }

    // Move veth1 to the new namespace
    if (move_interface_to_netns("veth1", target_tid) < 0) {
        fprintf(stderr, "Failed to move interface to namespace\n");
        return 1;
    }

    printf("Successfully moved veth1 to namespace of thread %d\n", target_tid);

    // Wait for thread (in practice, you might want to implement proper cleanup)
    pthread_join(netns_thread, NULL);

    return 0;
}