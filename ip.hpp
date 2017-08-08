#include <string>
#include <bitset>
#include <iostream>

using std::string;
struct IP {
  string add_string;
  long long add_long;

  IP () {
    add_long = 0;
    add_string = "0.0.0.0";
  }

  IP (long long add) {
    add_long = add;
    add_string = get_string(add);
  }
  IP (string add) {
    add_string = add;
    add_long = parse(add);
  }

  private:
  string get_string(long long add){
    long long A = add >> 24;  // A
    long long temp = add - (A << 24);  // A.B.C.D - A.0.0.0 = B.C.D
    long long B = temp >> 16;
    temp = temp - (B << 16); // B.C.D - B.0.0 = C.D
    long long C = temp >> 8;
    long long D = temp - (C << 8);

    string ret = std::to_string(A) + '.' + std::to_string(B) + "." + std::to_string(C) + "." + std::to_string(D);
    return ret;
  }

  private:
  long long parse(string add) {

    int a = add.find(".");
    int b = add.find(".", a+1);
    int c = add.find(".", b+1);
    long long A = stoll(add.substr(0, a)) << 24;
    long long B = stoll(add.substr(a+1, b-a-1)) << 16;
    long long C = stoll(add.substr(b+1, c-b-1)) << 8;
    long long D = stoll(add.substr(c+1, add.size()-c-1));

    // std::cout << "A: " << A << std::endl;
    // std::cout << "B: " << B << std::endl;
    // std::cout << "C: " << C << std::endl;
    // std::cout << "D: " << D << std::endl;

    long long ret = A+B+C+D;
    return ret;
  }

};

struct SUBNET {
  string subnet;
  IP ip;
  int mask;

  SUBNET(string subnet) {
    // 0123/34;
    std::size_t s = subnet.find("/");
    ip = IP(subnet.substr(0, s));
    mask = stoi(subnet.substr(s+1, subnet.size() - s - 1 ));
    subnet = subnet;
  }

  bool ipBelongs(IP inip) {
    string valid = std::bitset<32>(ip.add_long >> (32 - mask)).to_string();
    string candidate = std::bitset<32>(inip.add_long >> (32 - mask)).to_string();
    return valid.compare(candidate) == 0;
  }

  bool notResevered(IP inip) {
    // true if not first or last
    long long firstIP = (ip.add_long >> (32 - mask) ) << (32-mask);
    long long lastIP = firstIP + (1 << (32 - mask)) - 1;
    return firstIP != inip.add_long && lastIP != inip.add_long;
  }

  long long getFirstValid() {
    // std::cout << "************************\n";
    // std::cout << ip.add_long << std::endl;
    long long firstIP = (ip.add_long >> (32 - mask) ) << (32-mask);
    // std::cout << "First Valid is: " << firstIP << std::endl;
    // std::cout << "************************";
    return firstIP;
  }


};
