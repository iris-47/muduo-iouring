#include "File.h"
#include <iostream>
#include <muduo/base/Logging.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

using namespace muduo;
using namespace muduo::net;

File::File(EventLoop* loop, const std::string& filename, int flags)
    : loop_(loop),
      channel_(loop, ::open(filename.c_str(), flags, 0666))
{
    if (channel_.fd() < 0)
    {
        LOG_SYSFATAL << "Failed to open file: " << filename;
    }
}

File::~File()
{
    if (channel_.fd() >= 0)
    {
        ::close(channel_.fd());
    }
}

void File::write(const std::string& data)
{
    buffer_ = data;
    channel_.setWriteCallback(std::bind(&File::handleWrite, this));
    channel_.enableWriting(); // channel新增监听fd的read事件
}

void File::read()
{
    channel_.setReadCallback(std::bind(&File::handleRead, this));
    channel_.enableReading(); // channel新增监听fd的write事件
}

void File::handleWrite()
{
    ssize_t n = ::write(channel_.fd(), buffer_.data(), buffer_.size());
    if (n > 0)
    {
        LOG_INFO << "Wrote " << n << " bytes to file.";
        buffer_.erase(0, n);
        if (buffer_.empty())
        {
            channel_.disableWriting();
        }
    }
    else
    {
        LOG_ERROR << "Write error.";
    }
}

void File::handleRead()
{
    char buf[1024];
    ssize_t n = ::read(channel_.fd(), buf, sizeof(buf) - 1);
    if (n > 0)
    {
        buf[n] = '\0';
        LOG_INFO << "Read " << n << " bytes from file: " << buf;
        if (readCallback_)
        {
            readCallback_(std::string(buf));
        }
        else
        {
            std::cout.write(buf, n + 1);
        }
        channel_.disableReading();
    }
    else if (n == 0)
    {
        LOG_INFO << "EOF reached.";
        ::close(channel_.fd());
    }
    else
    {
        LOG_ERROR << "Read error.";
    }
}


void File::close()
{
    if (fd_ >= 0)
    {
        channel_.disableAll();
        ::close(fd_);
        fd_ = -1;
    }
}