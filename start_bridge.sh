cd BUILD
make -j$(nproc)
cd ..

sudo ip link set eno1 promisc on
sudo ethtool -K eno1 tcp-segmentation-offload off generic-segmentation-offload off generic-receive-offload off

sudo setcap cap_net_raw=eip ./BUILD/bin/channel_emulator

./BUILD/bin/channel_emulator
