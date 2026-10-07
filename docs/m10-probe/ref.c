#include <stdio.h>
int add(int a, int b) {
    int s = a + b;
    return s * 2;
}
int main(void) {
    int x = 20;
    int y = add(x, 1);
    printf("%d\n", y);
    return 0;
}
