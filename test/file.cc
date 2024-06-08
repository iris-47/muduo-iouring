#include "muduo/net/EventLoop.h"
#include "muduo/net/File.h"

#include <fcntl.h>
#include <iostream>

using namespace muduo;
using namespace muduo::net;

int main()
{
  EventLoop loop;
  File file(&loop, "examplefile.txt", O_WRONLY|O_CREAT);

  file.write("Hello io_uring!");
  // file.read();

  loop.loop();
  return 0;
}