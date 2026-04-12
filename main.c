#include<stdio.h>
#include<stdlib.h>
#include<windows.h>
#include<string.h>
#include<direct.h>
#include"code.h"
int main(){
	int weight[256]={0};
	/*char s[120];
	scanf("%s",s);
	countChar(s,weight);
	char* code[256]={0};
	RETcode(weight,code);
	for(int i=0;i<256;i++){
		if(code[i])printf("%d ",i);
		puts(code[i]);
	}
	writeFile("out.huf",code,s,weight);*/
	char path[MAX_PATH];
	char cmd[100];
	char in[MAX_PATH+100];
	char arg[MAX_PATH];
	if (_getcwd(path, MAX_PATH)==NULL){
		puts("获取当前目录失败");
		return -1;
	}
	while(1){
		printf("%s> ",path);
		if(fgets(in,sizeof(in),stdin)==NULL){
			break;
		}
		in[strcspn(in,"\n")]='\0';
		char* p=in;
		while(*p==' '||*p=='\t'){//\t->tab
			p++;
		}
		if(*p=='\0')continue;//写的时候漏了个=，导致总是输出原目录
		int n=sscanf(p,"%99s %255[^\n]",cmd,arg);
		if(n<1){//[^\n]读取直到遇到\n
			continue;
		}
		if(_stricmp(cmd,"exit")==0||_stricmp(cmd,"quit")==0){
			break;
		}
		if(_stricmp(cmd,"cd")==0){
			if(n<2)continue;
			if(strlen(arg)==0){
				continue;
			}
			if(SetCurrentDirectoryA(arg)){//设置当前工作目录
				if(_getcwd(path,MAX_PATH)==NULL){
					puts("无法获取当前目录");
				}
			}else{
				DWORD error=GetLastError();//错误处理，错误处理这部分是AI写的
				printf("无法切换到目录 \"%s\"。错误码: %lu\n", arg, error);
				if(error==ERROR_FILE_NOT_FOUND){
					printf("原因：目录不存在。\n");
				}
				else if(error==ERROR_ACCESS_DENIED){
					printf("原因：访问被拒绝。\n");
				}
			}
		}else if(_stricmp(cmd,"")){
			
		}
	}
}
