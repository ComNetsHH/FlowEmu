docker build -t channel_emulator .
docker run -d -it --name channel_emulator channel_emulator

pid=$(docker inspect -f '{{.State.Pid}}' channel_emulator)

sudo mkdir -p /var/run/netns/
sudo ln -sfT /proc/$pid/ns/net /var/run/netns/channel_emulator

sudo ip netns add source
sudo ip link add emulator netns source type veth peer name in netns channel_emulator
sudo ip netns exec source ethtool -K emulator rx off tx off
sudo ip netns exec source ip addr add 10.0.1.1/24 dev emulator
sudo ip netns exec channel_emulator ip link set in up
sudo ip netns exec source ip link set lo up
sudo ip netns exec source ip link set emulator up
sudo ip netns exec source ip route add 10.0.2.0/24 dev emulator

sudo ip netns add sink
sudo ip link add emulator netns sink type veth peer name out netns channel_emulator
sudo ip netns exec sink ethtool -K emulator rx off tx off
sudo ip netns exec sink ip addr add 10.0.2.1/24 dev emulator
sudo ip netns exec channel_emulator ip link set out up
sudo ip netns exec sink ip link set lo up
sudo ip netns exec sink ip link set emulator up
sudo ip netns exec sink ip route add 10.0.1.0/24 dev emulator

docker exec -it channel_emulator channel_emulator

docker stop channel_emulator; true
docker rm channel_emulator; true
sudo ip netns delete channel_emulator; true
sudo ip netns delete source; true
sudo ip netns delete sink; true
