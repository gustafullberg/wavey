#include "file_notification.hpp"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <map>
#include <thread>
#include <vector>

namespace {
class MacFileModificationNotifier : public FileModificationNotifier {
   public:
    MacFileModificationNotifier(std::function<void(int)> on_modification)
        : on_modification_(on_modification) {
        kq_ = kqueue();
        assert(kq_ != -1);
        if (pipe(stop_pipe_) == -1) {
            assert(false);
        }

        struct kevent ev;
        EV_SET(&ev, stop_pipe_[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
        if (kevent(kq_, &ev, 1, NULL, 0, NULL) == -1) {
            assert(false);
        }

        monitor_thread_ =
            std::make_unique<std::thread>(&MacFileModificationNotifier::Monitor, this);
    }

    ~MacFileModificationNotifier() {
        const char c = 'q';
        write(stop_pipe_[1], &c, 1);
        if (monitor_thread_ && monitor_thread_->joinable()) {
            monitor_thread_->join();
        }

        for (auto const& [id, fd] : watch_map_) {
            close(fd);
        }
        close(kq_);
        close(stop_pipe_[0]);
        close(stop_pipe_[1]);
    }

    int Watch(const std::string& filename) override {
        int fd = open(filename.c_str(), O_RDONLY);
        if (fd == -1) {
            return -1;
        }

        struct kevent ev;
        // Watch for writes, extensions, attribute changes, deletions, and renames.
        EV_SET(&ev, fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR,
               NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_DELETE | NOTE_RENAME, 0,
               (void*)(intptr_t)fd);

        if (kevent(kq_, &ev, 1, NULL, 0, NULL) == -1) {
            close(fd);
            return -1;
        }

        watch_map_[fd] = fd;
        return fd;
    }

    void Unwatch(int id) override {
        auto it = watch_map_.find(id);
        if (it != watch_map_.end()) {
            struct kevent ev;
            EV_SET(&ev, it->second, EVFILT_VNODE, EV_DELETE, 0, 0, NULL);
            kevent(kq_, &ev, 1, NULL, 0, NULL);
            close(it->second);
            watch_map_.erase(it);
        }
    }

   private:
    void Monitor() {
        struct kevent ev;
        for (;;) {
            int n = kevent(kq_, NULL, 0, &ev, 1, NULL);
            if (n > 0) {
                if (ev.ident == (uintptr_t)stop_pipe_[0]) {
                    break;
                }
                if (ev.filter == EVFILT_VNODE) {
                    on_modification_((int)(intptr_t)ev.udata);

                    // If the file was deleted or renamed, the watch is effectively dead
                    // on current fd. But the listener will call Reload() which uses path-based
                    // stat.
                }
            } else if (n == -1 && errno != EINTR) {
                break;
            }
        }
    }

    std::function<void(int)> on_modification_;
    int kq_;
    int stop_pipe_[2];
    std::map<int, int> watch_map_;
    std::unique_ptr<std::thread> monitor_thread_;
};
}  // namespace

std::unique_ptr<FileModificationNotifier> FileModificationNotifier::Create(
    std::function<void(int)> on_modification) {
    return std::make_unique<MacFileModificationNotifier>(on_modification);
}
