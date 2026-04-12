#ifndef FOLDERCODE_H
#define FOLDERCODE_H
#include<windows.h>
#include<stdio.h>
#include"code.h"
void normalCode(char* wFilePath,char* rFilePath,char* code[],int weight[]){
	FILE* wfp;
	FILE* rfp;
	wfp=fopen(wFilePath,"ab");
	if(wfp==NULL){
		printf("无法打开编码文件");
		return;
	}
	rfp=fopen(rFilePath,"rb");
	if(rfp==NULL){
		fclose(wfp);
		printf("无法打开原文件");
		return;
	}
	char t=0;
	fwrite(&t,sizeof(t),1,wfp);
	writeFile(wfp,code,rfp,weight);
	fclose(wfp);
	fclose(rfp);
}
void directoryCode(char* wFilePath,char* rFilePath,char* code[],int weight[]){
	printf("还没写\n");
}
int codeFile(char* wFilePath,char* rFilePath,char* code[],int weight[]){
	DWORD attri=GetFileAttributesA(rFilePath);
	if(attri==INVALID_FILE_ATTRIBUTES){
		puts("路径无效或者不存在");
		return 1;
	}
	if(attri&FILE_ATTRIBUTE_DIRECTORY){
		directoryCode(wFilePath,rFilePath,code,weight);
	}else{
		normalCode(wFilePath,rFilePath,code,weight);
	}
	return 0;
}
#endif
