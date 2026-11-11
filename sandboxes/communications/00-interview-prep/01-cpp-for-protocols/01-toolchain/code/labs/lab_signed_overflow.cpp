// LAB B (worksheet section 3.4 / Q9). Deliberately buggy.
// UBSan reports: "signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'"
#include <climits>
#include <cstdio>

int main(int argc, char**) {
    int x = INT_MAX - 1 + argc;   // argc == 1 when run with no arguments -> INT_MAX
    int y = x + 1;                // BUG: signed overflow is undefined behaviour
    std::printf("y=%d\n", y);
    return 0;
}
