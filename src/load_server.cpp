#include "load_server.hpp"

#include <assert.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

constexpr size_t BUFFER_LENGTH = 4096;

LoadServer::LoadServer(std::function<void(const std::string&)> on_load) : on_load(on_load) {
    // Create and bind socket.
    socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sun_family = AF_UNIX;
    strncpy(addr_server.sun_path + 1, "wavey", sizeof(addr_server.sun_path) - 2);
    bind(socket_fd, (sockaddr*)&addr_server, sizeof(addr_server));

    close_fd = eventfd(0, 0);
    monitor_thread = std::make_unique<std::thread>(&LoadServer::Monitor, this);
}

LoadServer::~LoadServer() {
    const uint64_t one = 1;
    write(close_fd, &one, sizeof(one));
    monitor_thread->join();
    close(socket_fd);
    close(close_fd);
}

void LoadServer::Monitor() {
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
