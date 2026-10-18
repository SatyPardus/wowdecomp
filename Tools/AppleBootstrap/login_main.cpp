#include <cstdio>
#include <exception>
void CheckGameLogin();
int main() {
    try { CheckGameLogin(); return 0; }
    catch (const std::exception& error) { std::fprintf(stderr, "Native login check failed: %s\n", error.what()); return 1; }
}
