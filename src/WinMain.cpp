#if defined(_WIN32)
#include <Windows.h>

extern int main();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main();
}
#endif