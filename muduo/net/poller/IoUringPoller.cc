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

    // 将timeoutMs转化为__kernel_timespec
    struct __kernel_timespec ts;
    ts.tv_sec = timeoutMs / 1000;
    ts.tv_nsec = (timeoutMs % 1000) * 1000000;
    
    // 获取cqe中被触发的fd
    int ret = io_uring_wait_cqe_timeout(&ring_, &cqe, &ts);

    if (ret >= 0) {
        events_.clear();

        // 结果暂存到events_中
        while (ret == 0) {
            events_.push_back(cqe);
            io_uring_cqe_seen(&ring_, cqe);
            ret = io_uring_peek_cqe(&ring_, &cqe);
        }

        // 将结果填充到activateChannels中
        fillActiveChannels(static_cast<int>(events_.size()), activeChannels);
    } else if (ret < 0 && ret != -ETIME) {
        LOG_ERROR << "io_uring_wait_cqe_timeout error: " << strerror(-ret);
    }

    return Timestamp::now();
}

void IoUringPoller::updateChannel(Channel* channel) {
    const int fd = channel->fd();

    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
        LOG_ERROR << "io_uring_get_sqe failed";
        return;
    }

    io_uring_prep_poll_add(sqe, fd, channel->events()); // POLL事件和EPOLL事件的标志是一样的。

    // io-uring cqe返回结果中没有revent,因此在user_data中设置
    PollerInfo* user_data = new(PollerInfo);
    user_data->channel = channel;
    user_data->events = channel->events();

    io_uring_sqe_set_data(sqe, user_data);
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
        PollerInfo* user_data = reinterpret_cast <PollerInfo*>(events_[i]->user_data);

        Channel* channel = user_data->channel;
        channel->set_revents(user_data->events);
        activeChannels->push_back(channel);

        delete(user_data); // user_data由updateChannel()函数new。
    }
}