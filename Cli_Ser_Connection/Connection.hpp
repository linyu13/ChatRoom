#include "../include/headFile.hpp"

class Sen
{ 
public:
    ssize_t writen(int fd, const char *buf, size_t len);
    void sendToCli(int fd, const std::string &buf);
    void sendStatusOfInt(int fd, int status);
    void sendStatusOfSize_t(int fd, size_t status);
};

// 发送指定长度的数据到文件描述符
ssize_t Sen::writen(int fd, const char *buf, size_t len)
{
    const char *ptr = buf;  // 指向当前发送数据的位置
    size_t remaining = len; // 剩余待发送的字节数

    while (remaining > 0)
    {
        ssize_t sent = send(fd, ptr, remaining, 0);
        if (sent == -1)
        {
            if (errno == EINTR)
            {
                continue; // 如果被信号中断，继续尝试发送
            }
            std::cerr << "Error in writen: " << strerror(errno) << std::endl;
            return -1; // 发送失败，返回-1
        }
        ptr += sent;        // 移动指针到已发送部分的后面
        remaining -= sent;  // 更新剩余待发送字节数
    }
    return len; // 返回实际发送的字节数
}

// 发送一个以长度为前缀的字符串数据
void Sen::sendToCli(int fd, const std::string &buf)
{
    if (fd < 0 || buf.empty())
    {
        return; // 文件描述符无效或字符串为空，不进行发送
    }

    uint32_t len = htonl(static_cast<uint32_t>(buf.size())); // 将长度转为网络字节序
    std::vector<char> data(sizeof(len) + buf.size()); // 创建一个存放长度和内容的缓冲区
    std::memcpy(data.data(), &len, sizeof(len)); // 复制长度到缓冲区
    std::memcpy(data.data() + sizeof(len), buf.data(), buf.size()); // 复制字符串内容到缓冲区

    if (writen(fd, data.data(), data.size()) == -1)
    {
        std::cerr << "Failed to send message" << std::endl;
        close(fd); // 发送失败，关闭文件描述符
    }
}

// 发送整数状态码
void Sen::sendStatusOfInt(int fd, int status)
{
    if (send(fd, &status, sizeof(status), 0) == -1)
    {
        std::cerr << "Error in send_status (int): " << strerror(errno) << std::endl;
        close(fd); // 发送失败，关闭文件描述符
    }
}

// 发送 size_t 类型状态码
void Sen::sendStatusOfSize_t(int fd, size_t status)
{
    if (send(fd, &status, sizeof(status), 0) == -1)
    {
        std::cerr << "Error in send_status (size_t): " << strerror(errno) << std::endl;
        close(fd); // 发送失败，关闭文件描述符
    }
}

class Rec
{
public:
    ssize_t readBuf(int fd, char *buf, size_t len);
    int recvToCil(int fd, std::string &buf);
    int recvStatusOfInt(int fd);
    size_t recvStatusOfSize_t(int fd);
};

// 从文件描述符读取指定长度的数据
ssize_t Rec::readBuf(int fd, char *buf, size_t len)
{
    char *ptr = buf;  // 指向当前读取数据的位置
    size_t remaining = len; // 剩余待读取的字节数

    while (remaining > 0)
    {
        ssize_t received = recv(fd, ptr, remaining, 0);
        if (received == -1)
        {
            if (errno == EINTR)
            {
                continue; // 如果被信号中断，继续尝试读取
            }
            std::cerr << "Error in readn: " << strerror(errno) << std::endl;
            close(fd); // 读取失败，关闭文件描述符
            return -1;
        }
        else if (received == 0)
        {
            return len - remaining; // 连接关闭，返回已读取字节数
        }
        ptr += received;       // 移动指针到已读取部分的后面
        remaining -= received; // 减少剩余待读取字节数
    }
    return len - remaining; // 返回实际读取的字节数
}

// 接收一个带长度前缀的字符串数据
int Rec::recvToCil(int fd, std::string &buf)
{
    uint32_t len = 0;
    if (readBuf(fd, reinterpret_cast<char*>(&len), sizeof(len)) != sizeof(len))
    {
        std::cerr << "Failed to read message length" << std::endl;
        return -1; // 读取长度失败
    }

    len = ntohl(len); // 将长度从网络字节序转换为主机字节序
    std::vector<char> data(len); // 创建一个缓冲区用于存储数据

    ssize_t received = readBuf(fd, data.data(), len);
    if (received != static_cast<ssize_t>(len))
    {
        std::cerr << "Incomplete message received" << std::endl;
        return -1; // 数据接收不完整
    }

    buf.assign(data.begin(), data.end()); // 将接收到的数据赋值给字符串
    return received; // 返回实际接收到的字节数
}

// 接收整数类型的状态码
int Rec::recvStatusOfInt(int fd)
{
    int status = 0;
    ssize_t received = recv(fd, &status, sizeof(status), 0);
    if (received == -1)
    {
        std::cerr << "Error in recv_status: " << strerror(errno) << std::endl;
    }
    else if (received == 0)
    {
        std::cerr << "Connection closed by peer" << std::endl;
        close(fd); // 连接关闭，关闭文件描述符
    }
    return status; // 返回接收到的状态码
}

// 接收 size_t 类型的状态码
size_t Rec::recvStatusOfSize_t(int fd)
{
    size_t status = 0;
    ssize_t received = recv(fd, &status, sizeof(status), 0);
    if (received == -1)
    {
        std::cerr << "Error in recv_status_long: " << strerror(errno) << std::endl;
    }
    else if (received == 0)
    {
        std::cerr << "Connection closed by peer" << std::endl;
    }
    return status; // 返回接收到的状态码
}
