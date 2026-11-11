// LAB C (worksheet section 3.4). Deliberately buggy: LeakSanitizer reports at process exit.
#include <cstdio>

static unsigned char* make_frame_buffer() {
    return new unsigned char[258];
}

int main() {
    unsigned char* p = make_frame_buffer();
    p[0] = 0x7E;
    std::printf("first byte 0x%02X\n", p[0]);
    p = nullptr;   // BUG: last pointer to the allocation is dropped; nobody called delete[]
    return 0;
}
