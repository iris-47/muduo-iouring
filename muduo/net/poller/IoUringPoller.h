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

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override; // 返回触发事件的所有fd对应的channel
    void updateChannel(Channel* channel) override;  // 新增一个channel(也就是新建立对一个fd的监控)
    void removeChannel(Channel* channel) override;

private:
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const; // 模仿的EpollPoller, 填充activeChannels

    struct io_uring ring_; // io-uring核心变量

    std::vector<struct io_uring_cqe*> events_; // 用于在一次poll()中存储从cqe中获取的所有结果
};

struct PollerInfo{
    Channel* channel;
    uint32_t events; // 事件类型
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_IORING_POLLER_H