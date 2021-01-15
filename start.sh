docker build -t channel_emulator .
docker run -d -it --net=none --name channel_emulator channel_emulator

pid=$(docker inspect -f '{{.State.Pid}}' channel_emulator)

sudo mkdir -p /var/run/netns/
sudo ln -sfT /proc/$pid/ns/net /var/run/netns/channel_emulator

sudo ip netns add source
sudo ip link add emulator_in type veth peer name in
sudo ip link set in netns channel_emulator
sudo ip link set emulator_in netns source
sudo ip netns exec channel_emulator ip link set in up
sudo ip netns exec source ip link set emulator_in up
sudo ip netns exec source ip addr add 10.0.0.1/24 dev emulator_in

sudo ip netns add sink
sudo ip link add emulator_out type veth peer name out
sudo ip link set out netns channel_emulator
sudo ip link set emulator_out netns sink
sudo ip netns exec channel_emulator ip link set out up
sudo ip netns exec sink ip link set emulator_out up
sudo ip netns exec sink ip addr add 10.0.0.2/24 dev emulator_out

docker exec -it channel_emulator channel_emulator

docker stop channel_emulator; true
docker rm channel_emulator; true
sudo ip netns delete source; true
sudo ip netns delete sink; true
