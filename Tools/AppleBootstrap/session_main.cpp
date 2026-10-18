#include <cstdio>
#include <exception>
void CheckGameSession();
int main() {
    try { CheckGameSession(); return 0; }
    catch (const std::exception& error) { std::fprintf(stderr, "Native session check failed: %s\n", error.what()); return 1; }
}
