#include "../include/headFile.hpp"
#include "client.hpp"

#define PORT 8080

int main(int argc, char **argv)
{
    // 禁用EOF
    // 可以考虑直接忽略 EOF
    // 目的: 防止EOF中断输入流   确保终端输入稳定
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) == -1)
    {
        std::cerr << "获取终端属性失败: " << strerror(errno) << std::endl;
        return 1;
    }

    term.c_cc[VEOF] = _POSIX_VDISABLE;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == -1)
    {
        std::cerr << "设置终端属性失败: " << strerror(errno) << std::endl;
        return 1;
    }

    // 禁用 ctrl + c / z / \

    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    std::string serverAddress = "127.0.0.1"; // 默认本机地址

    int port = PORT; // 默认端口

    if (argc >= 2) // 自定义服务器地址
    {
        serverAddress = argv[1];
    }

    if (argc >= 3) // 自定义端口
    {
        try
        {
            port = std::stoi(argv[2]);
            if (port <= 0 || port > 65535)
            {
                std::cerr << "端口号超出有效范围 (1-65535): " << argv[2] << std::endl;
                return 1;
            }
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "无效的端口号: " << argv[2] << std::endl;
            return 1;
        }
    }
    try
    {
        Client myClient(serverAddress, port);
        myClient.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}
