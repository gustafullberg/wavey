#include "file_load_server.hpp"

#include <assert.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace {
constexpr size_t BUFFER_LENGTH = 4096;

sockaddr_un GetServerAddress() {
    sockaddr_un addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sun_family = AF_UNIX;
    // Abstract socket namespace.
    strncpy(addr_server.sun_path + 1, "wavey", sizeof(addr_server.sun_path) - 2);
    return addr_server;
}
}  // namespace

FileLoadServer::FileLoadServer(std::function<void(const std::string&)> on_load) : on_load(on_load) {
    // Create and bind socket.
    socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    sockaddr_un addr_server = GetServerAddress();
    bind(socket_fd, (sockaddr*)&addr_server, sizeof(addr_server));

    close_fd = eventfd(0, 0);
    monitor_thread = std::make_unique<std::thread>(&FileLoadServer::Monitor, this);
}

FileLoadServer::~FileLoadServer() {
    const uint64_t one = 1;
    write(close_fd, &one, sizeof(one));
    monitor_thread->join();
    close(socket_fd);
    close(close_fd);
}

void FileLoadServer::Monitor() {
    char buffer[BUFFER_LENGTH];
    constexpr int kSocketIndex = 0;
    constexpr int kCloseIndex = 1;
    struct pollfd poll_fds[2];
    poll_fds[kSocketIndex] = {
        .fd = socket_fd,
        .events = POLLIN,
    };
    poll_fds[kCloseIndex] = {
        .fd = close_fd,
        .events = POLLIN,
    };
    buffer[BUFFER_LENGTH - 1] = '\0';

    for (;;) {
        int poll_num = poll(poll_fds, 2, -1);
        assert(poll_num != -1);
        if (poll_num > 0) {
            if (poll_fds[kCloseIndex].revents & POLLIN) {
                // Read the event counter.
                uint64_t c;
                read(close_fd, &c, sizeof(c));
                break;
            }

            if (poll_fds[kSocketIndex].revents & POLLIN) {
                ssize_t len = read(socket_fd, buffer, sizeof(buffer) - 1);
                if (len > 0) {
                    buffer[len] = '\0';
                    std::string file_name(buffer);
                    on_load(file_name);
                }
            }
        }
    }
}

bool FileLoadServer::Load(int argc, char** argv) {
    // No files to load?
    if (argc <= 1)
        return false;

    int socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    sockaddr_un addr_server = GetServerAddress();
    for (int i = 1; i < argc; ++i) {
        if (sendto(socket_fd, argv[i], strlen(argv[i]), 0, (const struct sockaddr*)&addr_server,
                   sizeof(addr_server)) < 0)
            return false;
    }
    close(socket_fd);
    return true;
}
