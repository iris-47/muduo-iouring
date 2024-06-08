#include "muduo/net/poller/IoUringPoller.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"
#include <gtest/gtest.h>
#include <sys/eventfd.h>

using namespace muduo;
using namespace muduo::net;

class IoUringPollerTest : public ::testing::Test {
protected:
  void SetUp() override {
    loop_ = new EventLoop();
    poller_ = new IoUringPoller(loop_);
  }

  void TearDown() override {
    delete poller_;
    delete loop_;
  }

  EventLoop* loop_;
  IoUringPoller* poller_;
};

TEST_F(IoUringPollerTest, PollerInitialization) {
  ASSERT_NE(poller_, nullptr);
}

TEST_F(IoUringPollerTest, AddChannel) {
  int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  ASSERT_NE(fd, -1);

  Channel channel(loop_, fd);
  poller_->updateChannel(&channel);

  // Simulate an event to test if the channel is handled correctly
  uint64_t one = 1;
  ssize_t n = ::write(fd, &one, sizeof one);
  ASSERT_EQ(n, sizeof one);

  Poller::ChannelList activeChannels;
  poller_->poll(1000, &activeChannels);

  ASSERT_EQ(activeChannels.size(), 1);
  ASSERT_EQ(activeChannels[0], &channel);

  ::close(fd);
}

TEST_F(IoUringPollerTest, RemoveChannel) {
  int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  ASSERT_NE(fd, -1);

  Channel channel(loop_, fd);
  poller_->updateChannel(&channel);
  poller_->removeChannel(&channel);

  Poller::ChannelList activeChannels;
  poller_->poll(1000, &activeChannels);

  ASSERT_EQ(activeChannels.size(), 0);

  ::close(fd);
}