#include <cstdio>
#include <exception>
void CheckGameRealm();
int main() {
    try { CheckGameRealm(); return 0; }
    catch (const std::exception& error) { std::fprintf(stderr, "Native realm check failed: %s\n", error.what()); return 1; }
}
