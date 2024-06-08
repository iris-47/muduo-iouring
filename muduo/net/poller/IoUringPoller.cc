#include "muduo/net/poller/IoUringPoller.h"
#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

IoUringPoller::IoUringPoller(EventLoop* loop)
    : Poller(loop) {
    int ret = io_uring_queue_init(256, &ring_, 0);
    if (ret < 0) {
        LOG_SYSFATAL << "io_uring_queue_init failed";
    }
}

IoUringPoller::~IoUringPoller() {
    io_uring_queue_exit(&ring_);
}

Timestamp IoUringPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    struct io_uring_cqe* cqe;

    int ret = io_uring_wait_cqe_timeout(&ring_, &cqe, nullptr);

    if (ret >= 0) {
        events_.clear();
        while (ret == 0) {
            events_.push_back(cqe);
            io_uring_cqe_seen(&ring_, cqe);
            ret = io_uring_peek_cqe(&ring_, &cqe);
        }
        fillActiveChannels(static_cast<int>(events_.size()), activeChannels);
    } else if (ret < 0 && ret != -ETIME) {
        LOG_ERROR << "io_uring_wait_cqe_timeout error: " << strerror(-ret);
    }

    return Timestamp::now();
}

void IoUringPoller::updateChannel(Channel* channel) {
    const int fd = channel->fd();
    const int events = channel->events();

    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
        LOG_ERROR << "io_uring_get_sqe failed";
        return;
    }

    io_uring_prep_poll_add(sqe, fd, events);
    io_uring_sqe_set_data(sqe, channel);
    int ret = io_uring_submit(&ring_);
    if (ret < 0) {
        LOG_ERROR << "io_uring_submit failed: " << strerror(-ret);
    }
    channels_[fd] = channel;
}

void IoUringPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    channels_.erase(fd);

    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
        LOG_ERROR << "io_uring_get_sqe failed";
        return;
    }

    io_uring_prep_poll_remove(sqe, fd);

    int ret = io_uring_submit(&ring_);
    if (ret < 0) {
        LOG_ERROR << "io_uring_submit failed: " << strerror(-ret);
    }
}

void IoUringPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    for (int i = 0; i < numEvents; ++i) {
        int fd = events_[i]->res;
        auto it = channels_.find(fd);
        if (it != channels_.end()) {
            Channel* channel = it->second;
            channel->set_revents(events_[i]->flags);
            activeChannels->push_back(channel);
        }
    }
}