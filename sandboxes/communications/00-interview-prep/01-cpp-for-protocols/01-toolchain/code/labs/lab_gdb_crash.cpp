// LAB D (worksheet section 3.5 / Q10 / Q19). Built WITHOUT sanitizers so you get a plain SIGSEGV.
// Task: use gdb to find which node pointer is null and which line dereferences it.
//   gdb ./lab_gdb_crash        then:  run, bt, frame 1, print n, print *head
#include <cstdio>

struct Node {
    int value;
    Node* next;
};

// Sum values until we see a negative sentinel value.
int sum_list(const Node* n) {
    int total = 0;
    while (n->value >= 0) {   // BUG: the list has no sentinel node; n becomes nullptr after the last node
        total += n->value;
        n = n->next;
    }
    return total;
}

int main() {
    Node c{7, nullptr};
    Node b{5, &c};
    Node a{3, &b};
    std::printf("sum=%d\n", sum_list(&a));
    return 0;
}
