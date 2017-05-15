#include <stdio.h>

int max(int x, int y) {
	return x > y ? x : y;
}

int main() {
	printf("max is %d\n", max(123, 321));
	printf("test many arguments %d %d %d %s %s %s %s %s %s\n", 1, 2, 3, "a", "b", "c", "d", "e", "f");
	return 100;
}
