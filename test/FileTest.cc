#include <gtest/gtest.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/File.h>
#include <muduo/base/Logging.h>
#include <fstream>
#include <fcntl.h>
#include <string>

using namespace muduo;
using namespace muduo::net;

class FileTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a test file with some content
        std::ofstream outfile(filename_);
        outfile << "Hello, Muduo!" << std::endl;
        outfile.close();

        loop_ = new EventLoop();
        file_ = new File(loop_, filename_, O_RDWR | O_CREAT);
    }

    void TearDown() override
    {
        delete file_;
        delete loop_;
        
        // Remove the test file
        std::remove(filename_.c_str());
    }

    EventLoop* loop_;
    File* file_;
    const std::string filename_ = "test_file.txt";
};

TEST_F(FileTest, ReadFile)
{
    bool readCalled = false;
    std::string readContent;

    file_->setReadCallback([&readCalled, &readContent](const std::string& content) {
        readCalled = true;
        readContent = content;
    });

    file_->read();
    loop_->loop();

    EXPECT_TRUE(readCalled);
    EXPECT_EQ(readContent, "Hello, Muduo!\n");
}

TEST_F(FileTest, WriteFile)
{
    bool writeFinished = false;

    const std::string testData = "This is a test";

    file_->write(testData);
    loop_->loop();

    // 验证写入的内容
    std::ifstream infile(filename_);
    std::string content((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find(testData), std::string::npos);
}

TEST_F(FileTest, CloseFile)
{
    file_->close();

    // 关闭后尝试读取，应该不会触发读取回调
    bool readCalled = false;

    file_->setReadCallback([&readCalled](const std::string&) {
        readCalled = true;
    });

    file_->read();
    loop_->loop();

    EXPECT_FALSE(readCalled);
}