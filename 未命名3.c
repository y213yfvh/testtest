#include<stdio.h>
#include<stdlib.h>
#include"codeMan.h"
int main(){
	int a[256]={1,1,4,5,1,4,1,9,1,9,8,1,0};
	char* code[256]={0};
	RETcode(a,code);
	for(int i=0;i<256;i++){
		puts(code[i]);
		if(code[i]==NULL)break;
	}
}
