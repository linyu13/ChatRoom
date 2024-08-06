#include "../include/headFile.hpp"
#include "../include/define.hpp"
#include "menu.hpp"


class Users : public Menu
{
public:
    void ClientEcho();

    void Enroll(int fd);

    void Login(int fd);

    void Logout(int fd);

    void Exit(int fd);

    int sendCode(const std::string To);

    int creatCode(int length);

    std::string CreatID();
};