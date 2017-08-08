#include "ip.hpp"
#include "json.hpp"
#include <string>
#include <unordered_map>
#include <fstream>
#include <functional>

using std::string;
using nlohmann::json;

/* Returns the maximum IP found in the file
in long long form.
However, if the file is empty, will return -1.
*/
long long getMaxIP(std::fstream& inFile) {
  string ip;
  string id;
  long long max = -1LL;
  while (inFile >> ip >> id){
    IP temp = IP(ip);
    max = max > temp.add_long ? max : temp.add_long;
  }
  return max;
}

void del_hostlocal(string cni_containerid, json * j){
  std::fstream fs;
  string ip;
  string id;

  fs.open ("test.txt", std::fstream::in | std::fstream::out);
  std::vector<string> toWrite;
  while (fs >> ip >> id){
    if (id.compare(cni_containerid)) {
      toWrite.push_back(ip + " " + id);
    }
  }
  fs.close();
  fs.open("test.txt", std::fstream::out | std::fstream::trunc );
  for (std::vector<string>::iterator it = toWrite.begin(); it != toWrite.end(); it++){
    fs << *it << std::endl;
  }
  fs.close();
}


string add_hostlocal(string cni_containerID, json * j){
  // Parse the subnet mask from the request
  std::unordered_map<std::string, json> ipam = j->at("ipam").get<std::unordered_map<std::string, json>>();
  string x = ipam.at("subnet");
  string gateway("");
  SUBNET subnet = SUBNET(x);

  // Get the next valid IP from the file
  std::fstream fs;
  fs.open ("test.txt", std::fstream::in | std::fstream::out);
  long long maxIP = getMaxIP(fs);
  if (maxIP == -1LL) {
    maxIP = subnet.getFirstValid();
  }
  IP candidate = IP(maxIP+1LL);
  if (subnet.ipBelongs(candidate) & subnet.notResevered(candidate)) {
    // Write to file
    string add = candidate.add_string + " " + cni_containerID;
    fs.clear();
    fs << add << std::endl;
    // Write json to stdout
    json result = {
      {"cniVersion", "0.3.1"},
      {"ips:", {
        {
          {"version", "4"},
          {"address", candidate.add_string + "/" + std::to_string(subnet.mask)},
          {"gateway", gateway}
        }
      }},
      {"routes", {
        {
          {"dst", "0.0.0.0/0"}
        }
      }},
      {"dns",json::object()}
    };
    return candidate.add_string;
  } else {
    fs.close();
    throw std::logic_error( "Not enough IPs remaining." );
  }
fs.close();
}
