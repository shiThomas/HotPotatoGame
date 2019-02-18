#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "potato.h"

int status;
int master_fd, player_fd;
int valread;
int len;
int port_num_master, port_num_player;
struct sockaddr_in newplayer, master_sock;
char str[64];
char buffer[40960], temp[40960];
struct hostent * master_host_addr;
struct sockaddr_in left_player, right_player;
const char * port;
using namespace std;
potato * plyr = NULL;

void set_player(const char * host, int port_num_master) {
  master_sock.sin_family = AF_INET;
  master_sock.sin_port = htons(port_num_master);
  memcpy(&master_sock.sin_addr, master_host_addr->h_addr_list[0], master_host_addr->h_length);

  player_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (player_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << host << "," << port << ")" << endl;
    exit(-1);
  }  //if

  int yes = 1;
  status = setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(master_fd, (struct sockaddr *)&master_sock, sizeof(master_sock));
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << host << "," << port << ")" << endl;
    exit(-1);
  }  //if

  status = listen(master_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << host << "," << port << ")" << endl;
    exit(-1);
  }  //if

  cout << "Waiting for connection on port " << port << endl;
}
//connect to players and store information in potato
void connect_player(potato player) {
  player.playerfd = accept(master_fd, (struct sockaddr *)&newplayer, &newplayer_addr_len);
  if (player.playerfd == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    exit(-1);
  }
  player.host_addr = gethostbyaddr((char *)&newplayer.sin_addr, sizeof(struct in_addr), AF_INET);
}

int main(int argc, char * argv[]) {
  // struct addrinfo host_info;
  //struct addrinfo * host_info_list;
  const char * host = NULL;

  if (argc != 3) {
    cout << "Syntax:" << argv[0] << "<machine_name> <port_num>" << endl;
    return 1;
  }
  //create space for current player;
  plyr = (potato *)malloc(sizeof(struct potato_t));
  master_host_addr = gethostbyname(argv[1]);
  if (master_host_addr == NULL) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }
  //hostname
  host = argv[1];
  //two types of argv[2];
  port = argv[2];
  port_num_master = atoi(argv[2]);
  set_player(host, port_num_master);

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  socket_fd =
      socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  cout << "Connecting to " << hostname << " on port " << port << "..." << endl;

  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  const char * message = "hi there!";
  send(socket_fd, message, strlen(message), 0);
  // char buffer[512];
  // recv(socket_fd, buffer, 40, 0);
  // buffer[40] = 0;
  char buffer[1024] = {0};
  valread = read(socket_fd, buffer, 1024);
  cout << "Player received: " << buffer << endl;

  freeaddrinfo(host_info_list);
  close(socket_fd);

  return 0;
}
