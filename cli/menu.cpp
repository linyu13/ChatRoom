#include "menu.hpp"
#include "../include/headFile.hpp"

void Menu::ShowMenu() {
    const char* text = "There is Menu";
    char command[256];
    snprintf(command, sizeof(command), "echo \"%s\" | figlet | boxes -d c | lolcat", text);
    int result = system(command);
}

void Menu::LoginMenu() {
    const char* text = "L O G I N";
    char command[256];
    snprintf(command, sizeof(command), "echo \"%s\" | figlet | boxes -d c | lolcat", text);
    int result = system(command);
}

void Menu::EnrollMenu() {
    const char* text = "E N R O L L";
    char command[256];
    snprintf(command, sizeof(command), "echo \"%s\" | figlet | boxes -d c | lolcat", text);
    int result = system(command);
}

void Menu::LogoutMenu() {
    const char* text = "L O G O U T";
    char command[256];
    snprintf(command, sizeof(command), "echo \"%s\" | figlet | boxes -d c | lolcat", text);
    int result = system(command);
}

void Menu::ExitMenu() {
    const char* text = "E X I T";
    char command[256];
    snprintf(command, sizeof(command), "echo \"%s\" | figlet | boxes -d c | lolcat", text);
    int result = system(command);
}