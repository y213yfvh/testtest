#ifndef FOLDERCODE_H
#define FOLDERCODE_H

#include<windows.h>
#include<stdio.h>
#include"code.h"

//文件压缩写入

//普通写入（文件）
int normalCode(FILE* wfp,char* rFilePath){
	FILE* rfp;
	if(wfp==NULL){
		printf("无法打开编码文件\n");
		return -1;
	}
	rfp=fopen(rFilePath,"rb");
	if(rfp==NULL){
		printf("无法打开原文件\n");
		return -1;
	}
	char t=0;
	if(fwrite(&t,sizeof(t),1,wfp)!=1){//标记是文件而不是目录
		printf("写入错误\n");
		return -1;
	}
	int len=strlen(rFilePath);
	if(fwrite(&len,sizeof(len),1,wfp)!=1){
		printf("写入错误\n");
		return -1;
	}
	if(fwrite(rFilePath,sizeof(char),len,wfp)!=len){
		printf("写入错误\n");
		return -1;
	}//长的错误处理
	char* code[256]={0};
	unsigned int weight[256]={0};
	countChar(rFilePath,weight);
	RETcode(weight,code);
	writeFile(wfp,code,rfp,weight);
	fclose(rfp);
	freeCode(code);
	return 0;
}
//目录写入，其实也能正常处理文件路径
int directoryCode(FILE* wfp,char* rFilePath){
	if(wfp==NULL){
		printf("无法打开编码文件\n");
		return -1;
	}
	DWORD attri=GetFileAttributesA(rFilePath);
	if(attri==INVALID_FILE_ATTRIBUTES){
		puts("路径无效或者不存在");
		return -1;
	}
	char t=1;
	if(fwrite(&t,sizeof(t),1,wfp)!=1){//标记目录开头
		printf("写入错误\n");
		return -1;
	}
	int len=strlen(rFilePath);
	if(fwrite(&len,sizeof(len),1,wfp)!=1){
		printf("写入错误\n");
		return -1;
	}
	if(fwrite(rFilePath,sizeof(char),len,wfp)!=len){
		printf("写入错误\n");
		return -1;
	}
	if(attri&FILE_ATTRIBUTE_DIRECTORY){//是目录
		char searchPath[MAX_PATH];
		snprintf(searchPath,MAX_PATH,"%s\\*",rFilePath);//拼接通配符，从而遍历目录下面所有文件和目录
		WIN32_FIND_DATA findData;
		HANDLE hFind;
		hFind=FindFirstFile(searchPath,&findData);//句柄
		if(hFind==INVALID_HANDLE_VALUE){
			printf("无法访问目录：%s\n",rFilePath);
			return -1;
		}
		do{
			if(strcmp(findData.cFileName,".")==0||strcmp(findData.cFileName,"..")==0){
				continue;//跳过自身和上级，避免死循环
			}
			char fullPath[MAX_PATH];
			snprintf(fullPath,MAX_PATH,"%s\\%s",rFilePath,findData.cFileName);//拼接路径和文件（或文件夹）名
			if(findData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
				if(directoryCode(wfp,fullPath)<0){//递归
					FindClose(hFind);
					return -1;
				}
			}else{
				if(normalCode(wfp,fullPath)<0){//文件压缩
					FindClose(hFind);
					return -1;
				}
			}
		}while(FindNextFile(hFind,&findData));//下一个
		FindClose(hFind);
	}else{
		if(normalCode(wfp,rFilePath)<0)return -1;
	}
	t=2;
	if(fwrite(&t,sizeof(t),1,wfp)!=1){//标记目录结束
		printf("写入错误\n");
		return -1;
	}
	return 0;
}
int codeFile(char* wFilePath,char* rFilePath){//综合的文件写入
	FILE* wfp;
	wfp=fopen(wFilePath,"ab");
	DWORD attri=GetFileAttributesA(rFilePath);
	if(attri==INVALID_FILE_ATTRIBUTES){
		puts("路径无效或者不存在");
		return 1;
	}
	if(attri&FILE_ATTRIBUTE_DIRECTORY){
		if(directoryCode(wfp,rFilePath))return -1;
	}else{
		if(normalCode(wfp,rFilePath)<0)return -1;
	}
	fclose(wfp);
	return 0;
}
#endif
