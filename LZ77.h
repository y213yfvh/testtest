#ifndef LZ77_H
#define LZ77_H
#include<stdio.h>
#include<windows.h>
#include<string.h>
struct window{
	char win[32768];
	unsigned int begin;
};
typedef struct window window;
inline void clearWindow(window* p){
	p->begin=0;
	memset(p->win,0,32768);
}
void pushWindow(window* p,char ch){
	p->win[(p->begin)&32767]=ch;
	p->begin++;
}
char getWindowChar(const window* p,unsigned int curPos,unsigned int dist){
	unsigned int index=(curPos-dist)&32767;
	return p->win[index];
}
static inline unsigned int hash3(char* p){
	return (p[0]*123456)^(p[1]*4567)^(p[2]*789);
}
int LZ77(char* rFilePath){
	FILE* rfp=NULL;
	FILE* wfp=NULL;
	rfp=fopen(rFilePath,"rb");
	if(rfp==NULL){
		printf("读取：打开文件错误。\n");
		return -1;
	}
	char wFilePath[MAX_PATH];
	strcpy(wFilePath,rFilePath);
	strcat(wFilePath,".mylz");
	wfp=fopen(wFilePath,"wb");
	if(wfp==NULL){
		printf("写入：打开文件错误。\n");
		return -1;
	}
	char a[3]={0};
	unsigned int cur=0;
	unsigned int head[32768]={0};
	unsigned int prev[32768]={0};
	window W;
	clearWindow(&W);
	char ch;
	fseek()
	while((ch=fgetc(rfp))!=EOF){
		if(cur>2){
			a[0]=a[1];
			a[1]=a[2];
			a[2]=ch;
			pushWindow(&W,ch);
			short h=(a[0]*123456)^(a[1]*789)^(a[2]*4567);
			h&=32767;
			;
		}else{
			a[cur]=ch;
			cur++;
		}
	}
	return 0;
}
#endif
