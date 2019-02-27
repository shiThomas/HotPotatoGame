#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
using namespace std;
#include "potato.h"

int status;
int master_fd;
int len;
//struct addrinfo host_info;
//struct addrinfo * host_info_list;
//struct sockaddr_storage socket_addr;
struct sockaddr_in newplayer, master_sock;
char host[64], str[64];
char buffer[40960], temp[40960];
struct hostent * master_host_addr;
socklen_t newplayer_addr_len = sizeof(newplayer);
//socklen_t socket_addr_len = sizeof(socket_addr);

void set_server(char host[64], const char * port, int port_num) {
  //set address and port
  master_sock.sin_family = AF_INET;
  master_sock.sin_port = htons(port_num);
  //  cout << "before memcpy" << endl;
  memcpy(&master_sock.sin_addr, master_host_addr->h_addr_list[0], master_host_addr->h_length);
  // cout << "finish memcpy" << endl;
  master_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (master_fd == -1) {
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

  status = listen(master_fd, 5);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << host << "," << port << ")" << endl;
    exit(-1);
  }  //if

  //cout << "Waiting for connection on port " << port << endl;
}
//connect to players and store information in potato
potato connect_player(potato player) {
  player.playerfd = accept(master_fd, (struct sockaddr *)&newplayer, &newplayer_addr_len);
  if (player.playerfd == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    exit(-1);
  }
  player.host_addr = gethostbyaddr((char *)&newplayer.sin_addr, sizeof(struct in_addr), AF_INET);
  //update potato struct
  return player;
}

void neigh_setup(potato * player_list, int num_players) {
  //receive msg from player
  for (int i = 0; i < num_players; i++) {
    len = recv(player_list[i].playerfd, buffer, 32, 0);
    buffer[len] = '\0';
    // cout << "Server received: " << buffer << endl;
    if (len > 0) {
      strcpy(temp, buffer);
      char * temp_ptr;
      temp_ptr = strtok(temp, " ");
      player_list[i].player_port = atoi(temp_ptr);
      temp_ptr = strtok(NULL, " ");
      strcpy(player_list[i].player_hostname, temp_ptr);
    }
    //  cout << "hostname " << player_list[i].player_hostname << endl;
    //cout << "port " << player_list[i].player_port << endl;
  }

  //inform players of their right neighbour
  for (int i = 0; i < num_players; i++) {
    char neigh_info[512];
    sprintf(neigh_info,
            "%d %d %s %d",

            num_players,
            player_list[(i + 1) % num_players].player_id,
            player_list[(i + 1) % num_players].player_hostname,
            player_list[(i + 1) % num_players].player_port);
    // cout << "Neigh Info " << neigh_info << endl;
    len = send(player_list[i].playerfd, neigh_info, strlen(neigh_info), 0);
  }
}

int main(int argc, char * argv[]) {
  potato * player_list;
  srand(time(NULL));
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <port_num> <num_players> <num_hops>\n", argv[0]);
    exit(1);
  }
  int port_num = atoi(argv[1]);
  int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);
  //Initial Erro Checking

  if (port_num < 0) {
    printf("Invalid Port Numebr\n");
    return 0;
  }
  if (num_players < 2 || num_hops < 0 || num_hops > 512) {
    printf("Invalid Number of Players/Hops\n");
    return 0;
  }

  //  printf("<port_num>: %d,  <num_players>: %d,  <num_hops>: %d\n", port_num, num_players, num_hops);
  //Just for communication between master and player
  //set port number to argv[1]
  const char * port = argv[1];

  gethostname(host, sizeof(host));
  master_host_addr = gethostbyname(host);

  if (master_host_addr == NULL) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }
  //cout << "before setserver" << endl;

  set_server(host, port, port_num);
  //cout << "finish setserver" << endl;

  //  cout << "Port Number " << port_num << endl;
  cout << "Potato Ringmaster" << endl;
  cout << "Players = " << num_players << endl;
  cout << "Hops = " << num_hops << endl;

  // int client_connection_fd;
  //initialize player list;
  player_list = (potato *)malloc(num_players * sizeof(struct potato_t));
  //assign value to each of player in the list
  // cout << "finish malloc" << endl;
  for (int i = 0; i < num_players; i++) {
    player_list[i] = connect_player(player_list[i]);
    //set value to potato struct
    player_list[i].hops_total = num_hops;
    player_list[i].player_num = num_players;
    player_list[i].player_id = i;
    // cout << "Player " << player_list[i].player_id << " on PORT " << port_num << endl;
    //send

    //len = send(player_list[i].playerfd, player_list[i], sizeof(struct potato_t), 0);
  }

  //finish sending player_id, send msg to activate player
  for (int i = 0; i < num_players; i++) {
    //   sprintf(buffer, "%s", "ready");
    //   len = send(player_list[i].playerfd, buffer, 5, 0);
    //send player_id to player
    sprintf(str, "%d", player_list[i].player_id);
    len = send(player_list[i].playerfd, str, strlen(str), 0);
  }

  // client_connection_fd = accept(master_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  // if (client_connection_fd == -1) {
  //   cerr << "Error: cannot accept connection on socket" << endl;
  //   return -1;
  // }  //if

  //receive msgs from players
  neigh_setup(player_list, num_players);
  //check msg Player <number> is ready to play

  char ready_msg[64];
  for (int i = 0; i < num_players; i++) {
    len = recv(player_list[i].playerfd, ready_msg, sizeof(ready_msg), 0);
    ready_msg[len] = '\0';
    sleep(1);
    cout << ready_msg << endl;
  }

  //game start
  //Define some variables for game start here
  sleep(3);
  //when there is no hops
  if (num_hops == 0) {
    //  cout << "Ready to start the game, sending potato to player " << endl;
    //cout << "Trace of Potato:" << endl;
    //cout << "" << endl;
    char exit_msg[64];
    sprintf(exit_msg, "%s", "exit");
    //inform players to exit
    for (int i = 0; i < num_players; i++) {
      len = send(player_list[i].playerfd, exit_msg, strlen(exit_msg), 0);
      // if (len != strlen(str)) {
      // perror("send");
      // exit(1);
    }
  }
  //When hops exist
  else {
    //initialize variables for game;

    int first_player_id = rand() % num_players;
    int fdmax;
    fdmax = master_fd;
    fd_set read_fds;     // temp file descriptor list for select()
    FD_ZERO(&read_fds);  //clear temp sets
    for (int i = 0; i < num_players; i++) {
      if (player_list[i].playerfd > fdmax) {
        fdmax = player_list[i].playerfd;
      }
      FD_SET(player_list[i].playerfd, &read_fds);
    }
    cout << "Ready to start the game, sending potato to player " << first_player_id << endl;
    sprintf(str, "%s %s %s %s%d", "Start!", "Available", " Hops", "#", num_hops);
    //send starting info to the selected player
    // cout << "sending" << str << endl;
    len = send(player_list[first_player_id].playerfd, str, strlen(str), 0);

    len = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
    if (len == -1) {
      perror("select");
      exit(4);
    }
    int Player_return = 0;

    for (int i = 0; i < num_players; i++) {
      if (FD_ISSET(player_list[i].playerfd, &read_fds)) {
        FD_CLR(player_list[i].playerfd, &read_fds);
        Player_return = i;
        break;
      }
    }
    //where trace info is acquired
    //   for(int i =0;i<num_hops;i++){
    //len = acq
    //}

    //Where master receives return
    char return_msg[40960];
    len = recv(player_list[Player_return].playerfd, return_msg, sizeof(return_msg), 0);
    return_msg[len] = '\0';
    //cout << "Player " << return_msg << " returned potato" << endl;
    //haven't figured out a way to print trace
    cout << "Trace of Potato:" << endl;
    cout << return_msg << endl;
    char exit_m[64];
    sprintf(exit_m, "%s", "exit");
    //inform players to exit
    for (int i = 0; i < num_players; i++) {
      len = send(player_list[i].playerfd, exit_m, strlen(exit_m), 0);
    }
  }

  //closing window
  sleep(2);
  close(master_fd);
  return 0;
}
