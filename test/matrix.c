#include "lib.h"

#define x 64
#define y 64
#define z 64

int a[x][y];
int b[y][z];
int c[x][z];


void Mul(){
	for (int i = 0; i != x; ++i) {
		for (int j = 0; j != y; ++j) {
			c[i][j] = 0;
		}
	}
	for (int i = 0; i != x; ++i) {
		for (int j = 0; j != y; ++j) {
			for (int k = 0; k != z; ++k) {
				c[i][k] += a[i][j] * b[j][k];
			}
		}
	}
}

int main() {
	for (int i = 0; i != x; ++i) {
		for (int j = 0; j != y; ++j) {
			a[i][j] = i + j;
		}
	}
	for (int i = 0; i != y; ++i) {
		for (int j = 0; j != z; ++j) {
			b[i][j] = i + j;
		}
	}
    Mul();
    print_d(c[32][15]);
    return 0;
}