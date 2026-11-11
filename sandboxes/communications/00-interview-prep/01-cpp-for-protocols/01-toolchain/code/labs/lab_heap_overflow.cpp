// LAB A (worksheet section 3.4 / Q8 / Q12). Deliberately buggy - do not "fix" before running it.
// Run:  ./lab_heap_overflow        (n defaults to 8)
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    const int n = argc > 1 ? std::atoi(argv[1]) : 8;
    unsigned char* buf = new unsigned char[8];
    for (int i = 0; i <= n; ++i) {           // BUG: <= writes one past the end when n == 8
        buf[i] = static_cast<unsigned char>(i);
    }
    std::printf("last=%d\n", buf[n]);
    delete[] buf;
    return 0;
}
