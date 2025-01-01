#include "network_interface_handler.hpp"

int main() {
    net_interface_handler handle;
    handle.get_network_interfaces();

    /* printf("\n=== Network Interface Details (Netlink) ===\n\n");

    for(int i = 0; i<num_of_network_interfaces; i++){
        network_interface& current_if = interface_array[i];
        printf("Interface: %s\n", current_if.name);
        printf("Index: %d\n", current_if.index);
        std::cout<<current_if.addr_str<<"\n";
    } */

    return 0;
}
