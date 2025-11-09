#pragma once

#include <cstddef>
#include <string>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
using socket_handle = SOCKET;
constexpr socket_handle SOCKET_INVALID = INVALID_SOCKET;
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
using socket_handle = int;
constexpr socket_handle SOCKET_INVALID = -1;
#endif

namespace anm2ed
{
  enum SocketRole
  {
    SERVER,
    CLIENT
  };

  struct SocketAddress
  {
    std::string host{};
    unsigned short port{};
  };

  class Socket
  {
  private:
    socket_handle handle;
    SocketRole role{};

  public:
    Socket();
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    bool open(SocketRole role);
    bool bind(const SocketAddress&);
    bool listen();
    Socket accept();
    bool connect(const SocketAddress&);

    bool send(const void*, size_t);
    bool receive(void*, size_t);

    void close();
    bool is_valid() const;
  };
}
