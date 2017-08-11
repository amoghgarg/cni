#include "host-local.h"
#include "json.hpp"

#ifndef UTIL_I
#define UTIL_I
#include "util.h"
#endif

#include <string>
#include <unordered_map>
#include <fstream>
#include <functional>

using std::string;
using nlohmann::json;
using std::sprintf;

const string VERSION="0.3.1";

void createBridge(const string &name, const SUBNET &subnet){
  // Check if bridge exists:
  char command[200];
  sprintf(command, "ip link show %s", name.c_str());
  if (!run(command)) { // If exit code = 0 -> present
    return;
  }

  string gateway = IP(subnet.getGatewayAddress()).add_string;
  string address = gateway + "/" + std::to_string(subnet.mask);

  // Add a new bridge
  sprintf(command, "brctl addbr %s", name.c_str());
  run(command);
  // Set address to the bridge
  sprintf(command, "ip addr add %s dev %s", address.c_str(), name.c_str());
  run(command);
  // Set the bridge up
  sprintf(command, "ip link set %s up", name.c_str());
  run(command);
  // Set up iptables rules for the bridge
  sprintf(command, "iptables -A FORWARD -i %s -j ACCEPT", name.c_str());
  run(command);
  sprintf(command, "iptables -A FORWARD -o %s -j ACCEPT", name.c_str());
  run(command);
  sprintf(command, "iptables -t nat -A POSTROUTING --source %s -j MASQUERADE", subnet.subnet.c_str());
  run(command);

}

string configureVeth(const string &ip, const string &netns, const SUBNET &subnet) {
  string pid = containerPid(netns);
  string o_veth = "veth_" + pid;
  string i_veth = "eth0";
  char command[200];
  string gateway = IP(subnet.getGatewayAddress()).add_string;

  // Create veth
  sprintf(command, "ip link add dev %s type veth peer name %s", o_veth.c_str(), i_veth.c_str());
  run(command);
  // Add to bridge
  sprintf(command, "brctl addif cni0 %s ", o_veth.c_str());
  run(command);
  // Add to ns
  sprintf(command, "ip link set %s netns %s", i_veth.c_str(), pid.c_str());
  run(command);
  // Add address
  sprintf(command, "nsenter -t %s -n ip addr add %s dev %s", pid.c_str(),  ip.c_str(), i_veth.c_str());
  run(command);
  //Up the veths
  sprintf(command, "ip link set %s up", o_veth.c_str());
  run(command);
  sprintf(command, "nsenter -t %s -n ip link set %s up", pid.c_str(), i_veth.c_str());
  run(command);
  // Set up the route inside the ns
  sprintf(command, "nsenter -t %s -n ip route add %s dev %s src %s", pid.c_str(), subnet.subnet.c_str(), i_veth.c_str(), ip.c_str());
  run(command);
  sprintf(command, "nsenter -t %s -n ip route add default via %s", pid.c_str(), gateway.c_str());
  run(command);

  return i_veth;
}

json add(const string &cni_containerID, const string &cni_netns, const json &j){
  string bridgeName = j.at("bridge");
  std::unordered_map<std::string, json> ipam = j.at("ipam").get<std::unordered_map<std::string, json>>();
  string x = ipam.at("subnet");
  SUBNET subnet = SUBNET(x);
  createBridge(bridgeName, subnet);

  // Run the loopback thing
  json res =  add_hostlocal(cni_containerID, j);
  string IP = res.at("address_internal");
  string i_name = configureVeth(IP, cni_netns, subnet);

  res["interfaces"] = {
    {"name", i_name},
    {"sandbox", cni_netns}
  };

  return res;
}

void del(const string &cni_containerID, const string &cni_netns, const json &j){
  del_hostlocal(cni_containerID, j);

  // Delete the outer veth pair
  string pid = containerPid(cni_netns);
  string o_veth = "veth_" + pid;
  char command[200];
  sprintf(command, "ip link delete %s", o_veth.c_str());
  run(command);
}

int main() {
  string cni_version = getEnv("CNI_VERSION");
  string cni_command = getEnv("CNI_COMMAND");
  string cni_containerid = getEnv("CNI_CONTAINERID");
  string cni_netns = getEnv("CNI_NETNS");
  string cni_args = getEnv("CNI_ARGS");

  string in;
  getline(std::cin, in);

  try {
    json j = json::parse(in);
    if (!cni_command.compare("VERSION")) {
      json result = {
        {"cniVersion", VERSION},
        {"supportedVersions", {
          "0.1.1", "0.2.0", "0.2.3"
        }}
      };
      std::cout << result.dump(2) << std::endl;
    } else if (!cni_command.compare("ADD")) {
      json res = add(cni_containerid, cni_netns, j);
      std::cout << res.dump(2) << std::endl;
    } else if (!cni_command.compare("DEL")) {
      del(cni_containerid, cni_netns, j);
    } else {
      throw std::invalid_argument("Env variable CNI_VERSION should be set from: VERSION, ADD, DEL.");
    }
    return 0;
  }
  catch(const std::exception &e){
    json result = {
      {"cniVersion", VERSION},
      {"code", 89},
      {"msg", e.what()}
    };
    std::cout << result.dump(2) << std::endl;
    return 1;
  }

}
