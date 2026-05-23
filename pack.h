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

//文件打包

//如其名，把文件内容写入输出文件中
static int writeFileContentToStream(const char* filepath,FILE* out){
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

//收集dir下的所有文件到输出文件中
static int collectFilesToStream(const char* dir,FILE* out){
	char search[MAX_PATH];
	snprintf(search,MAX_PATH,"%s\\*",dir);//拼接通配符，找目录下所有文件/目录
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
			unsigned short pathLen=(unsigned short)strlen(full);//路径字符串长度
			fwrite(&pathLen,2,1,out);//把长度写入输出流
			fwrite(full,1,pathLen,out);//把路径字符串写入（没有串结束符\0）
			FILE* f=fopen(full,"rb");
			if(!f)continue;
			fseek(f,0,SEEK_END);//到文件末尾
			unsigned int contentLen=ftell(f);//计算文件大小
			rewind(f);
			fwrite(&contentLen,4,1,out);//把大小写入
			char buf[65536];//本来是动态分配，但是可能太大了，改成固定大小缓冲区
			size_t n;//容纳对象尺寸的类型，应该是8字节
			while((n=fread(buf,1,sizeof(buf),f))>0){
			    fwrite(buf,1,n,out);//写入
			}
			fclose(f);
		}
	}while(FindNextFile(h,&fd));
	FindClose(h);
	return 0;
}

//把资源打包去文件PACKED_TMP.bin
static char* packSourcesToTemp(const char* sources){
	char fullPath[MAX_PATH]="PACKED_TMP.bin";
	DeleteFileA(fullPath);//删掉重名文件
	FILE* temp=fopen(fullPath,"wb");
	if(!temp)return NULL;
	char* srcCopy=_strdup(sources);//拷贝一份路径名
	char* token=strtok(srcCopy,";");//分号分隔路径名
	while(token){
		while(*token==' '||*token=='\t')token++;//跳过空格和\t
		char* end=token+strlen(token)-1;//末尾指针
		while(end>token&&(*end==' '||*end=='\t'))end--;//去除末尾的空格和\t
		1[end]='\0';//等价于end[1]='\0'
		if(strlen(token)==0){token=strtok(NULL,";");continue;}
		for(char *p=token;*p;p++)if(*p=='/')*p='\\';//把/换成、
		if(strpbrk(token,"*?")){//通配符处理
			WIN32_FIND_DATA fd;
			HANDLE h=FindFirstFile(token,&fd);
			if(h!=INVALID_HANDLE_VALUE){
				char dirPart[MAX_PATH]="";
				char* last=strrchr(token,'\\');//找最右边的斜杠，从而获取目录名
				if(last){
					strncpy(dirPart,token,last-token);
					dirPart[last-token]='\0';
				}
				do{
					if(strcmp(fd.cFileName,".")==0||strcmp(fd.cFileName,"..")==0)continue;
					char full[MAX_PATH];
					if(dirPart[0])snprintf(full,MAX_PATH,"%s\\%s",dirPart,fd.cFileName);
					else strcpy(full,fd.cFileName);
					for(char *p=full;*p;p++)if(*p=='/')*p = '\\';
					unsigned short pathLen=(unsigned short)strlen(full);
					//同样的写入路径名长度和路径名以及内容
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
				//同样...
				unsigned short pathLen=(unsigned short)strlen(token);
				fwrite(&pathLen,2,1,temp);
				fwrite(token,1,pathLen,temp);
				writeFileContentToStream(token,temp);
			}
		}
		//到分号分隔的下一个
		token=strtok(NULL,";");
	}
	free(srcCopy);
	unsigned short end=0;
	fwrite(&end,2,1,temp);
	fclose(temp);
	return _strdup(fullPath);//返回路径的副本
}


//主打包函数
static int packCompress(const char* outFile,const char* sources,int useLz){
	char* tempPack=packSourcesToTemp(sources);
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


//解包函数
static int unpackDecompress(const char* inFile,const char* outDir,int useLz){
	int ret;
	if(useLz)ret=decodeFile_Hybrid(inFile,outDir);//解压得到打包后文件
	else ret=decodeFile(inFile,outDir);
	if(ret!=0)return-1;//错误
	char packedFile[MAX_PATH];
	snprintf(packedFile,MAX_PATH,"%s\\PACKED_TMP.bin",outDir);
	FILE* f=fopen(packedFile,"rb");
	if(!f){printf("未找到打包数据文件 %s\n",packedFile);return-1;}
	fseek(f,0,SEEK_END);
	long dataLen=ftell(f);//获取长度
	rewind(f);//回去
	char* data=(char*)malloc(dataLen);
	if(!data){fclose(f);return-1;}
	fread(data,1,dataLen,f);
	fclose(f);
	DeleteFileA(packedFile);
	ensureDirectoryExists(outDir);//创建目录
	char* p=data;
	char* end=data+dataLen;
	while(p+2<=end){
		unsigned short pathLen=*(unsigned short*)p;//指针强转，取出路径长度，默认小端
		p+=2;//移动指针，跳过读取的长度
		if(pathLen==0)break;
		if(p+pathLen>end)break;
		char* path=(char*)malloc(pathLen+1);
		memcpy(path,p,pathLen);
		path[pathLen]='\0';
		p+=pathLen;
		if(p+4>end){free(path);break;}
		unsigned int contentLen=*(unsigned int*)p;//取出文件大小
		p+=4;
		if(p+contentLen>end){free(path);break;}
		char* rel=path;
		if(path[1]==':'&&(path[2]=='\\'||path[2]=='/'))rel=path+3;
		char full[MAX_PATH];
		snprintf(full,MAX_PATH,"%s\\%s",outDir,rel);
		char* lastSlash=strrchr(full,'\\');
		if(lastSlash){
			*lastSlash='\0';//确认路径存在
			ensureDirectoryExists(full);
			*lastSlash='\\';//恢复、
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
