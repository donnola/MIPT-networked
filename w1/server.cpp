#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <thread>
#include <iostream>
#include "socket_tools.h"

int main(int argc, const char **argv)
{
  const char *port_1 = "2222";
  const char *port_2 = "3333";

  addrinfo resAddrInfo;
  int sfd_server_sender = create_dgram_socket("localhost", port_2, &resAddrInfo);
  int sfd_server_listener = create_dgram_socket(nullptr, port_1, nullptr);

  if (sfd_server_listener == -1)
  {
    return 1;
  }
  if (sfd_server_sender == -1)
  {
    printf("Cannot create a socket\n");
    return 1;
  }
  printf("listening!\n");

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd_server_listener, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd_server_listener + 1, &readSet, NULL, NULL, &timeout);


    if (FD_ISSET(sfd_server_listener, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      ssize_t numBytes = recvfrom(sfd_server_listener, buffer, buf_size - 1, 0, nullptr, nullptr);
      if (numBytes > 0)
      {
        printf("< %s\n", buffer); // assume that buffer is a string
        std::string input = "get_package";
        ssize_t res = sendto(sfd_server_sender, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
        if (res == -1)
        {
          std::cout << strerror(errno) << std::endl;
        }
      }
    }
  }
  return 0;
}
