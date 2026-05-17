#ifndef PACK_H
#define PACK_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<windows.h>
#include"folderCode.h"
#include"AIDecode.h"
#include"LZ77.h"

#define PACK_TEMP_PREFIX "PK"

static int writeFileContentToStream(const char*filepath,FILE*out){
	FILE*f=fopen(filepath,"rb");
	if(!f)return-1;
	fseek(f,0,SEEK_END);
	unsigned int len=ftell(f);
	rewind(f);
	char*buf=(char*)malloc(len);
	if(!buf){fclose(f);return-1;}
	fread(buf,1,len,f);
	fwrite(buf,1,len,out);
	free(buf);
	fclose(f);
	return 0;
}

static int collectFilesToStream(const char*dir,FILE*out){
	char search[MAX_PATH];
	snprintf(search,MAX_PATH,"%s\\*",dir);
	WIN32_FIND_DATA fd;
	HANDLE h=FindFirstFile(search,&fd);
	if(h==INVALID_HANDLE_VALUE)return-1;
	do{
		if(strcmp(fd.cFileName,".")==0||strcmp(fd.cFileName,"..")==0)continue;
		char full[MAX_PATH];
		snprintf(full,MAX_PATH,"%s\\%s",dir,fd.cFileName);
		if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
			collectFilesToStream(full,out);
		}else{
			unsigned short pathLen=(unsigned short)strlen(full);
			fwrite(&pathLen,2,1,out);
			fwrite(full,1,pathLen,out);
			FILE*f=fopen(full,"rb");
			if(!f)continue;
			fseek(f,0,SEEK_END);
			unsigned int contentLen=ftell(f);
			rewind(f);
			fwrite(&contentLen,4,1,out);
			char*buf=(char*)malloc(contentLen);
			if(buf){
				fread(buf,1,contentLen,f);
				fwrite(buf,1,contentLen,out);
				free(buf);
			}
			fclose(f);
		}
	}while(FindNextFile(h,&fd));
	FindClose(h);
	return 0;
}

static char* packSourcesToTemp(const char*sources){
	char fullPath[MAX_PATH]="PACKED_TMP.bin";
	DeleteFileA(fullPath);
	FILE*temp=fopen(fullPath,"wb");
	if(!temp)return NULL;
	char*srcCopy=_strdup(sources);
	char*token=strtok(srcCopy,";");
	while(token){
		while(*token==' '||*token=='\t')token++;
		char*end=token+strlen(token)-1;
		while(end>token&&(*end==' '||*end=='\t'))end--;
		end[1]='\0';
		if(strlen(token)==0){token=strtok(NULL,";");continue;}
		if(strpbrk(token,"*?")){
			WIN32_FIND_DATA fd;
			HANDLE h=FindFirstFile(token,&fd);
			if(h!=INVALID_HANDLE_VALUE){
				char dirPart[MAX_PATH]="";
				char*last=strrchr(token,'\\');
				if(last){
					strncpy(dirPart,token,last-token);
					dirPart[last-token]='\0';
				}
				do{
					if(strcmp(fd.cFileName,".")==0||strcmp(fd.cFileName,"..")==0)continue;
					char full[MAX_PATH];
					if(dirPart[0])snprintf(full,MAX_PATH,"%s\\%s",dirPart,fd.cFileName);
					else strcpy(full,fd.cFileName);
					unsigned short pathLen=(unsigned short)strlen(full);
					fwrite(&pathLen,2,1,temp);
					fwrite(full,1,pathLen,temp);
					writeFileContentToStream(full,temp);
				}while(FindNextFile(h,&fd));
				FindClose(h);
			}else printf("无法访问: %s\n",token);
		}else{
			DWORD attrs=GetFileAttributesA(token);
			if(attrs==INVALID_FILE_ATTRIBUTES)printf("跳过: %s\n",token);
			else if(attrs&FILE_ATTRIBUTE_DIRECTORY)collectFilesToStream(token,temp);
			else{
				unsigned short pathLen=(unsigned short)strlen(token);
				fwrite(&pathLen,2,1,temp);
				fwrite(token,1,pathLen,temp);
				writeFileContentToStream(token,temp);
			}
		}
		token=strtok(NULL,";");
	}
	free(srcCopy);
	unsigned short end=0;
	fwrite(&end,2,1,temp);
	fclose(temp);
	return _strdup(fullPath);
}

static int packCompress(const char*outFile,const char*sources,int useLz){
	char*tempPack=packSourcesToTemp(sources);
	if(!tempPack){printf("打包失败\n");return-1;}
	FILE*f=fopen(outFile,"wb");
	if(f)fclose(f);
	else{free(tempPack);return-1;}
	int ret;
	if(useLz)ret=codeFile_LZ((char*)outFile,tempPack,1);
	else ret=codeFile((char*)outFile,tempPack);
	remove(tempPack);
	free(tempPack);
	return ret;
}

static int unpackDecompress(const char*inFile,const char*outDir,int useLz){
	int ret;
	if(useLz)ret=decodeFile_Hybrid(inFile,outDir);
	else ret=decodeFile(inFile,outDir);
	if(ret!=0)return-1;
	char packedFile[MAX_PATH];
	snprintf(packedFile,MAX_PATH,"%s\\PACKED_TMP.bin",outDir);
	FILE*f=fopen(packedFile,"rb");
	if(!f){printf("未找到打包数据文件 %s\n",packedFile);return-1;}
	fseek(f,0,SEEK_END);
	long dataLen=ftell(f);
	rewind(f);
	char*data=(char*)malloc(dataLen);
	if(!data){fclose(f);return-1;}
	fread(data,1,dataLen,f);
	fclose(f);
	DeleteFileA(packedFile);
	ensureDirectoryExists(outDir);
	char*p=data;
	char*end=data+dataLen;
	while(p+2<=end){
		unsigned short pathLen=*(unsigned short*)p;
		p+=2;
		if(pathLen==0)break;
		if(p+pathLen>end)break;
		char*path=(char*)malloc(pathLen+1);
		memcpy(path,p,pathLen);
		path[pathLen]='\0';
		p+=pathLen;
		if(p+4>end){free(path);break;}
		unsigned int contentLen=*(unsigned int*)p;
		p+=4;
		if(p+contentLen>end){free(path);break;}
		char*rel=path;
		if(path[1]==':'&&(path[2]=='\\'||path[2]=='/'))rel=path+3;
		char full[MAX_PATH];
		snprintf(full,MAX_PATH,"%s\\%s",outDir,rel);
		char*lastSlash=strrchr(full,'\\');
		if(lastSlash){
			*lastSlash='\0';
			ensureDirectoryExists(full);
			*lastSlash='\\';
		}
		FILE*out=fopen(full,"wb");
		if(out){
			fwrite(p,1,contentLen,out);
			fclose(out);
		}
		free(path);
		p+=contentLen;
	}
	free(data);
	return 0;
}

#endif
