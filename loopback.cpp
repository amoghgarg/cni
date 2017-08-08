#include <iostream>
#include <cstdlib>
#include <string>
#include <cstdio>
#include "util.h"

using std::string;

int runCommand(string netCommand, string pid) {
  char command[200];
  sprintf(command, "nsenter -t %s -n %s", pid.c_str(), netCommand.c_str());
  return system(command);
}

void add(string cni_netns){
  string pid = containerPid(cni_netns);
  // Up the link
  string upCommand("ifconfig lo 127.0.0.1 netmask 255.0.0.0 up");
  runCommand(upCommand, pid);
}

void del(string cni_netns) {
  string pid = containerPid(cni_netns);
  // Down the link
  string upCommand("ifconfig lo 127.0.0.1 netmask 255.0.0.0 down");
  runCommand(upCommand, pid);
}

int main() {

  // Read arguments from environment variables
  string cni_version = getEnv("CNI_VERSION");
  string cni_command = getEnv("CNI_COMMAND");
  string cni_containerid = getEnv("CNI_CONTAINERID");
  string cni_netns = getEnv("CNI_NETNS");
  string cni_args = getEnv("CNI_ARGS");

  if (!cni_command.compare("VERSION")) {
    std::cout << "{\"cniVersion\":\"0.3.1\",\"supportedVersions\":[\"0.1.0\",\"0.2.0\",\"0.3.0\",\"0.3.1\"]}";
  } else if (!cni_command.compare("ADD")) {
    add(cni_netns);
  } else if (!cni_command.compare("DEL")) {
    del(cni_netns);
  } else {
    std::cout << "Env variable CNI_VERSION should be set from: VERSION, ADD, DEL.";
  }
}
