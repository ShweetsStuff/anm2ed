#include "instance.h"

#include <SDL3/SDL.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using Handle = SOCKET;
constexpr Handle HANDLE_INVALID = INVALID_SOCKET;
constexpr auto SOCKET_REUSE = SO_EXCLUSIVEADDRUSE;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using Handle = int;
constexpr Handle HANDLE_INVALID = -1;
constexpr auto SOCKET_REUSE = SO_REUSEADDR;
#define closesocket close
#endif

constexpr auto INSTANCE_ADDRESS = "127.0.0.1";
constexpr auto INSTANCE_PORT = 11414;
// The listening editor's answer once it has every path, telling a later one it was heard.
constexpr char INSTANCE_ANSWER[] = "anm2ed";
constexpr int RECEIVE_TIMEOUT_MS = 1000;
constexpr int BUFFER_SIZE = 4096;
constexpr std::size_t MESSAGE_SIZE_MAX = 1 << 20;

static Handle listener = HANDLE_INVALID;
static std::thread listenThread;
static std::atomic<bool> isListening = false;
static std::mutex pendingMutex;
static std::optional<std::vector<std::string>> pending;

static sockaddr_in address_get()
{
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(INSTANCE_PORT);
    inet_pton(AF_INET, INSTANCE_ADDRESS, &address.sin_addr);
    return address;
}

// A silent peer can't hold a receive up for longer than RECEIVE_TIMEOUT_MS.
static void receive_timeout_set(Handle handle)
{
#ifdef _WIN32
    DWORD timeout = RECEIVE_TIMEOUT_MS;
#else
    timeval timeout = {RECEIVE_TIMEOUT_MS / 1000, RECEIVE_TIMEOUT_MS % 1000 * 1000};
#endif
    setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
}

// Sends each path followed by a zero, then waits for the answer.
static bool paths_send(const std::vector<std::string>& paths)
{
    auto handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    auto address = address_get();
    if (handle == HANDLE_INVALID)
        return false;
    if (connect(handle, (sockaddr*)&address, sizeof(address)) != 0)
    {
        closesocket(handle);
        return false;
    }

    std::string message;
    for (auto& path : paths)
        message += path + '\0';
    auto isSent = send(handle, message.data(), (int)message.size(), 0) == (int)message.size();
#ifdef _WIN32
    shutdown(handle, SD_SEND);
#else
    shutdown(handle, SHUT_WR);
#endif
    receive_timeout_set(handle);
    char answer[sizeof(INSTANCE_ANSWER)] = {};
    auto isAnswered = isSent && recv(handle, answer, sizeof(answer), MSG_WAITALL) == sizeof(answer) &&
                      std::string_view(answer, sizeof(answer)) == std::string_view(INSTANCE_ANSWER, sizeof(answer));
    closesocket(handle);
    return isAnswered;
}

// Queues the paths for instance_paths_take before answering, and wakes the event loop if it is up yet.
static void paths_receive(Handle client)
{
    std::string message;
    char buffer[BUFFER_SIZE];
    receive_timeout_set(client);
    for (int count = 0; message.size() < MESSAGE_SIZE_MAX && (count = recv(client, buffer, sizeof(buffer), 0)) > 0;)
        message.append(buffer, count);
    {
        std::lock_guard lock(pendingMutex);
        auto& paths = pending ? *pending : pending.emplace();
        for (std::size_t start = 0, end = 0; (end = message.find('\0', start)) != std::string::npos; start = end + 1)
            if (end != start)
                paths.push_back(message.substr(start, end - start));
    }
    send(client, INSTANCE_ANSWER, sizeof(INSTANCE_ANSWER), 0);
    closesocket(client);
    SDL_Event event = {.type = SDL_EVENT_USER};
    SDL_PushEvent(&event);
}

std::optional<std::vector<std::string>> instance_paths_take()
{
    std::lock_guard lock(pendingMutex);
    return std::exchange(pending, std::nullopt);
}

InstanceRole instance_start(const std::vector<std::string>& paths)
{
#ifdef _WIN32
    WSADATA data = {};
    WSAStartup(MAKEWORD(2, 2), &data);
#endif
    auto handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    auto address = address_get();
    int isReused = 1;
    setsockopt(handle, SOL_SOCKET, SOCKET_REUSE, (const char*)&isReused, sizeof(isReused));
    if (handle == HANDLE_INVALID || bind(handle, (sockaddr*)&address, sizeof(address)) != 0 ||
        listen(handle, SOMAXCONN) != 0)
    {
        if (handle != HANDLE_INVALID)
            closesocket(handle);
        return paths_send(paths) ? INSTANCE_FORWARDED : INSTANCE_ALONE;
    }

    listener = handle;
    isListening = true;
    listenThread = std::thread([] {
        while (isListening)
            if (auto client = accept(listener, nullptr, nullptr); client != HANDLE_INVALID)
                paths_receive(client);
    });
    return INSTANCE_FIRST;
}

// Wakes the listening thread with an empty hand-off so it can see it should stop.
void instance_stop()
{
    if (!isListening)
        return;
    isListening = false;
    paths_send({});
    listenThread.join();
    closesocket(listener);
}
