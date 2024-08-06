#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <hiredis/hiredis.h>

#define MAX_EVENTS 10
#define READ_BUFFER 1024

void set_nonblocking(int sock) {
    int opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        exit(EXIT_FAILURE);
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr{}, client_addr{};
    socklen_t client_len = sizeof(client_addr);

    // 创建 Redis 客户端
    redisContext *redis_ctx = redisConnect("127.0.0.1", 6379);
    if (redis_ctx == nullptr || redis_ctx->err) {
        if (redis_ctx) {
            std::cerr << "Redis connection error: " << redis_ctx->errstr << std::endl;
            redisFree(redis_ctx);
        } else {
            std::cerr << "Redis connection error: can't allocate redis context" << std::endl;
        }
        exit(EXIT_FAILURE);
    }

    // 创建服务器socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址和端口
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);

    // 绑定socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 设置为非阻塞模式
    set_nonblocking(server_fd);
    // 创建epoll实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // 将服务器socket添加到epoll监听中
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }

    // 开始事件循环
    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                // 处理新连接
                client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }
                set_nonblocking(client_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl: client_fd");
                    exit(EXIT_FAILURE);
                }
                std::cout << "Accepted connection from client." << std::endl;
            } else {
                // 处理客户端发送的数据
                char buffer[READ_BUFFER];
                int bytes_read = read(events[i].data.fd, buffer, READ_BUFFER);
                if (bytes_read <= 0) {
                    // 客户端断开连接
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1) {
                        perror("epoll_ctl: EPOLL_CTL_DEL");
                    }
                    close(events[i].data.fd);
                    std::cout << "Closed connection with client." << std::endl;
                } else {
                    // 显示客户端发送的数据
                    buffer[bytes_read] = '\0';
                    std::string received_message(buffer);

                    // 解析命令
                    if (received_message.find("REGISTER") == 0) {
                        // 格式: REGISTER username password
                        size_t space1 = received_message.find(' ', 8);
                        size_t space2 = received_message.find(' ', space1 + 1);
                        if (space1 != std::string::npos && space2 != std::string::npos) {
                            std::string username = received_message.substr(8, space1 - 8);
                            std::string password = received_message.substr(space1 + 1);
                            std::string redis_key = "user:" + username;

                            // 保存到Redis
                            redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SET %s %s", "msg", "test");
                            if (reply == nullptr) {
                                std::cerr << "Redis command error" << std::endl;
                            } else {
                                std::cout << "Registered user: " << username << std::endl;
                                freeReplyObject(reply);
                            }
                        }
                    } else {
                        // 处理其他类型的消息
                        std::cout << "Received from client: " << buffer << std::endl;
                    }

                    // 回显收到的数据
                    write(events[i].data.fd, buffer, bytes_read);
                }
            }
        }
    }

    redisFree(redis_ctx);
    close(server_fd);
    return 0;
}
