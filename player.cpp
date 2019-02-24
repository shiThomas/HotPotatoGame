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
int server_fd, player_fd, left_fd, right_fd;
int valread;
int len;
int player_id;
int left_player_id, right_player_id;
int num_hops;
int num_players;
int right_port;
char right_hostname[64];
int port_num_master, port_num_player;
struct sockaddr_in newplayer, master_sock, right_sock;
char Potato_str[40960], player_hostname[64];
char buffer[40960], temp[40960];
char small_msg[64];
struct hostent *master_host_addr, *player_host_addr, *right_host_addr;
struct sockaddr_in left_player, right_player;
struct sockaddr_in server_in, player_in;
socklen_t newplayer_addr_len = sizeof(player_in);
const char * port;
using namespace std;
//potato * plyr = NULL;
int flag;
int fdmax;

int set_player(const char * host, int port_num_master) {
  int player_id;
  master_sock.sin_family = AF_INET;
  master_sock.sin_port = htons(port_num_master);
  memcpy(&master_sock.sin_addr, master_host_addr->h_addr_list[0], master_host_addr->h_length);

  player_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (player_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << host << "," << port << ")" << endl;
    exit(-1);
  }  //if

  cout << "Connecting to " << host << " on port " << port << "..." << endl;

  status = connect(player_fd, (struct sockaddr *)&master_sock, sizeof(master_sock));
  ;
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << host << "," << port << ")" << endl;
    exit(-1);
  }  //if

  //receive player info;
  len = recv(player_fd, buffer, strlen(buffer), 0);
  //decide not to pass potato
  //len = recv(player_fd, plyr, sizeof(struct potato_t), 0);
  buffer[len] = '\0';
  player_id = atoi(buffer);
  cout << "Player ID:" << player_id << endl;
  //send port info and hostname to master;
  sprintf(buffer, "%d %s", port_num_master, host);
  len = send(player_fd, buffer, strlen(buffer), 0);
  cout << "player info sent success" << endl;
  //  sprintf(buffer, "%s %d %s %s", "Player", player_id, "is", "ready");
  // len = send(player_fd, buffer, sizeof(buffer), 0);
  //cout << "Ready Sent Success" << endl;
  return player_id;
}
void set_right_player() {
  right_sock.sin_family = AF_INET;
  right_sock.sin_port = htons(right_port);
  right_host_addr = gethostbyname(right_hostname);
  if (right_host_addr == NULL) {
    fprintf(stderr, " host not found (%s)\n", right_hostname);
    exit(1);
  }
  memcpy(&right_sock.sin_addr, right_host_addr->h_addr_list[0], right_host_addr->h_length);

  right_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (right_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << right_hostname << "," << right_port << ")" << endl;
    exit(-1);
  }  //if

  cout << "Connecting to " << right_hostname << " on port " << right_port << "..." << endl;
}
void connect_neigh() {
  //right then left
  if (player_id == 0) {
    status = connect(right_fd, (struct sockaddr *)&right_sock, sizeof(right_sock));

    if (status == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      cerr << "  (" << right_hostname << "," << right_port << ")" << endl;
      exit(-1);
    }
    left_fd = accept(server_fd, (struct sockaddr *)&player_in, &newplayer_addr_len);
    if (status == -1) {
      cerr << "Error: cannot accept connection on socket" << endl;
      exit(-1);
    }
  }
  //left then right
  else {
  }
  left_fd = accept(server_fd, (struct sockaddr *)&player_in, &newplayer_addr_len);
  if (status == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    exit(-1);
  }
  status = connect(right_fd, (struct sockaddr *)&right_sock, sizeof(right_sock));

  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << right_hostname << "," << right_port << ")" << endl;
    exit(-1);
  }
}

//int get_hops(char buffer[40960]) {}

void play() {
  fd_set read_fds;  // temp file descriptor list for select()

  while (1) {
    FD_ZERO(&read_fds);  //clear temp sets
    FD_SET(player_fd, &read_fds);
    FD_SET(right_fd, &read_fds);
    FD_SET(left_fd, &read_fds);
    //get max value for fd;
    fdmax = player_fd;
    if (fdmax < right_fd) {
      fdmax = right_fd;
    }
    if (fdmax < left_fd) {
      fdmax = left_fd;
    }
    //wait to see where does potato come from
    len = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
    if (len == -1) {
      perror("select");
      exit(4);
    }
    //this case is from server for start;
    if (FD_ISSET(player_fd, &read_fds)) {
      len = recv(player_fd, buffer, 40960, 0);
      buffer[len] = '\0';
      //close signal

      if (!strcmp("exit", buffer)) {
        return;
      }
      else {
        // num_hops = get_hops(buffer);
        strcpy(temp, buffer);
        char * temp_ptr;
        //input: Start! Available Hops #<num_hops>
        temp_ptr = strtok(temp, "#");
        //this part is <num_hops>
        temp_ptr = strtok(NULL, " ");
        //store this value
        num_hops = atoi(temp_ptr);
      }
    }
    //this case is from right player;
    else if (FD_ISSET(right_fd, &read_fds)) {
      len = recv(player_fd, buffer, 40960, 0);
      buffer[len] = '\0';
      strcpy(temp, buffer);
      char * temp_ptr;
      //input: Start! Available Hops #<num_hops>
      temp_ptr = strtok(temp, "#");
      //this part is <num_hops>
      temp_ptr = strtok(NULL, " ");
      //store this value
      num_hops = atoi(temp_ptr);
    }
    //this case is from left player;
    else if (FD_ISSET(left_fd, &read_fds)) {
      len = recv(player_fd, buffer, 40960, 0);
      buffer[len] = '\0';
      strcpy(temp, buffer);
      char * temp_ptr;
      //input: Start! Available Hops #<num_hops>
      temp_ptr = strtok(temp, "#");
      //this part is <num_hops>
      temp_ptr = strtok(NULL, " ");
      //store this value
      num_hops = atoi(temp_ptr);
    }
    //game time
    if (num_hops == 1) {
      //num_hops--=0
      cout << "I'm it!" << endl;
      sprintf(buffer, "%s %d", "Player_ID:", player_id);
      len = send(player_fd, buffer, strlen(buffer), 0);
    }
    //time for left and right pass
    else {
      num_hops--;
      //right
      if (rand() % 2 == 0) {
        right_player_id = (player_id + 1) % num_players;
        sprintf(Potato_str,
                "%s %s %s %s %s%d %s%d",
                "Pass",
                "potato",
                "to",
                "Player",
                "Number",
                right_player_id,
                "Num_hops#",
                num_hops);
        len = send(right_fd, Potato_str, strlen(Potato_str), 0);
      }
      //left
      else {
        left_player_id = (player_id - 1 + num_players) % num_players;
        sprintf(Potato_str,
                "%s %s %s %s %s%d %s%d",
                "Pass",
                "potato",
                "to",
                "Player",
                "Number",
                left_player_id,
                "Num_hops#",
                num_hops);
        len = send(left_fd, Potato_str, strlen(Potato_str), 0);
      }
    }
  }
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
  // plyr = (potato *)malloc(sizeof(struct potato_t));
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
  //acquire player_id here;
  player_id = set_player(host, port_num_master);
  //server set up
  //get neighbour info

  gethostname(player_hostname, sizeof(player_hostname));
  player_host_addr = gethostbyname(player_hostname);
  if (player_host_addr == NULL) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << host << "," << port << ")" << endl;
    exit(-1);
  }  //if

  port_num_player = 1000 + (rand() % 10000);
  server_in.sin_family = AF_INET;
  flag = 0;
  while (flag != 1) {
    server_in.sin_port = htons(port_num_player);
    memcpy(&server_in.sin_addr, player_host_addr->h_addr_list[0], player_host_addr->h_length);
    status = bind(server_fd, (struct sockaddr *)&server_in, sizeof(server_in));
    if (status < 0) {
      port_num_player++;
    }
    else {
      flag = 1;
    }
  }
  status = listen(server_fd, 5);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << host << "," << port << ")" << endl;
    exit(-1);
  }  //if

  cout << "Waiting for connection on port " << port_num_player << endl;
  //neighbour info including id, host and port
  len = recv(player_fd, buffer, strlen(buffer), 0);
  buffer[len] = '\0';
  //parse info
  strcpy(temp, buffer);
  char * temp_ptr;
  temp_ptr = strtok(temp, " ");
  //num_players declared here
  num_players = atoi(temp_ptr);
  temp_ptr = strtok(NULL, " ");
  //right_id here
  right_player_id = atoi(temp_ptr);
  temp_ptr = strtok(NULL, " ");
  //right_host here
  strcpy(right_hostname, temp_ptr);
  temp_ptr = strtok(NULL, " ");
  //right port here
  right_port = atoi(temp_ptr);
  //connect to neighbour
  set_right_player();
  connect_neigh();
  //send ready
  sprintf(buffer, "%s %d %s %s %s %s", "Player", player_id, "is", "ready", "to", "play!");
  len = send(player_fd, buffer, strlen(buffer), 0);

  play();
  sleep(3);
  close(player_fd);
  close(left_fd);
  return 0;
}
