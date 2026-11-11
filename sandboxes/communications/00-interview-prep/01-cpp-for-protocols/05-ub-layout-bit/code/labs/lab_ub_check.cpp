// LAB A (worksheet 3.1 / Q8). Deliberate UB: "detect overflow by testing x + 1 < x".
// Build gives lab_ub_check_O0 and lab_ub_check_O2. Run both with NO arguments (argc == 1 -> x == INT_MAX).
// Typical GCC/Clang result:  -O0 prints 1 (the add wraps at run time)   -O2 prints 0 (compiler folds x+1<x to false).
// Neither result is guaranteed: signed overflow is undefined behaviour.
#include <climits>
#include <cstdio>

static bool overflows(int x) {
    return x + 1 < x;  // BUG: relies on signed wrap-around
}

int main(int argc, char**) {
    int x = INT_MAX - 1 + argc;
    std::printf("overflows(%d) = %d\n", x, overflows(x) ? 1 : 0);
    return 0;
}
