#include <cstdio>
#include <exception>
void CheckGameNetwork();
int main() {
    try { CheckGameNetwork(); return 0; }
    catch (const std::exception& error) { std::fprintf(stderr, "Native network check failed: %s\n", error.what()); return 1; }
}
