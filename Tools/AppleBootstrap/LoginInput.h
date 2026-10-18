#pragma once
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>

struct NativeLoginInput {
    std::string account, password;
    void Clear() {
        auto* bytes = reinterpret_cast<volatile char*>(password.data());
        for (size_t i = 0; i < password.size(); ++i) bytes[i] = 0;
        password.clear();
    }
    ~NativeLoginInput() { Clear(); }
    void Read() {
        std::fputs("Account: ", stderr);
        if (!std::getline(std::cin, account) || account.empty() || account.size() > 127)
            throw std::runtime_error("Missing or oversized account");
        std::fputs("Password: ", stderr);
        struct Echo {
            termios saved{}; bool restore = false;
            Echo() {
                if (!isatty(STDIN_FILENO)) return;
                if (tcgetattr(STDIN_FILENO, &saved)) throw std::runtime_error("Cannot protect password input");
                auto hidden = saved; hidden.c_lflag &= ~ECHO;
                if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &hidden)) throw std::runtime_error("Cannot protect password input");
                restore = true;
            }
            ~Echo() { if (restore) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved); std::fputc('\n', stderr); } }
        } echo;
        if (!std::getline(std::cin, password) || password.empty() || password.size() > 128)
            throw std::runtime_error("Missing or oversized password");
    }
};
