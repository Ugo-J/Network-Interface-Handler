# Network Interface Handler Library

A header-only C++ library for managing network interfaces and network namespaces on Linux systems using the Netlink protocol. This library provides a clean interface for querying, configuring, and isolating network interfaces across different network namespaces.

## Features

- Query and enumerate network interfaces on the system
- Retrieve detailed interface information (IP addresses, routes, flags)
- Move network interfaces between network namespaces
- Configure interface addresses and bring interfaces up/down
- Manage routing tables (add direct and gateway routes)
- Support for network namespace isolation
- Loopback interface management
- IPv4 support with IPv6 structure (IPv6 parsing currently disabled)

## Requirements

- Linux kernel with Netlink support
- C++11 or later
- Root privileges (required for network namespace operations)
- Standard Linux headers: `<linux/netlink.h>`, `<linux/rtnetlink.h>`

## Installation

This is a header-only library. Simply include the necessary headers in your project:

```cpp
#include "network_interface_structure.hpp"
#include "network_interface_handler.hpp"
```

## Architecture

The library consists of three main header files:

1. **network_interface_headers.hpp** - System includes and dependencies
2. **network_interface_structure.hpp** - Core data structures and class definition
3. **network_interface_handler.hpp** - Implementation of all methods

## Core Components

### Class: `net_interface_handler`

The main class providing all network interface management functionality.

#### Key Data Structures

**`network_interface`** - Stores complete interface information:
- Interface name and index
- IPv4/IPv6 addresses (as strings and binary)
- Network flags (UP, RUNNING, LOOPBACK, etc.)
- Prefix length and scope
- Routing table (up to 128 routes per interface)

**`route_info`** - Stores routing table entries:
- Destination, source, and gateway addresses
- Protocol, scope, type, and flags
- Metric and prefix length

### Static Members

```cpp
// Total number of active network interfaces
static int num_of_network_interfaces;

// Loopback interface details (always available)
static network_interface loopback_interface;

// Flag indicating if loopback has been configured
static bool loopback_interface_set;
```

## Usage Examples

### Basic Interface Enumeration

```cpp
#include "network_interface_structure.hpp"
#include "network_interface_handler.hpp"

int main() {
    net_interface_handler handler;
    
    // Query all network interfaces
    if (handler.get_network_interfaces() == 0) {
        std::cout << "Found " << handler.num_of_network_interfaces 
                  << " interfaces" << std::endl;
        
        // Access interface details
        for (int i = 0; i < handler.num_of_network_interfaces; i++) {
            std::cout << "Interface: " << handler.interface_array[i].name 
                      << " (" << handler.interface_array[i].addr_str << ")" 
                      << std::endl;
        }
    }
    
    return 0;
}
```

### Network Namespace Isolation

```cpp
#include "network_interface_structure.hpp"
#include "network_interface_handler.hpp"
#include <thread>

void isolated_network_thread() {
    // Create new network namespace for this thread
    if (net_interface_handler::net_ns_unshare() == 0) {
        net_interface_handler handler;
        
        // Move interface 0 to this namespace
        handler.add_network_interface(0);
        
        // Query interfaces in new namespace
        handler.get_network_interfaces();
        
        // Now this thread has isolated network access
        // Perform network operations...
    }
}

int main() {
    std::thread t(isolated_network_thread);
    t.join();
    return 0;
}
```

### Viewing Route Information

```cpp
net_interface_handler handler;
handler.get_network_interfaces();

// Print routes for first interface
for (int i = 0; i < handler.interface_array[0].num_of_routes; i++) {
    net_interface_handler::print_route_info(
        &handler.interface_array[0].route_array[i]
    );
}
```

## API Reference

### Public Methods

#### `int get_network_interfaces()`
Queries the system for all active network interfaces and populates the internal interface array.

**Returns:** `0` on success, `-1` on failure

**Behavior:**
- Resets interface counter to 0
- Queries interface addresses via Netlink RTM_GETADDR
- Queries routing tables via RTM_GETROUTE
- Filters for UP and RUNNING interfaces (excludes loopback)
- Stores loopback interface separately

#### `int add_network_interface(int index)`
Moves a network interface to the current thread's network namespace.

**Parameters:**
- `index` - Index in the interface_array (0 to num_of_network_interfaces-1)

**Returns:** `0` on success, `-1` on failure

**Behavior:**
- Validates index range
- Moves interface from root namespace to current namespace
- Brings up loopback interface in new namespace
- Brings up the moved interface
- Configures interface address
- Recreates routing table entries

#### `static int net_ns_unshare()`
Creates a new network namespace for the calling thread.

**Returns:** `0` on success, error code on failure

**Error Codes:**
- `ENOMEM` - Insufficient memory
- `EINVAL` - Invalid configuration
- `EPERM` - Permission denied (requires root)

### Private Methods

#### Interface Configuration
- `int bring_up_loopback()` - Activates loopback interface
- `int bring_up_interface(int if_index)` - Brings interface up
- `int configure_loopback_address()` - Sets 127.0.0.1/8 on loopback
- `int configure_interface_address(int index)` - Configures interface IP

#### Routing
- `int add_direct_route(...)` - Adds a direct route (no gateway)
- `int add_gateway_route(...)` - Adds a route via gateway
- `int add_interface_routes(int index)` - Recreates all routes for interface

#### Netlink Communication
- `bool send_netlink_request(int sock, int type, int seq)` - Sends generic request
- `bool send_route_request(int sock)` - Queries routing table
- `void process_link_info(struct nlmsghdr *nlh)` - Processes link messages
- `void process_addr_info(struct nlmsghdr *nlh)` - Processes address messages
- `void process_route_info(struct nlmsghdr *nlh)` - Processes route messages

#### Utilities
- `static void print_route_info(struct route_info *route)` - Pretty-prints route details
- `void parse_rtattr(...)` - Parses Netlink route attributes
- `static void addAttr(...)` - Adds attributes to Netlink messages

## Configuration Constants

```cpp
static const int BUFFER_SIZE = 32 * 1024;        // 32KB Netlink buffer
static const int INTERFACE_ARRAY_SIZE = 128;     // Max interfaces
static const int ROUTE_ARRAY_SIZE = 128;         // Max routes per interface
```

## Interface Filtering

The library automatically filters interfaces based on these criteria:

**Included:**
- Interfaces with IFF_UP flag set
- Interfaces with IFF_RUNNING flag set
- Interfaces with assigned IPv4 addresses

**Excluded:**
- Loopback interface (stored separately)
- Down interfaces
- Interfaces without IP addresses
- IPv6-only interfaces (currently)

## Network Namespace Workflow

When moving an interface to a new namespace:

1. Thread calls `net_ns_unshare()` to create isolated namespace
2. Thread calls `add_network_interface(index)` with desired interface
3. Library switches to root namespace temporarily
4. Interface is moved via Netlink RTM_SETLINK with IFLA_NET_NS_FD
5. Library switches back to target namespace
6. Loopback interface is brought up and configured (127.0.0.1/8)
7. Moved interface is brought up
8. Interface IP address is configured
9. All routing table entries are recreated

## Limitations

- IPv6 support is structurally present but disabled in processing
- Maximum 128 interfaces supported
- Maximum 128 routes per interface
- Requires root/CAP_NET_ADMIN privileges
- Linux-specific (uses Netlink protocol)

## Error Handling

Most methods return `-1` on failure and print error messages to stderr/stdout. Check return values and ensure:

- Running with sufficient privileges (root or CAP_NET_ADMIN)
- Kernel supports required Netlink operations
- Interface indices are valid before operations

## Thread Safety

- Each thread can have its own network namespace
- Static members (`num_of_network_interfaces`, `loopback_interface`) are shared
- Use separate `net_interface_handler` instances per thread for namespace isolation

## Example: Multi-Interface Isolation

```cpp
#include "network_interface_structure.hpp"
#include "network_interface_handler.hpp"
#include <thread>
#include <vector>

void worker_thread(int interface_index) {
    if (net_interface_handler::net_ns_unshare() == 0) {
        net_interface_handler handler;
        handler.add_network_interface(interface_index);
        
        // This thread now has exclusive access to this interface
        // Perform isolated network operations...
    }
}

int main() {
    net_interface_handler handler;
    handler.get_network_interfaces();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < handler.num_of_network_interfaces; i++) {
        threads.emplace_back(worker_thread, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

## Compilation

```bash
# Basic compilation
g++ -std=c++11 your_program.cpp -o your_program

# With optimization
g++ -std=c++11 -O2 your_program.cpp -o your_program

# Run with required privileges
sudo ./your_program
```

## License

MIT License

Copyright (c) 2026

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Contributing

Contributions are welcome! This project follows a permissive contribution model:

- Feel free to fork, modify, and submit pull requests
- All contributions will be manually reviewed before merging
- Please ensure your code follows the existing style and structure
- Include clear descriptions of changes and their purpose
- Test your changes thoroughly before submitting

For questions or discussions about contributions, please reach out via email.

## Contact

For questions, bug reports, or collaboration inquiries:

**Email:** ujezeorah@gmail.com
