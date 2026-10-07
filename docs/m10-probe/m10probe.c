int g = 7;
int add(int a, int b) {
    int s = a + b;
    return s * 2;
}
int printf(const char *, ...);
int main(void) {
    int x = 20;
    int y = add(x, 1);
    printf("%d\n", y + g);
    return 0;
}
