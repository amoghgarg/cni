#include "json.hpp"
#include <string>
#include <unordered_map>
#include <fstream>
#include <functional>
#include "host-local.h"
#ifndef UTIL_I
#define UTIL_I
#include "util.h"
#endif


using std::string;
using nlohmann::json;

// const string DATA_DIR="/tmp/host-local";
const string VERSION="0.3.1";

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
      add_hostlocal(cni_containerid, j);
    } else if (!cni_command.compare("DEL")) {
      del_hostlocal(cni_containerid, j);
    } else {
      throw std::invalid_argument("Env variable CNI_VERSION should be set from: VERSION, ADD, DEL.");
    }
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
