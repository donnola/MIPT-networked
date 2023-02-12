#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>
#include "socket_tools.h"

void keep_alive(int sfd, addrinfo res_addr)
{
  while(true)
  {
    std::string input = "keep_alive___" + std::to_string(std::rand());
    ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, res_addr.ai_addr, res_addr.ai_addrlen);
    if (res == -1)
    {
      std::cout << strerror(errno) << std::endl;
    }
    sleep(30);
  }
}

void send_message(int sfd, addrinfo res_addr)
{
  while(true)
  {
    std::thread t_sender([&]() { 
    std::string input;
    std::getline(std::cin, input);
    input += "___" + std::to_string(std::rand());
    ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, res_addr.ai_addr, res_addr.ai_addrlen);
    if (res == -1)
    {
      std::cout << strerror(errno) << std::endl;
    }
    printf("> %s\n", input.c_str());
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    t_sender.join();
  }
}

void listen_message(int sfd)
{
  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd + 1, &readSet, NULL, NULL, &timeout);


    if (FD_ISSET(sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, nullptr, nullptr);
      if (numBytes > 0)
      {
        printf("< %s\n", buffer); // assume that buffer is a string
      }
    }
  }
}

int main(int argc, const char **argv)
{
  const char *port_1 = "2222";
  const char *port_2 = "3333";

  addrinfo resAddrInfo;
  int sfd_client_sender = create_dgram_socket("localhost", port_1, &resAddrInfo);
  int sfd_client_listener = create_dgram_socket(nullptr, port_2, nullptr);

  if (sfd_client_listener == -1)
  {
    return 1;
  }
  if (sfd_client_sender == -1)
  {
    printf("Cannot create a socket\n");
    return 1;
  }
  printf("listening!\n");

  srand(time(0));
  std::string input;
  input = "initial data from client" + std::to_string(std::rand());
  printf(">>>> %s\n", input.c_str());
  ssize_t res = sendto(sfd_client_sender, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
  if (res == -1)
  {
    std::cout << strerror(errno) << std::endl;
  }

  std::thread t0(keep_alive, sfd_client_sender, resAddrInfo);
  std::thread t1(send_message, sfd_client_sender, resAddrInfo);
  std::thread t2(listen_message, sfd_client_listener);
  
  t0.join();
  t1.join();
  t2.join();

  return 0;
}
