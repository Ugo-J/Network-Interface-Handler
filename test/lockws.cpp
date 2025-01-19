#include "lockws.hpp"
#include <thread>
#include "network_interface_handler.hpp"

net_interface_handler handle;

lock_client second;
lock_client third;
in_addr interface_address;

void second_thread() {
   std::cout<<"Thread 2"<<std::endl;
   if(third.connect("wss://testnet.binance.vision:443", "/ws-api/v3", &(handle.interface_array[1].ip_addr), handle.interface_array[1].name))
      std::cout<<third.get_error_message()<<std::endl;
   for(int i=0; i<10; i++)
      third.send(R"({"id":-1,"method":"time"})");
   for(int i=0; i<10; i++)
      third.basic_read();
}

int main(){

   handle.get_network_interfaces();

   if(second.connect("wss://testnet.binance.vision:443", "/ws-api/v3", &(handle.interface_array[0].ip_addr), handle.interface_array[0].name))
      std::cout<<second.get_error_message()<<std::endl;
   std::cout<<"Thread 1"<<std::endl;
   for(int i=0; i<10; i++)
      second.send(R"({"id":-1,"method":"time"})");
   for(int i=0; i<10; i++)
      second.basic_read();
   
   std::thread t1(second_thread);

   t1.join();
   std::cout<<"Thread 1"<<std::endl;
   second.send(R"({"id":-1,"method":"time"})");
   second.basic_read();

}
