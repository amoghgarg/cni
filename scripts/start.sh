#!/bin/sh

contid=$(docker run -d --net=none busybox:latest /bin/sleep 10000000)
pid=$(docker inspect -f '{{ .State.Pid }}' $contid)
netnspath=/proc/$pid/ns/net

echo "Container created with id: $(echo $contid | cut -c1-10)"
echo ""

echo '{"cniVersion": "0.2.0","name": "mynet","type": "bridge","bridge": "cni0","isGateway": true,"ipMasq": true,"ipam":{"type": "host-local","subnet": "172.18.0.0/16","routes": [{ "dst": "0.0.0.0/0" }]}}' \
  | sudo CNI_COMMAND=ADD CNI_NETNS=$netnspath CNI_CONTAINERID=$contid ./bin/bridge


echo "-----------------------------------------------------------------"
echo "You can try pinging the container on the address mentioned above."
echo "You can stop the container with this command:"
echo "    sudo scripts/stop.sh $(echo $contid | cut -c1-10)"
