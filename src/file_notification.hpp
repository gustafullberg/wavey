#ifndef FILE_NOTIFICATION_HPP_
#define FILE_NOTIFICATION_HPP_

#include <functional>
#include <memory>
#include <string>
#include <thread>

class FileModificationNotifier {
   public:
    FileModificationNotifier(std::function<void(int)> on_modification);
    ~FileModificationNotifier();

    int Watch(const std::string& filename);
    void Unwatch(int id);

   private:
    void Monitor();

    std::function<void(int)> on_modification_;

    int inotify_fd_;
    int close_fd_;

    std::vector<int> watch_descriptor_;
    std::unique_ptr<std::thread> monitor_thread_;
};

#endif  // FILE_NOTIFICATION_HPP_
