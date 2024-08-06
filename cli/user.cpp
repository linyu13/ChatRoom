#include "user.hpp"
#include "../Cli&Ser_Connection/Connection.hpp"


void Users::ClientEcho()
{
}
void Users::Enroll(int fd)
{
    std::string userName;
    std::string password;
    std::string Email;
    std::string telephoneNumber;

    std::cout << "请输入你的昵称" << std::endl;
    std::cin >> userName;
    std::cout << "请输入你的密码" << std::endl;
    std::cin >> password;
    std::cout << "请输入你的邮箱" << std::endl;
    std::cin >> Email;
    std::cout << "请输入你的电话" << std::endl;
    std::cin >> telephoneNumber;

    std::string To = "<" + Email + ">";
    int code = Users::sendCode(To);
    std::string userCodeStr;
    int userCodeNum;
    std::cout << "验证码已发送, 请输入验证码" << std::endl;
tryAgain:
    std::cin >> userCodeStr;
    try
    {
        userCodeNum = std::stoi(userCodeStr);
        if (userCodeNum <= 99999 || userCodeNum > 999999)
        {
            std::cout << "验证码为六位数字, 请重新操作" << std::endl;
            goto tryAgain;
        }
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "验证码输入错误, 请重新操作" << std::endl;
        goto tryAgain;
    }
    if (userCodeNum == code)
    {
        nlohmann::json EnrollRequest = {{"username", userName},
                                        {"password", password},
                                        {"Email", Email},
                                        {"telephoneNumber", telephoneNumber}};
        Sen s;
        s.sendToCli(fd, EnrollRequest.dump());

        Rec r;
        int status = r.recvStatusOfInt(fd);
        std::cout << "The status is" << status << std::endl;

        if (status == SUCCESS)
        {
            std::cout << "注册成功" << std::endl;

            std::string ID;


        }
    }
}

void Users::Login(int fd)
{
    std::string ID;
    std::cout << "" << std::endl;
}
