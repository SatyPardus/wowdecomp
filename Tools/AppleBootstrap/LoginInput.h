#pragma once
#include <cstdio>
#include <cerrno>
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
    void Read(int descriptor = STDIN_FILENO) {
        auto line = [&](std::string& output, size_t limit) {
            output.clear();
            for (;;) {
                char ch; const auto count = read(descriptor, &ch, 1);
                if (count < 0 && errno == EINTR) continue;
                if (!count) return !output.empty();
                if (count != 1) return false;
                if (ch == '\n') return !output.empty();
                if (output.size() == limit) return false;
                output.push_back(ch);
            }
        };
        std::fputs("Account: ", stderr);
        if (!line(account, 127))
            throw std::runtime_error("Missing or oversized account");
        std::fputs("Password: ", stderr);
        struct Echo {
            termios saved{}; bool restore = false; int descriptor;
            explicit Echo(int fd) : descriptor(fd) {
                if (!isatty(descriptor)) return;
                if (tcgetattr(descriptor, &saved)) throw std::runtime_error("Cannot protect password input");
                auto hidden = saved; hidden.c_lflag &= ~ECHO;
                if (tcsetattr(descriptor, TCSAFLUSH, &hidden)) throw std::runtime_error("Cannot protect password input");
                restore = true;
            }
            ~Echo() { if (restore) { tcsetattr(descriptor, TCSAFLUSH, &saved); std::fputc('\n', stderr); } }
        } echo(descriptor);
        if (!line(password, 128))
            throw std::runtime_error("Missing or oversized password");
    }
};
