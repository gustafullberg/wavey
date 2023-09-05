#ifndef LOAD_SERVER_HPP
#define LOAD_SERVER_HPP

#include <functional>
#include <memory>
#include <string>
#include <thread>

class LoadServer {
   public:
    LoadServer(std::function<void(const std::string&)> on_modification);
    ~LoadServer();

   private:
    void Monitor();

    std::function<void(const std::string&)> on_load;

    int socket_fd;
    int close_fd;

    std::unique_ptr<std::thread> monitor_thread;
};

#endif  // LOAD_SERVER_HPP
