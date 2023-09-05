#include "load_client.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

bool LoadClient::Load(int argc, char** argv) {
    if (argc <= 1)
        return false;
    int socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sun_family = AF_UNIX;
    strncpy(addr_server.sun_path + 1, "wavey", sizeof(addr_server.sun_path) - 2);
    for (int i = 1; i < argc; ++i) {
        if (sendto(socket_fd, argv[i], strlen(argv[1]), 0, (const struct sockaddr*)&addr_server,
                   sizeof(addr_server)) < 0)
            return false;
    }
    close(socket_fd);
    return true;
}
