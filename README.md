# Playing with container network interfaces (CNI)

## Why (am I playing with CNI)?

I had to set up Kubernetes clusters on AWS recently. This naturally involved dealing with networking. I ran into a few networking issues. I started digging into how Kubernetes and Docker handle networking. In the process I learned somethings about how Linux handles IP routing and various command line tools which help in configuring routing at the Linux host level.

Container network interface ([CNI](https://github.com/containernetworking/cni)) is one of the things I learned about. CNI is a Linux foundations project attempting to standardise how container provisioning applications (like Kubernetes) can interact with networking pluggin (Weave, Calico etc). Kubernetes can request networking plugin to create or delete a network interface for a container through this interface. So as long as a networking plugin conforms to CNI, it can be used with K8s.

Kubenet is one such network plugin which is included in the K8s repository. It requires three binaries to work though:
  - loopback: creates loopback interface for container
  - host-local: Handles IP address management (IPAM) on a single host.
  - bridge: Creates a bridge, and a veth pair. One of the peer is added to container network space, the other is enslaved to the bridge.

This repository is an attempt to have VERY simple implementations of these three binaries.

## Some theory

Network namespaces is the Linux feature which allows isolated looking network stacks for containers running on the same host. Linux host has a default network stack. You can create multiple network spaces on a Linux host. A process can be started on one of these network namespaces. Processes running on separate namespaces can only connect to each other if their is some sort of bridge between their respective namespaces.

When a docker container is started (with default network), docker creates a network namespace for that container. It also creates a veth pair and places one of the veth peer in the new namespace. The other peer is enslaved to a bridge on the default namespace (docker0 by default). You can do all this process yourself too if you so desire. Try out the commands [here](/manual.md).

Lets, talk about Kubernetes networking. For simplicity I will talk about the case of Kubenet plugin. Kubernetes assigns each worker node (host) a pod CIDR. When a pod has to be started (in default networking mode), Kubernetes chooses a node. Kubelet daemon on that node starts a docker container (using the pause image) with `network=none` setting. Also Kubelet instructs Kubenet plugin to set up networking stack for this container. Kubenet plugin will basically do the steps outlined earlier above. Specifically, it assigns an IP from the pod-CIDR to the pod and sets up veth pairs. The pause container set up does not do anything from this point forward. It just sits around. All the containers of the pod are now started with network set as that of the pause network. This ensures that all the containers in the pod are in same namespace and can reach out to each other on the localhost IP. The pause container is a very light weight container and enusres the network stack remains up even if one of the pod container dies.

## Try out the code in this repository

You can try out setting up a docker container with self defined network settings.
Ensure that you have docker, iptables and brctl installed on your machine.

```
// Build the binary binary
g++ bridge.cpp -o bin/bridge -std=c++11
// Create a container
sudo scripts/start.sh
```

This will print out the IP address of the newly created docker container.
You should be able to ping to that address. Try creating a few containers. You can experiment and see if you can reach containers from other containers.
