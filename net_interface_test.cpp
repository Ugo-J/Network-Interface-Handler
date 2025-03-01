#include "network_interface_structure.hpp"
#include "network_interface_handler.hpp"
#include <curl/curl.h>
#include <thread>
#include <atomic>
#include <chrono>

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
        curl_easy_setopt(handle2, CURLOPT_URL, "https://ifconfig.me/");
        curl_easy_setopt(handle2, CURLOPT_USERAGENT, "curl/7.68.0");
        std::cout<<"IP Address From Thread 2\n";
        curl_easy_perform(handle2);
        std::cout<<std::endl;
        check = true;
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL); 
    handle = curl_easy_init();
    handle2 = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, "https://ifconfig.me/");
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "curl/7.68.0");
    curl_easy_setopt(handle2, CURLOPT_URL, "https://ifconfig.me/");
    curl_easy_setopt(handle2, CURLOPT_USERAGENT, "curl/7.68.0");

    for(int i = 0; i<3; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        curl_easy_perform(handle);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "\nRequest " << (i+1) << " took " << elapsed.count() << " seconds" << std::endl;
    }
    
    return 0;
}
