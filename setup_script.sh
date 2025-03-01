# This script is used to set up the server on which the system is to run
#!/bin/bash

dpdk_version=23.11.2
default_vpp_version="jammy main"

# function to test that commands execute successfully
error_check(){

    # test whether the last command executed with an error
    if [ $? -ne 0 ]; then
        # echo out the error message
        echo $1
        exit 1
    fi

}

echo "Updating package list..."
# Update package list
sudo apt update -y
error_check "Error Updating Package List..."

echo "Upgrading package list..."
# Upgrade package list - Use dist-upgrade to enact intelligent upgrades
sudo apt dist-upgrade -y
error_check "Error Upgrading Package List..."

# we do not test if packages are installed since the script would just leave the installed version of the package as is if it is the latest version

echo "Checking for build-essentials..."
# install build-essentials, the -y flag is added so yes is automatically selected whenever the user is prompted for a response
sudo apt install -y build-essential
error_check "Error Installing build-essential..."

echo "Checking for libtool..."
# install libtool
sudo apt install -y libtool-bin
error_check "Error Installing libtool..."

echo "Checking for automake..."
# install automake
sudo apt install -y automake
error_check "Error Installing automake..."

echo "Checking for autoconf..."
# install autoconf
sudo apt install -y autoconf
error_check "Error Installing autoconf..."

echo "Checking for perl..."
# install perl
sudo apt install -y perl
error_check "Error Installing perl..."

echo "Installing Linux Headers..."
# install headers
sudo apt install -y linux-headers-$(uname -r)
error_check "Error Installing Linux Headers..."

echo "Checking for sed..."
# install sed
sudo apt install -y sed
error_check "Error Installing sed..."

echo "Checking for gawk..."
# install gawk
sudo apt install -y gawk
error_check "Error Installing gawk..."

echo "Checking for binutils..."
# install binutils
sudo apt install -y binutils
error_check "Error Installing binutils..."

echo "Checking for gettext..."
# install gettext
sudo apt install -y gettext
error_check "Error Installing gettext..."

echo "Checking for glibc-common..."
# install glibc-common
sudo apt install -y locales
error_check "Error Installing glibc-common..."

echo "Checking for make..."
# install make
sudo apt install -y make
error_check "Error Installing make..."

echo "Checking for glibc..."
# install glibc
sudo apt install -y libc6
error_check "Error Installing glibc..."

echo "Checking for glibc-dev..."
# install glibc-dev
sudo apt install -y libc6-dev
error_check "Error Installing glibc-dev..."

echo "Checking for python3..."
# install python3
sudo apt install -y python3
error_check "Error Installing python3..."

echo "Checking for python3-pyelftools..."
# install python3-pyelftools
sudo apt install -y python3-pyelftools
error_check "Error Installing python3-pyelftools..."

echo "Checking for python3-dev..."
# install python3-dev
sudo apt install -y python3-dev
error_check "Error Installing python3-dev..."

echo "Checking for libnuma-dev..."
# install libnuma-dev
sudo apt install -y libnuma-dev
error_check "Error Installing libnuma-dev..."

echo "Checking for meson..."
# install meson
sudo apt install -y meson
error_check "Error Installing meson..."

echo "Checking for ninja-build..."
# install ninja-build
sudo apt install -y ninja-build
error_check "Error Installing ninja-build..."

echo "Checking for CURL..."
# Install CURL
sudo apt install -y curl
error_check "Error Installing CURL..."

echo "Checking for Libcurl..."
# Install Libcurl
sudo apt install -y libcurl4-openssl-dev
error_check "Error Installing Libcurl..."

echo "Checking for Libssl-dev..."
# Install Libssl-dev
sudo apt install -y libssl-dev
error_check "Error Installing Libssl-dev..."

echo "Checking for Libfmt-dev..."
# Install Libfmt-dev
sudo apt install -y libfmt-dev
error_check "Error Installing Libfmt-dev..."

echo "Checking for tar..."
# Install tar
sudo apt install -y tar
error_check "Error Installing tar..."

echo "Checking for wget..."
# Install wget
sudo apt install -y wget
error_check "Error Installing wget..."

echo "Checking for Libarchive..."
# Install Libarchive. Libarchive is used by some dpdk development tools
sudo apt install -y libarchive-dev
error_check "Error Installing Libarchive..."

echo "Checking for Libelf..."
# Install Libelf
sudo apt install -y libelf1
error_check "Error Installing Libelf..."

echo "Checking for Libelf-dev..."
# Install Libelf. Libelf is used by some dpdk development tools
sudo apt install -y libelf-dev
error_check "Error Installing Libelf-dev..."

echo "Checking for Libcap-dev..."
# Install Libcap. Libcap is used by some dpdk development tools
sudo apt install -y libcap-dev
error_check "Error Installing Libcap-dev..."

echo "Checking for Libpcap..."
# Install Libcap
sudo apt install -y libpcap0.8
error_check "Error Installing Libpcap..."

echo "Checking for Libpcap-dev..."
# Install Libcap
sudo apt install -y libpcap-dev
error_check "Error Installing Libpcap-dev..."

echo "Checking for Libxdp-dev..."
# Install Libxdp. Libxdp is used by some dpdk development tools
sudo apt install -y libxdp-dev
error_check "Error Installing Libxdp..."

echo "Checking for Libbpf..."
# Install Libbpf. Libbpf is used by some dpdk development tools
sudo apt install -y libbpf-dev
error_check "Error Installing Libbpf..."

echo "Checking Downloading openonload..."
# download openonload
wget "https://www.xilinx.com/content/dam/xilinx/publications/solarflare/onload/openonload/9_0_1_86/SF-109585-LS-46-OpenOnload-Release-Package.zip"
error_check "Error Downloading openonload..."

echo "Untarring Openonload Archive..."
# Untar Openonload
tar -xvf SF-109585-LS-46-OpenOnload-Release-Package.zip
error_check "Error Untarring Openonload Archive..."

echo "Entering Openonload Directory..."
cd SF-109585-LS-46-OpenOnload-Release-Package
error_check "Error Entering Openonload Directory..."

echo "Untarring Openonload Base Archive..."
# Untar Openonload Base Archive
tar -xvf onload-9.0.1.86.tgz
error_check "Error Untarring Openonload Base Archive..."

echo "Entering Openonload scripts Directory..."
cd onload-9.0.1.86/scripts
error_check "Error Entering Openonload Base Directory..."

echo "Installing Openonload"
# install openonload with no-sfc flag
./onload_install --no-sfc

# echo "Downloading DPDK..."
# # Download DPDK
# wget "https://fast.dpdk.org/rel/dpdk-$dpdk_version.tar.xz"
# error_check "Error Downloading DPDK..."

# echo "Untarring DPDK Archive..."
# # Untar DPDK
# tar -xvf dpdk-$dpdk_version.tar.xz
# error_check "Error Untarring DPDK Archive..."

# echo "Changing To DPDK Directory..."
# # Change directory to the DPDK directory
# # We first test whether the DPDK downloaded is an ordinary version or a stable version, a stable version's untarred folder name would begin with "dpdk-stable-"
# if compgen -d "dpdk-stable"* > /dev/null; then
#     cd "dpdk-stable-$dpdk_version" || { echo "Error Changing To Directory Matching 'dpdk-stable-*' "; exit 1; }
# elif compgen -d "dpdk-"* > /dev/null; then
#     cd "dpdk-$dpdk_version" || { echo "Error Changing To Directory Matching 'dpdk-*' "; exit 1; }
# else
#     echo "No DPDK directory found."
#     exit 1
# fi

# echo "Building DPDK..."
# # Building DPDK
# meson setup build
# error_check "Error Building DPDK..."

# echo "Building DPDK Drivers and Libraries..."
# ninja -C build
# error_check "Error Building DPDK Drivers and Libraries..."

# echo "Installing DPDK Drivers and Libraries..."
# sudo ninja -C build install
# error_check "Error Installing DPDK Drivers and Libraries..."

# echo "Creating Directory /dev/hugepages..."
# mkdir -p /dev/hugepages
# error_check "Error Creating Directory /dev/hugepages..."

# echo "Mounting Directory /dev/hugepages..."
# mountpoint -q /dev/hugepages || mount -t hugetlbfs nodev /dev/hugepages
# error_check "Error Mounting Directory /dev/hugepages..."

# echo "Reserving Huge Pages..."
# # Reserve Huge Pages
# echo 1024 | sudo tee /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
# error_check "Error Reserving Huge Pages..."

# echo "Printing DPDK Devbind Status..."
# # Print Out The Status of The System NICs
# ./usertools/dpdk-devbind.py --status
# error_check "Error Printing DPDK Devbind Status..."

# echo "Creating Directory /var/log/vpp..."
# # Create the /var/log/vpp directory for vpp, the -p flag creates the parent directory if necessary
# sudo mkdir -p /var/log/vpp
# error_check "Error Creating Directory /var/log/vpp..."

# # Running The VPP Installation Setup Script
# echo "Running The VPP Installation Setup Script..."
# curl -s https://packagecloud.io/install/repositories/fdio/release/script.deb.sh | sudo bash
# error_check "Error Running The VPP Installation Setup Script..."

# # Update the apt source list
# sudo apt update -y

# # Installing VPP
# echo "Installing VPP..."
# sudo apt install -y vpp

# # the most recent version of vpp available does not always correspond to the most recent ubuntu version so to avoid any errors of missing packages when this is run we manually update the sources folder if the sudo apt install vpp returns with an error
# if [ $? -ne 0 ]; then
#     # Check for sed utility
#     echo "Checking for Sed..."
#     sudo apt install -y sed
#     error_check "Error Installing Sed..."

#     # update the package list with the default VPP version
#     sudo sed -i "s/\(.*\) main/$default_vpp_version/g" /etc/apt/sources.list.d/fdio_release.list
# fi

# # Update the apt source list
# sudo apt update -y

# # If installing VPP fails again this time we just print the error and exit
# echo "Retrying Installing VPP..."
# sudo apt install -y vpp
# error_check "Error Installing VPP..."

# # Installing VPP-Plugin-Core
# echo "Installing VPP-Plugin-Core..."
# sudo apt install -y vpp-plugin-core
# error_check "Error Installing VPP-Plugin-Core..."

# # Installing VPP-Plugin-DPDK
# echo "Installing VPP-Plugin-DPDK..."
# sudo apt install -y vpp-plugin-dpdk
# error_check "Error Installing VPP-Plugin-DPDK..."

# # Installing Python3-VPP-API
# echo "Installing Python3-VPP-API..."
# sudo apt install -y python3-vpp-api
# error_check "Error Installing Python3-VPP-API..."

# # Installing VPP-Dbg
# echo "Installing VPP-Dbg..."
# sudo apt install -y vpp-dbg
# error_check "Error Installing VPP-Dbg..."

# # Installing VPP-Dev
# echo "Installing VPP-Dev..."
# sudo apt install -y vpp-dev
# error_check "Error Installing VPP-Dev..."

