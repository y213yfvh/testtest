#include<stdio.h>
#include"codeMan.h"
#ifndef CODE_H
#define CODE_H
//read txt
void countChar(char* fileName,int weight[]){//weight[256]!!!
	FILE* fp;
	fp=fopen(fileName,"rb");
	if(fp==NULL){
		printf("打开目标文件错误");
		return;
	}
	int t;
	while((t=fgetc(fp))!=EOF){
		weight[t]++;
	}
	fclose(fp);
}
void Decode(char* rFileName,char* wFileName){
	
}
//write
inline void bitwrite(FILE* fl,char u8[]){//char u8[8]!!!
	unsigned char ch=0;
	for(int i=0;i<8;i++){
		ch=(ch<<1)|(u8[i]-'0');
	}
	fwrite(&ch,sizeof(char),1,fl);
}
struct strque{
	char s[514];
	unsigned short begin;
	unsigned short size;
};
typedef struct strque strque;
inline void clearStrque(strque* sq){
	sq->begin=0;
	sq->size=0;
}
inline void popStrque(strque* sq,char u8[]){//u8[8]!!!
	for(short i=0;i<8;i++){
		u8[i]=sq->s[(sq->begin+i)%514];
	}
	sq->size-=8;
	sq->begin=(sq->begin+8)%514;
}
inline void pushStrque(strque* sq,char s[],short slen){
	for(short i=0;i<slen;i++){
		sq->s[(sq->begin+sq->size+i)%514]=s[i];
	}
	sq->size+=slen;
}
long long writeFile(char* wFileName,char* code[],char* rFileName,int weight[]){//weight[256],code[256]!!!
	FILE* wfp;
	FILE* rfp;
	wfp=fopen(wFileName,"ab");
	if(wfp==NULL){
		printf("无法打开编码文件");
		return -1;
	}
	rfp=fopen(rFileName,"rb");
	if(rfp==NULL){
		fclose(wfp);
		printf("无法打开原文件");
		return -1;
	}
	short cha=0;
	for(int i=0;i<256;i++){
		if(weight[i]){
			cha++;
		}
	}
	fwrite(&cha,sizeof(short),1,wfp);
	for(int i=0;i<256;i++){
		if(weight[i]){
			unsigned char ii=i;
			fwrite(&ii,sizeof(unsigned char),1,wfp);
			fwrite(&weight[i],sizeof(int),1,wfp);
		}
	}
	fseek(rfp,0,SEEK_END);
	long long fllen = ftell(rfp);
	fwrite(&fllen,sizeof(fllen),1,wfp);
	rewind(rfp);
	int t;
	strque buffer;
	clearStrque(&buffer);
	char u8[8];
	while((t=fgetc(rfp))!=EOF){
		pushStrque(&buffer,code[t],strlen(code[t]));
		while(buffer.size>=8){
			popStrque(&buffer,u8);
			bitwrite(wfp,u8);
		}
	}
	if(buffer.size>0){
		for(int i=0;i<buffer.size;i++){
			u8[i]=buffer.s[(buffer.begin+i)%514];
		}
		for(int i=buffer.size;i<8;i++){
			u8[i]='0';
		}
		bitwrite(wfp,u8);
	}
	fclose(wfp);
	fclose(rfp);
	return fllen;
}
#endif
