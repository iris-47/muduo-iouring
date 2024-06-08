// IoUringPoller.h
#ifndef MUDUO_NET_IORING_POLLER_H
#define MUDUO_NET_IORING_POLLER_H

#include "muduo/net/Poller.h"
#include <liburing.h>
#include <vector>

namespace muduo{
namespace net{
class IoUringPoller : public Poller {
public:
    IoUringPoller(EventLoop* loop);
    ~IoUringPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    struct io_uring ring_;
    // struct io_uring_cqe *cqeVec_[32];
    std::vector<struct io_uring_cqe*> events_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_IORING_POLLER_H