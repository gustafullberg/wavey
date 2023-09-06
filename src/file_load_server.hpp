#ifndef FILE_LOAD_SERVER_HPP
#define FILE_LOAD_SERVER_HPP

#include <functional>
#include <memory>
#include <string>
#include <thread>

class FileLoadServer {
   public:
    FileLoadServer(std::function<void(const std::string&)> on_modification);
    ~FileLoadServer();

    // Returns true if the files could be loaded by an already running instance, false otherwise.
    static bool Load(int argc, char** argv);

   private:
    void Monitor();

    std::function<void(const std::string&)> on_load;

    int socket_fd;
    int close_fd;

    std::unique_ptr<std::thread> monitor_thread;
};

#endif  // FILE_LOAD_SERVER_HPP
