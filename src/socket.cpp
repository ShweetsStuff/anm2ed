#include "socket.h"

#include <cerrno>

namespace anm2ed
{
  namespace
  {
#ifdef _WIN32
    struct WSAInitializer
    {
      WSAInitializer()
      {
        WSADATA data{};
        WSAStartup(MAKEWORD(2, 2), &data);
      }
      ~WSAInitializer() { WSACleanup(); }
    };

    WSAInitializer initializer{};
#endif

    int socket_last_error()
    {
#ifdef _WIN32
      return WSAGetLastError();
#else
      return errno;
#endif
    }
  }

  Socket::Socket() : handle(SOCKET_INVALID), role(CLIENT), lastError(0) {}

  Socket::Socket(Socket&& other) noexcept : handle(other.handle), role(other.role), lastError(other.lastError)
  {
    other.handle = SOCKET_INVALID;
    other.lastError = 0;
  }

  Socket& Socket::operator=(Socket&& other) noexcept
  {
    if (this != &other)
    {
      close();
      handle = other.handle;
      role = other.role;
      other.handle = SOCKET_INVALID;
      lastError = other.lastError;
      other.lastError = 0;
    }

    return *this;
  }

  Socket::~Socket() { close(); }

  bool Socket::open(SocketRole newRole)
  {
    close();
    role = newRole;

    handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (!is_valid())
    {
      lastError = socket_last_error();
      return false;
    }

    if (role == SERVER)
    {
#ifdef _WIN32
      BOOL opt = TRUE;
      if (::setsockopt(handle, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&opt), sizeof(opt)) != 0)
      {
        lastError = socket_last_error();
        close();
        return false;
      }
#else
      int opt = 1;
      if (::setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) != 0)
      {
        lastError = socket_last_error();
        close();
        return false;
      }
#endif
    }

    lastError = 0;
    return true;
  }

  bool Socket::bind(const SocketAddress& address)
  {
    if (!is_valid())
    {
      lastError = EINVAL;
      return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(address.port);

    if (address.host.empty())
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else
    {
      if (::inet_pton(AF_INET, address.host.c_str(), &addr.sin_addr) <= 0)
      {
        lastError = socket_last_error();
        return false;
      }
    }

    if (::bind(handle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0)
    {
      lastError = 0;
      return true;
    }

    lastError = socket_last_error();
    return false;
  }

  bool Socket::listen()
  {
    if (!is_valid())
    {
      lastError = EINVAL;
      return false;
    }
    if (::listen(handle, SOMAXCONN) == 0)
    {
      lastError = 0;
      return true;
    }

    lastError = socket_last_error();
    return false;
  }

  Socket Socket::accept()
  {
    Socket client{};
    if (!is_valid()) return client;

    auto accepted = ::accept(handle, nullptr, nullptr);
    if (accepted == SOCKET_INVALID)
    {
      lastError = socket_last_error();
      return client;
    }

    client.close();
    client.handle = accepted;
    client.role = CLIENT;
    lastError = 0;
    return client;
  }

  bool Socket::connect(const SocketAddress& address)
  {
    if (!is_valid())
    {
      lastError = EINVAL;
      return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(address.port);

    if (::inet_pton(AF_INET, address.host.c_str(), &addr.sin_addr) <= 0)
    {
      lastError = socket_last_error();
      return false;
    }

    if (::connect(handle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0)
    {
      lastError = 0;
      return true;
    }

    lastError = socket_last_error();
    return false;
  }

  bool Socket::send(const void* data, size_t size)
  {
    if (!is_valid() || !data || size == 0)
    {
      lastError = EINVAL;
      return false;
    }

    auto bytes = reinterpret_cast<const char*>(data);
    size_t totalSent = 0;

    while (totalSent < size)
    {
      auto sent = ::send(handle, bytes + totalSent, static_cast<int>(size - totalSent), 0);
      if (sent <= 0)
      {
        lastError = socket_last_error();
        return false;
      }
      totalSent += static_cast<size_t>(sent);
    }

    lastError = 0;
    return true;
  }

  bool Socket::receive(void* buffer, size_t size)
  {
    if (!is_valid() || !buffer || size == 0)
    {
      lastError = EINVAL;
      return false;
    }

    auto* bytes = reinterpret_cast<char*>(buffer);
    size_t totalReceived = 0;

    while (totalReceived < size)
    {
      auto received = ::recv(handle, bytes + totalReceived, static_cast<int>(size - totalReceived), 0);
      if (received <= 0)
      {
        lastError = socket_last_error();
        return false;
      }
      totalReceived += static_cast<size_t>(received);
    }

    lastError = 0;
    return true;
  }

  void Socket::close()
  {
    if (!is_valid()) return;

#ifdef _WIN32
    ::closesocket(handle);
#else
    ::close(handle);
#endif
    handle = SOCKET_INVALID;
    lastError = 0;
  }

  bool Socket::is_valid() const { return handle != SOCKET_INVALID; }

  int Socket::last_error() const { return lastError; }
}
