#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
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
  memcpy(&master_sock.sin_addr, master_host_addr->h_addr_list[0], master_host_addr->h_length);

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

  cout << "Waiting for connection on port " << port << endl;
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
    len = recv(player_list[i].playerfd, buffer, strlen(buffer), 0);
    buffer[len] = '\0';
    cout << "Server received: " << buffer << endl;
    if (len > 0) {
      strcpy(temp, buffer);
      char * temp_ptr;
      temp_ptr = strtok(temp, " ");
      player_list[i].player_port = atoi(temp_ptr);
      temp_ptr = strtok(NULL, " ");
      strcpy(player_list[i].player_hostname, temp_ptr);
    }
  }
  //inform players of their right neighbour
  for (int i = 0; i < num_players; i++) {
    sprintf(buffer,
            "%d %d %s %d",

            num_players,
            player_list[(i + 1) % num_players].player_id,
            player_list[(i + 1) % num_players].player_hostname,
            player_list[(i + 1) % num_players].player_port);

    len = send(player_list[i].playerfd, buffer, strlen(buffer), 0);
  }
}

int main(int argc, char * argv[]) {
  potato * player_list;

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

  set_server(host, port, port_num);
  cout << "finish setserver" << endl;
  master_host_addr = gethostbyname(host);
  cout << "reached here" << endl;
  if (master_host_addr == NULL) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }

  cout << "Master on " << host << endl;
  cout << "Port Number" << port_num << endl;
  cout << "Number of Players " << num_players << endl;
  cout << "Number of Hops" << num_hops << endl;

  // int client_connection_fd;
  //initialize player list;
  player_list = (potato *)malloc(num_players * sizeof(struct potato_t));
  //assign value to each of player in the list
  cout << "finish malloc" << endl;
  for (int i = 0; i < num_players; i++) {
    player_list[i] = connect_player(player_list[i]);
    //set value to potato struct
    player_list[i].hops_total = num_hops;
    player_list[i].player_num = num_players;
    player_list[i].player_id = i;
    cout << "Player " << player_list[i].player_id << "on PORT " << port_num << endl;
    //send

    //send player_id to player
    sprintf(str, "%d", player_list[i].player_id);
    len = send(player_list[i].playerfd, str, strlen(str), 0);
    //len = send(player_list[i].playerfd, player_list[i], sizeof(struct potato_t), 0);
  }

  // client_connection_fd = accept(master_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  // if (client_connection_fd == -1) {
  //   cerr << "Error: cannot accept connection on socket" << endl;
  //   return -1;
  // }  //if

  //receive msgs from players
  neigh_setup(player_list, num_players);
  //check msg Player <number> is ready to play
  for (int i = 0; i < num_players; i++) {
    len = recv(player_list[i].playerfd, buffer, strlen(buffer), 0);
    buffer[len] = '\0';
    cout << "Server received: " << buffer << endl;
  }

  //game start
  //Define some variables for game start here

  //when there is no hops
  if (num_hops == 0) {
    cout << "Game has started, sending potato to player: No Player" << endl;
    cout << "Trace of Potato:" << endl;
    sprintf(str, "%s", "exit");
    //inform players to exit
    for (int i = 0; i < num_players; i++) {
      len = send(player_list[i].playerfd, str, strlen(str), 0);
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
    cout << "Game has started, sending potato to player: " << first_player_id << endl;
    sprintf(str, "%s %s %s %s%d", "Start!", "Available", " Hops", "#", num_hops);
    //send starting info to the selected player
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
    len = recv(player_list[Player_return].playerfd, buffer, strlen(buffer), 0);
    buffer[len] = '\0';
    cout << buffer << "returned potato" << endl;
    //haven't figured out a way to print trace
    cout << "Trace of Potato:" << endl;
    sprintf(str, "%s", "exit");
    //inform players to exit
    for (int i = 0; i < num_players; i++) {
      len = send(player_list[i].playerfd, str, strlen(str), 0);
    }
  }

  //closing window
  sleep(3);
  close(master_fd);
  return 0;
}
