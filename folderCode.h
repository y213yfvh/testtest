#ifndef FOLDERCODE_H
#define FOLDERCODE_H
#include<windows.h>
#include<stdio.h>
#include"code.h"
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
	if(fwrite(&t,sizeof(t),1,wfp)!=1){
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
	char* code[256]={0};
	int weight[256]={0};
	countChar(rFilePath,weight);
	RETcode(weight,code);
	writeFile(wfp,code,rfp,weight);
	fclose(rfp);
	freeCode(code);
	return 0;
}
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
	if(fwrite(&t,sizeof(t),1,wfp)!=1){
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
	if(attri&FILE_ATTRIBUTE_DIRECTORY){
		char searchPath[MAX_PATH];
		snprintf(searchPath,MAX_PATH,"%s\\*",rFilePath);//居然支持通配符，早知道用这个了
		WIN32_FIND_DATA findData;
		HANDLE hFind;
		hFind=FindFirstFile(searchPath,&findData);
		if(hFind==INVALID_HANDLE_VALUE){
			printf("无法访问目录：%s\n",rFilePath);
			return -1;
		}
		do{
			if(strcmp(findData.cFileName,".")==0||strcmp(findData.cFileName,"..")==0){
				continue;
			}
			char fullPath[MAX_PATH];
			snprintf(fullPath,MAX_PATH,"%s\\%s",rFilePath,findData.cFileName);
			if(findData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
				if(directoryCode(wfp,fullPath)<0){
					FindClose(hFind);
					return -1;
				}
			}else{
				if(normalCode(wfp,fullPath)<0){
					FindClose(hFind);
					return -1;
				}
			}
		}while(FindNextFile(hFind,&findData));
		FindClose(hFind);
	}else{
		if(normalCode(wfp,rFilePath)<0)return -1;
	}
	t=2;
	if(fwrite(&t,sizeof(t),1,wfp)!=1){
		printf("写入错误\n");
		return -1;
	}
	return 0;
}
int codeFile(char* wFilePath,char* rFilePath){
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
