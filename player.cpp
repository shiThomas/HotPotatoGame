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
#include <time.h>
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
  char plyr_id[64];
  len = recv(player_fd, plyr_id, sizeof(plyr_id), 0);
  //decide not to pass potato
  //len = recv(player_fd, plyr, sizeof(struct potato_t), 0);
  plyr_id[len] = '\0';
  player_id = atoi(plyr_id);
  cout << "Player ID:" << player_id << endl;
  //send port info and hostname to master;

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
    //cout << "player 0" << endl;
    status = connect(right_fd, (struct sockaddr *)&right_sock, sizeof(right_sock));
    cout << "connect success" << endl;
    cout << "right port" << right_port << endl;
    if (status == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      cerr << "  (" << right_hostname << "," << right_port << ")" << endl;
      exit(-1);
    }
    left_fd = accept(server_fd, (struct sockaddr *)&player_in, &newplayer_addr_len);
    cout << "accept success" << endl;
    if (status == -1) {
      cerr << "Error: cannot accept connection on socket" << endl;
      exit(-1);
    }
  }
  //left then right
  else {
    //cout << "player not 0" << endl;
    left_fd = accept(server_fd, (struct sockaddr *)&player_in, &newplayer_addr_len);
    cout << "accept success" << endl;
    if (status == -1) {
      cerr << "Error: cannot accept connection on socket" << endl;
      exit(-1);
    }
    status = connect(right_fd, (struct sockaddr *)&right_sock, sizeof(right_sock));
    cout << "connect success" << endl;
    cout << "right port" << right_port << endl;
    if (status == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      cerr << "  (" << right_hostname << "," << right_port << ")" << endl;
      exit(-1);
    }
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
    /////////
    right_player_id = (player_id + 1) % num_players;
    left_player_id = (player_id - 1 + num_players) % num_players;
    //    cout << "right " << right_player_id << endl;
    //cout << "left " << left_player_id << endl;
    if (FD_ISSET(player_fd, &read_fds)) {
      char game[512];
      len = recv(player_fd, game, sizeof(game), 0);
      game[len] = '\0';
      cout << "receive from master " << game << endl;
      //close signal

      if (!strcmp("exit", game)) {
        return;
      }
      else {
        // num_hops = get_hops(buffer);
        strcpy(temp, game);
        char * temp_ptr;
        //input: Start! Available Hops #<num_hops>
        temp_ptr = strtok(temp, "#");
        //this part is <num_hops>
        temp_ptr = strtok(NULL, " ");
        //store this value
        num_hops = atoi(temp_ptr);
        cout << "testing from master number of hops: " << num_hops << endl;
      }
    }
    //this case is from right player;
    else if (FD_ISSET(right_fd, &read_fds)) {
      //   cout << "from right!!!!" << endl;
      char from_right[512];
      len = recv(right_fd, from_right, sizeof(from_right), 0);
      from_right[len] = '\0';
      strcpy(temp, from_right);
      char * temp_ptr;
      //input: Start! Available Hops #<num_hops>
      temp_ptr = strtok(temp, "#");
      //this part is <num_hops>
      temp_ptr = strtok(NULL, " ");
      //store this value
      num_hops = atoi(temp_ptr);
      cout << "Recevive potato from Player " << right_player_id << ", Existing number of hops "
           << num_hops << endl;
      //cout << "testing from right number of hops: " << num_hops << endl;
    }
    //this case is from left player;
    else if (FD_ISSET(left_fd, &read_fds)) {
      // cout << "from left !!!" << endl;
      char from_left[512];
      len = recv(left_fd, from_left, sizeof(from_left), 0);
      from_left[len] = '\0';

      strcpy(temp, from_left);
      char * temp_ptr;
      //input: Start! Available Hops #<num_hops>
      temp_ptr = strtok(temp, "#");
      //this part is <num_hops>
      temp_ptr = strtok(NULL, " ");
      //store this value
      num_hops = atoi(temp_ptr);
      cout << "recevive potato from Player " << left_player_id << ", Existing number of hops "
           << num_hops << endl;
      //      cout << "testing from left number of hops: " << num_hops << endl;
    }
    //game time
    if (num_hops == 1) {
      //num_hops--=0
      cout << "I'm it!" << endl;
      char end_msg[64];
      sprintf(end_msg, "%s %d", "Player_ID:", player_id);
      len = send(player_fd, end_msg, strlen(end_msg), 0);
    }
    //time for left and right pass
    else {
      num_hops--;
      //right

      if (rand() % 2 == 0) {
        char Potato_str_right[512];
        sprintf(Potato_str_right,
                "%s %s %s %s %s %d %s %s %s%d",
                "Pass",
                "potato",
                "right",
                "to",
                "Player",
                right_player_id,
                "Available",
                "Hops",
                "#",
                num_hops);
        cout << "Passing Statement: " << Potato_str_right << endl;
        len = send(right_fd, Potato_str_right, strlen(Potato_str_right), 0);
        //   cout << "finish sending right" << endl;
      }
      //left
      else {
        char Potato_str_left[512];
        sprintf(Potato_str_left,
                "%s %s %s %s %s %d %s %s %s%d",
                "Pass",
                "potato",
                "left",
                "to",
                "Player",
                left_player_id,
                "Available",
                "Hops",
                "#",
                num_hops);
        cout << "Passing Statement: " << Potato_str_left << endl;
        len = send(left_fd, Potato_str_left, strlen(Potato_str_left), 0);
      }
    }
  }
}

int main(int argc, char * argv[]) {
  // struct addrinfo host_info;
  //struct addrinfo * host_info_list;
  srand(time(NULL));

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

  port_num_player = 1000 + player_id;
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
  sprintf(buffer, "%d %s", port_num_player, player_hostname);
  cout << " things to be sent " << buffer << endl;
  len = send(player_fd, buffer, strlen(buffer), 0);
  cout << "player info sent success" << endl;

  cout << "Waiting for connection on port " << port_num_player << endl;
  //neighbour info including id, host and port
  char right_info[512];
  len = recv(player_fd, right_info, sizeof(right_info), 0);
  right_info[len] = '\0';
  //parse info
  cout << "received neigh info " << right_info << endl;
  strcpy(temp, right_info);
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
  cout << "settings for right finished" << endl;
  set_right_player();
  cout << "finish setting right player" << endl;

  //send ready
  char ready_msg[64];
  sprintf(ready_msg, "%s %d %s %s %s %s", "Player", player_id, "is", "ready", "to", "play!");
  len = send(player_fd, ready_msg, strlen(ready_msg), 0);
  cout << "ready_msg sent" << endl;
  connect_neigh();
  cout << "Starting Playing" << endl;
  play();
  sleep(3);
  close(player_fd);
  close(left_fd);
  return 0;
}
