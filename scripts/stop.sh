#!/bin/sh

contid=$(docker inspect -f '{{ .Id }}' $1)
pid=$(docker inspect -f '{{ .State.Pid }}' $1)
netnspath=/proc/$pid/ns/net

echo docker container: $contid
echo pid: $pid
echo netnspath: $netnspath

echo '{"cniVersion": "0.2.0","name": "mynet","type": "bridge","bridge": "cni0","isGateway": true,"ipMasq": true,"ipam":{"type": "host-local","subnet": "172.18.0.0/16","routes": [{ "dst": "0.0.0.0/0" }]}}' \
  | sudo CNI_COMMAND=DEL CNI_NETNS=$netnspath CNI_CONTAINERID=$contid ./a.out

docker stop $contid
