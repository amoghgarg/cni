## Trying out network namespace with a docker container

The following steps will show how to set up a docker container with `--network=none` and then add interfaces and routing rules to connect with that container.

```
// Run a containers
sudo docker run -d --name=test --network=none busybox sleep 1000000
pid=$(sudo docker inspect -f '{{ .State.Pid }}' test)

// Check that it only has a loop back interface inside the container, which is created automatically.
sudo docker exec test ip link
```

After setting up the container, we create a bridge `bri0` and a pair of veth interfaces.
The bridge and interface named `v_out` will stay in the default network namespaces of the Linux host. The veth interface `v_in` will be put into the container network namespace.
```
sudo brctl addbr bri0
sudo ip link add dev v_out type veth peer name v_in
sudo brctl addif bri0 v_out
// Shift the v_in to the container network namespace (netns)
sudo ip link set v_in netns $pid
```

Now the interface v_in will be given the address `172.20.0.2` and the bridge will be given address `172.20.0.1`.
```
// Set up addresses
sudo ip addr add 172.20.0.1/16 dev bri0
sudo nsenter -t $pid -n ip addr add 172.20.0.2/16 dev v_in
// Set the bridge and interfaces up
sudo ip link set bri0 up
sudo ip link set v_out up
sudo nsenter -t $pid -n ip link set v_in up
```

Following routing commands will let Linux route traffic to and from the bridge.
```
sudo ip route add 172.20.0.0/16 dev bri0 src 172.20.0.1
sudo nsenter -t $pid -n ip route add 172.20.0.0/16 dev v_in src 172.20.0.2
sudo nsenter -t $pid -n ip route add default via 172.20.0.1
```

You should be able to ping to the container now:
```
$ ping 172.20.0.2
```

If you want to ping to other containers on the host from inside this container, you will need to enable IP forwarding in ip tables:
```
sudo iptables -A FORWARD -i bri0 -j ACCEPT
sudo iptables -A FORWARD -o bri0 -j ACCEPT
```

If you want to ping to addresses outside your machine, you will need to enable MASQUERADE on NAT table:
```
sudo iptables -t nat -A POSTROUTING --source 172.20.0.0/16 -j MASQUERADE
```

You should be able to ping to internet from inside the container now.
