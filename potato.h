#include <iostream>
#include <vector>
using namespace std;
struct potato_t {
  int hops_total;
  int player_num;
  int player_id;
  int player_port;
  //may not be used hostname
  char player_hostname[64];
  struct hostent * host_addr;
  int playerfd;
  //string player_list;
};
typedef struct potato_t potato;

void initialize_potato(potato * p) {
  p->hops_total = 0;

  p->player_id = -1;
  //p->player_list = "";
}
