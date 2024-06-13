cmake_minimum_required (VERSION 2.6)

project (test)
enable_testing()

include_directories (../)

find_library(LIB_MUDUO_BASE libmuduo_base.a PATHS ~/Desktop/muduo-iouring/build/lib/)
find_library(LIB_MUDUO_NET libmuduo_net.a PATHS ~/Desktop/muduo-iouring/build/lib/)

link_directories(/home/pjj/CodeLib/gtest/googletest/build/lib/)

add_executable(IoUringPollerTest IoUringPollerTest.cc)


target_link_libraries (IoUringPollerTest ${LIB_MUDUO_NET})
target_link_libraries (IoUringPollerTest ${LIB_MUDUO_BASE})
target_link_libraries (IoUringPollerTest pthread rt uring)
target_link_libraries(IoUringPollerTest gtest gtest_main gmock)

add_test(NAME IoUringPollerTest COMMAND IoUringPollerTest)


add_executable(FileTest FileTest.cc)

target_link_libraries (FileTest ${LIB_MUDUO_NET})
target_link_libraries (FileTest ${LIB_MUDUO_BASE})
target_link_libraries (FileTest pthread rt uring)
target_link_libraries(FileTest gtest gtest_main gmock)

add_test(NAME FileTest COMMAND FileTest)