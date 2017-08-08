#include "host-local.h"
#include "json.hpp"
#include "util.h"

#include <string>
#include <unordered_map>
#include <fstream>
#include <functional>

using std::string;
using nlohmann::json;
using std::sprintf;

const string VERSION="0.3.1";

void createBridge(string name, string subnet){
  // Check if bridge exists:
  char check[200];
  sprintf(check, "ip link show %s", name.c_str());
  if (!run(check)) { // If exit code = 0 -> present
    return;
  }

  // Add a new bridge
  char command[200];
  sprintf(command, "brctl addbr %s", name.c_str());
  run(command);
  // Set address to the bridge
  char command1[200];
  sprintf(command1, "ip addr add 172.18.0.1/16 dev %s", name.c_str());
  run(command1);
  // Set the bridge up
  char setUp[200];
  sprintf(setUp, "ip link set %s up", name.c_str());
  run(setUp);
}

void configureVeth(string IP, string netns) {
  string pid = containerPid(netns);
  string o_veth = "veth_" + pid;
  string i_veth = "eth0";

  // Create veth
  char c[200];
  sprintf(c, "ip link add dev %s type veth peer name %s", o_veth.c_str(), i_veth.c_str());
  run(c);
  // Add to bridge
  char d[200];
  sprintf(d, "brctl addif cni0 %s ", o_veth.c_str());
  run(d);
  // Add to ns
  char e[200];
  sprintf(e, "ip link set %s netns %s", i_veth.c_str(), pid.c_str());
  run(e);
  // Add address
  char f[200];
  sprintf(f, "nsenter -t %s -n ip addr add %s dev %s", pid.c_str(),  IP.c_str(), i_veth.c_str());
  run(f);
  //Up the veths
  char g[200];
  sprintf(g, "ip link set %s up", o_veth.c_str());
  run(g);
  char h[200];
  sprintf(h, "nsenter -t %s -n ip link set %s up", pid.c_str(), i_veth.c_str());
  run(h);
  // Set up the route inside the ns
  char i[200];
  sprintf(i, "nsenter -t %s -n ip route add 172.18.0.0/16 dev %s src %s", pid.c_str(), i_veth.c_str(), IP.c_str());
  run(i);
  char j[200];
  sprintf(j, "nsenter -t %s -n ip route add default via 172.18.0.1", pid.c_str());
  run(j);

}

void add(string cni_containerID, string cni_netns, json * j){
  string bridgeName = j->at("bridge");
  std::unordered_map<std::string, json> ipam = j->at("ipam").get<std::unordered_map<std::string, json>>();
  string subnet = ipam.at("subnet");
  createBridge(bridgeName, subnet);

  // Run the loopback thing
  string IP =  add_hostlocal(cni_containerID, j);
  configureVeth(IP, cni_netns);

}

void del(string cni_containerID, string cni_netns, json * j){
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
      add(cni_containerid, cni_netns, &j);
    } else if (!cni_command.compare("DEL")) {
      del(cni_containerid, cni_netns, &j);
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
