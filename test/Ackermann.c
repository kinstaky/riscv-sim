#include "lib.h"

int ack(int m,int n){
	while(m!=0){
		if( n==0) n=1;
		else{
			n=ack(m, n-1);}
			m--;
		}
	return n+1;
}

int main() {
    print_d(ack(3, 8));
    return 0;
}