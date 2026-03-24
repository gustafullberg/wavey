#ifndef FILE_NOTIFICATION_HPP_
#define FILE_NOTIFICATION_HPP_

#include <functional>
#include <memory>
#include <string>
#include <thread>

class FileModificationNotifier {
   public:
    virtual ~FileModificationNotifier() = default;

    virtual int Watch(const std::string& filename) = 0;
    virtual void Unwatch(int id) = 0;

    static std::unique_ptr<FileModificationNotifier> Create(
        std::function<void(int)> on_modification);
};

#endif  // FILE_NOTIFICATION_HPP_
