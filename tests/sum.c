/*
 * A simple problem for testing.
 */
#include <stdio.h>

int main(void) {
	int sum = 0;
	for (int i = 0; i <= 100; ++i) {
		sum += i;
	}
	printf("sum is %d\n", sum);
	return 0;
}
