#include "network_interface_handler.hpp"

int main() {
    net_interface_handler handle;
    std::cout<<"Interface List Before Unshare\n";
    handle.get_network_interfaces();

    if(net_interface_handler::net_ns_unshare())
        return 1;

    std::cout<<"Adding Interface "<<1<<" To Namespace\n";
    handle.add_network_interface(1);

    std::cout<<"Interface List After Adding To This Namespace\n";
    handle.get_network_interfaces();

    return 0;
}
