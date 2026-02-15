#include <chrono>
#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>

int main(){
    // Pin to CPU core 0
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    
    // Set highest priority (-20 is highest, 19 is lowest)
    setpriority(PRIO_PROCESS, 0, -20);
    
    std::chrono::_V2::system_clock::time_point start = std::chrono::high_resolution_clock::now();
    uint64_t count = 0;
    for(int i = 0; i<1'000'000'000; i++)
        count++;
    std::chrono::duration<double> seconds = std::chrono::high_resolution_clock::now() - start;
    std::cout<<seconds.count()<<"s\n";
}