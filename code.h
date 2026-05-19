#ifndef CODE_H
#define CODE_H

#include<stdio.h>
#include"codeMan.h"

//往文件里面写入数据的头，本来应该和folderCode.h合在一起的

//对文件里面的字符计数
void countChar(char* fileName,unsigned int weight[]){//weight[256]!!!
	FILE* fp;
	fp=fopen(fileName,"rb");
	if(fp==NULL){
		printf("打开目标文件错误\n");
		return;
	}
	int t;
	while((t=fgetc(fp))!=EOF){
		weight[t]++;
	}
	fclose(fp);
}

//传入一个字符串（01串），写入一字节
void bitwrite(FILE* fl,char u8[]){//char u8[8]!!!
	unsigned char ch=0;
	for(int i=0;i<8;i++){
		ch=(ch<<1)|(u8[i]-'0');
	}
	if(fwrite(&ch,sizeof(char),1,fl)!=1){
		printf("写入错误\n");
		return;
	}
}
//字符队列
struct strque{
	char s[514];
	unsigned short begin;
	unsigned short size;
};
typedef struct strque strque;
//清空队列
inline void clearStrque(strque* sq){
	sq->begin=0;
	sq->size=0;
}
//弹出8长度的字符串
inline void popStrque(strque* sq,char u8[]){//u8[8]!!!
	for(short i=0;i<8;i++){
		u8[i]=sq->s[(sq->begin+i)%514];
	}
	sq->size-=8;
	sq->begin=(sq->begin+8)%514;
}
//推入字符串
inline void pushStrque(strque* sq,char s[],short slen){
	for(short i=0;i<slen;i++){
		sq->s[(sq->begin+sq->size+i)%514]=s[i];
	}
	sq->size+=slen;
}
//写入
long long writeFile(FILE* wfp,char* code[],FILE* rfp,unsigned int weight[]){//weight[256],code[256]!!!
	short cha=0;
	for(int i=0;i<256;i++){
		if(weight[i]){
			cha++;
		}
	}
	if(fwrite(&cha,sizeof(short),1,wfp)!=1){//写入（字符+权值）的总数
		printf("写入错误\n");
		return -1;
	}
	for(int i=0;i<256;i++){
		if(weight[i]){
			unsigned char ii=i;
			if(fwrite(&ii,sizeof(unsigned char),1,wfp)!=1){//写入字符
				printf("写入错误\n");
				return -1;
			}
			if(fwrite(&weight[i],sizeof(unsigned int),1,wfp)!=1){//写入对应权值
				printf("写入错误\n");
				return -1;
			}
		}
	}
	int t;
	long long fllen=0;
	while((t=fgetc(rfp))!=EOF){
		fllen+=strlen(code[t]);//低效率的获取编码后文件内容长度
	}
	if(fwrite(&fllen,sizeof(fllen),1,wfp)!=1){
		printf("写入错误\n");
		return -1;
	}
	rewind(rfp);//让文件指针回去
	strque buffer;//字符队列缓冲区
	clearStrque(&buffer);//清空
	char u8[8];
	while((t=fgetc(rfp))!=EOF){
		pushStrque(&buffer,code[t],strlen(code[t]));
		while(buffer.size>=8){
			popStrque(&buffer,u8);
			bitwrite(wfp,u8);//转换后写入
		}
	}
	if(buffer.size>0){
		for(int i=0;i<buffer.size;i++){
			u8[i]=buffer.s[(buffer.begin+i)%514];
		}
		for(int i=buffer.size;i<8;i++){
			u8[i]='0';//末尾补0
		}
		bitwrite(wfp,u8);
	}
	return fllen;//返回文件长度
}
#endif
