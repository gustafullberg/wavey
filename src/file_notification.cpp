#include "file_notification.hpp"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

constexpr size_t BUFFER_LENGTH = 4096;

FileModificationNotifier::FileModificationNotifier(std::function<void(int)> on_modification)
    : on_modification_(on_modification) {
    inotify_fd_ = inotify_init();
    close_fd_ = eventfd(0, 0);
    assert(inotify_fd_ > 0);
    monitor_thread_ = std::make_unique<std::thread>(&FileModificationNotifier::Monitor, this);
}

FileModificationNotifier::~FileModificationNotifier() {
    for (int wd : watch_descriptor_) {
        inotify_rm_watch(inotify_fd_, wd);
    }
    const uint64_t one = 1;
    assert(write(close_fd_, &one, sizeof(uint64_t)) == sizeof(uint64_t));
    monitor_thread_->join();
    close(inotify_fd_);
    close(close_fd_);
}

int FileModificationNotifier::Watch(const std::string& filename) {
    int wd = inotify_add_watch(inotify_fd_, filename.c_str(), IN_CLOSE_WRITE);
    watch_descriptor_.push_back(wd);
    return wd;
}

void FileModificationNotifier::Unwatch(int id) {
    if (std::find(watch_descriptor_.begin(), watch_descriptor_.end(), id) ==
        watch_descriptor_.end()) {
        return;
    }
    inotify_rm_watch(inotify_fd_, id);
}

void FileModificationNotifier::Monitor() {
    char buffer[BUFFER_LENGTH];
    const struct inotify_event* event;
    constexpr int kInotifyIndex = 0;
    constexpr int kCloseIndex = 1;
    struct pollfd poll_fds[2];
    poll_fds[kInotifyIndex] = {
        .fd = inotify_fd_,
        .events = POLLIN,
    };
    poll_fds[kCloseIndex] = {
        .fd = close_fd_,
        .events = POLLIN,
    };

    for (;;) {
        std::cerr << "Wait for event" << std::endl;
        int poll_num = poll(poll_fds, 2, -1);
        assert(poll_num != -1);
        if (poll_num > 0) {
            if (poll_fds[kCloseIndex].revents & POLLIN) {
                /* Console input is available. Empty stdin and quit */
                char c;
                while (read(close_fd_, &c, 1) > 0)
                    continue;
                break;
            }

            if (poll_fds[kInotifyIndex].revents & POLLIN) {
                ssize_t len = read(inotify_fd_, buffer, sizeof(buffer));
                event = (const struct inotify_event*)buffer;
                /* Loop over all events in the buffer */
                for (char* ptr = buffer; ptr < buffer + len;
                     ptr += sizeof(struct inotify_event) + event->len) {
                    event = (const struct inotify_event*)ptr;
                    if (event->mask & IN_CLOSE_WRITE) {
                        printf("IN_CLOSE_WRITE: ");
                        on_modification_(event->wd);
                    }
                }
            }
        }
    }
}
