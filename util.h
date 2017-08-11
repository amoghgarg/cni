#include <string>

using std::string;

string getEnv(const char* var) {
  char const* val = std::getenv(var);
  return val == NULL ? string() : string(val);
}

int run(const char * command) {
  string c = command;
  c = c + " > null";
  return std::system(c.c_str());
}

string containerPid(const string &netns){
  /*
  The netns is in the form `/proc/{pid}/ns/net`
  The characters between the second and third
  / (slash) are assumed to be the PID
  */
  std::size_t f = netns.find('/', 1);
  std::size_t s = netns.find('/', f+1);
  string pid = netns.substr(f+1, s - f - 1);
  return pid;
}
