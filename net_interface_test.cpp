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

        std::cout<<"Adding Interface "<<1<<" To Namespace\n";
        net_handle.add_network_interface(1);

        handle2 = curl_easy_init();
        curl_easy_setopt(handle2, CURLOPT_URL, "https://api.binance.com/api/v3/time");
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

    std::cout<<"Interface List Before Unshare\n";
    net_handle.get_network_interfaces();

    std::thread t1(thread_function);

    while(!check);

    curl_easy_setopt(handle, CURLOPT_URL, "https://api.binance.com/api/v3/time");
    std::cout<<"IP Address From Thread 1\n";
    curl_easy_perform(handle);
    //std::cout<<std::endl;
    // curl_easy_perform(handle2);
    std::cout<<std::endl;

    t1.join();

    /* std::cout<<"Interface List Before Unshare\n";
    handle.get_network_interfaces();

    if(net_interface_handler::net_ns_unshare())
        return 1;

    std::cout<<"Adding Interface "<<0<<" To Namespace\n";
    handle.add_network_interface(0);

    std::cout<<"Interface List After Adding To This Namespace\n";
    handle.get_network_interfaces(); */

    return 0;
}
