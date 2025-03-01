#include "network_interface_structure.hpp"
#include "network_interface_handler.hpp"
#include <curl/curl.h>
#include <thread>
#include <atomic>

CURL* handle;
CURL* handle2;
struct curl_slist * headers = NULL;
net_interface_handler net_handle;
std::atomic<bool> check = false;

void thread_function(){
    if(!net_interface_handler::net_ns_unshare()){

        std::cout<<"Adding Interface "<<0<<" To Namespace\n";
        net_handle.add_network_interface(0);

        net_handle.get_network_interfaces();

        std::cout<<"Loopback Route In New Namespace"<<std::endl;

        for(int i=0; i<net_interface_handler::loopback_interface.num_of_routes; i++)
            net_interface_handler::print_route_info(&net_interface_handler::loopback_interface.route_array[i]);

        handle2 = curl_easy_init();
        curl_easy_setopt(handle2, CURLOPT_VERBOSE, 1L);
        /* curl_easy_setopt(handle2, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(handle2, CURLOPT_SSL_VERIFYHOST, 0L);
        headers = curl_slist_append(headers, "Host: ifconfig.me");  // The actual hostname
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");  // A common user agent
        headers = curl_slist_append(headers, "Accept: text/plain");  // Request plain text response
        headers = curl_slist_append(headers, "User-Agent: curl");
        curl_easy_setopt(handle2, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(handle2, CURLOPT_FOLLOWLOCATION, 1L); */
        // curl_easy_setopt(handle2, CURLOPT_URL, "https://34.160.111.145/ip");
        curl_easy_setopt(handle2, CURLOPT_URL, "https://ifconfig.me/");
        std::cout<<"IP Address From Thread 2\n";
        curl_easy_perform(handle2);
        std::cout<<std::endl;
        // net_handle.get_network_interfaces();
        check = true;

    }
    
}

int main() {

    curl_global_init(CURL_GLOBAL_ALL); 
    handle = curl_easy_init();
    handle2 = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, "https://ifconfig.me/");
    curl_easy_setopt(handle2, CURLOPT_URL, "https://ifconfig.me/");

    /* std::cout<<"Interface List Before Unshare\n";
    net_handle.get_network_interfaces();

    std::cout<<net_handle.loopback_interface.index<<'\n';
    std::cout<<net_handle.loopback_interface.name<<'\n';
    std::cout<<net_handle.loopback_interface.addr_str<<'\n';

    for(int i = 0; i<net_handle.num_of_network_interfaces; i++){

        std::cout<<net_handle.interface_array[i].index<<'\n';
        std::cout<<net_handle.interface_array[i].name<<'\n';
        std::cout<<net_handle.interface_array[i].addr_str<<'\n';

    } */

    // curl_easy_setopt(handle, CURLOPT_INTERFACE, net_handle.interface_array[0].addr_str);
    // curl_easy_setopt(handle2, CURLOPT_INTERFACE, net_handle.interface_array[1].addr_str);

    for(int i = 0; i<3; i++)

        curl_easy_perform(handle);

    // curl_easy_perform(handle2);

    return 0;
}
