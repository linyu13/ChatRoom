#include "../include/headFile.hpp"
#include "menu.hpp"
#include "user.hpp"


class Socket
{
public:
    Socket() : fd(-1) {}
    explicit Socket(int fd) : fd(fd) {}
    ~Socket()
    {
        if (fd != -1)
            close(fd);
    }
    int get() const { return fd; }
    void set(int new_fd) { fd = new_fd; }

private:
    int fd;
};

class Client : public Menu
{
public:
    Client(const std::string &serverAddress, int port)
        : serverAddress(serverAddress), socketPtr(std::make_unique<Socket>())
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        {
            // 套接字创建失败
            std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
            throw std::runtime_error("Socket creation failed");
        }
        socketPtr->set(fd);

        struct sockaddr_in server
        {
        };
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        if (inet_pton(AF_INET, serverAddress.c_str(), &server.sin_addr) <= 0)
        {
            // 地址转化失败
            std::cerr << "Invalid address not supported: " << strerror(errno) << std::endl;
            throw std::invalid_argument("Invalid server address");
        }

        if (connect(socketPtr->get(), (struct sockaddr *)&server, sizeof(server)) == -1)
        {
            // 连接失败
            std::cerr << "Connextion failed: " << strerror(errno) << std::endl;
            throw std::runtime_error("Connection failed");
        }
        std::cout << "Connected to server " << serverAddress << ":" << port << std::endl;

        startReceiveThread();
    }

    ~Client()
    {
        stopReceiveThread();
    }

    void run()
    {
        try
        {
            std::string selectStr = "";
            do
            {
                this->ShowMenu();
                std::cin >> selectStr;

                int select = 0;
                try
                {
                    select = std::stoi(selectStr);
                }
                catch (const std::invalid_argument &e)
                {
                    // 字符串无法转化为整数
                    std::cerr << "Invalid input, please enter a number." << std::endl;
                    continue;
                }
                catch (const std::out_of_range &e)
                {
                    // 转化后的输入超过了 int 范围
                    std::cerr << "Input number is out of range, please try again." << std::endl;
                    continue;
                }

                // 成功后进行操作
                if (select < 0 || select > 4)
                {
                    // 输入超出选项
                    std::cerr << "Invalid select, please try again." << std::endl;
                    continue;
                }

                switch (select)
                {
                case 1:
                    system("clear");
                    this->LoginMenu();
                    this->myUser.Login(socketPtr->get());
                    break;

                case 2:
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    system("clear");
                    this->EnrollMenu();
                    this->myUser.Enroll(socketPtr->get());
                    break;

                case 3:
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    system("clear");
                    this->LogoutMenu();
                    this->myUser.Logout(socketPtr->get());
                    break;

                case 4:
                    system("clear");
                    this->ExitMenu();
                    this->myUser.Exit(socketPtr->get());
                    select = 0;
                    break;

                default:
                    system("clear");
                    std::cout << "Invalid select" << std::endl;
                    break;
                }
            } while (true);
        }
        catch (const std::exception &e)
        {
            // 运行时捕获异常
            std::cerr << "Exception caught during run-time: " << e.what() << std::endl;
        }
    }

private:
    void startReceiveThread()
    {
        stopThread = false;
        receiveThread = std::thread(&Client::receiveMessages, this);
    }

    void stopReceiveThread()
    {
        stopThread = true;
        if (receiveThread.joinable())
        {
            receiveThread.join();
        }
    }

    void receiveMessages()
    {
        while (stopThread)
        {
            char buffer[1024];
            ssize_t bytesReceived = recv(socketPtr->get(), buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived > 0)
            {
                buffer[bytesReceived] = '\0';
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    message.push(std::string(buffer));
                }
                condVar.notify_one();
            }
            else if (bytesReceived == 0)
            {
                // 服务器关闭连接
                std::cerr << "Server closed the connection" << std::endl;
                stopThread = true;
            }
            else
            {
                std::cerr << "recv failed: " << strerror(errno) << std::endl;
                stopThread = true;
            }
        }
    }

    // void processMessage

    Users myUser;
    std::string serverAddress;
    std::unique_ptr<Socket> socketPtr;

    std::thread receiveThread;
    std::atomic<bool> stopThread{false};
    std::mutex mutex;
    std::condition_variable condVar;
    std::queue<std::string> message;
};