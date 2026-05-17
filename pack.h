#ifndef PACK_H
#define PACK_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "folderCode.h"
#include "AIDecode.h"
#include "LZ77.h"
#define PACK_TEMP_PREFIX "PK"
 // 写入单个文件内容到流
static int writeFileContentToStream(const char*filepath,FILE*out){
    FILE*f=fopen(filepath,"rb");
    if(!f)return-1;
    fseek(f,0,SEEK_END);
    unsigned int len=ftell(f);
    rewind(f);
    char*buf=(char*)malloc(len);
    if(!buf){
        fclose(f);
        return-1;
    }
    fread(buf,1,len,f);
    fwrite(buf,1,len,out);
    free(buf);
    fclose(f);
    return 0;
}
 // 递归遍历目录，将文件信息写入流（路径长度2字节 + 路径 + 内容长度4字节 + 内容）
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
        }
        else{
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
    }
    while(FindNextFile(h,&fd));
    FindClose(h);
    return 0;
}
 // 将分号分隔的源路径（支持通配符）打包成临时文件，返回临时文件路径（调用者需 free）
static char*packSourcesToTemp(const char*sources){
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH,tempPath);
    GetTempFileNameA(tempPath,PACK_TEMP_PREFIX,0,tempPath);
    FILE*temp=fopen(tempPath,"wb");
    if(!temp)return NULL;
    char*srcCopy=_strdup(sources);
    char*token=strtok(srcCopy,";");
    while(token){
        while(*token==' '||*token=='\t')token++;
        char*end=token+strlen(token)-1;
        while(end>token&&(*end==' '||*end=='\t'))end--;
        end[1]='\0';
        if(strlen(token)==0){
            token=strtok(NULL,";");
            continue;
        }
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
                }
                while(FindNextFile(h,&fd));
                FindClose(h);
            }
            else{
                printf("无法访问: %s\n",token);
            }
        }
        else{
            DWORD attrs=GetFileAttributesA(token);
            if(attrs==INVALID_FILE_ATTRIBUTES){
                printf("跳过: %s\n",token);
            }
            else if(attrs&FILE_ATTRIBUTE_DIRECTORY){
                collectFilesToStream(token,temp);
            }
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
    return _strdup(tempPath);
}
 // 打包并压缩（useLz: 1 用混合压缩，0 用纯哈夫曼）
static int packCompress(const char*outFile,const char*sources,int useLz){
    char*tempPack=packSourcesToTemp(sources);
    if(!tempPack){
        printf("打包失败\n");
        return-1;
    }
     // 清空输出文件
    FILE*f=fopen(outFile,"wb");
    if(f)fclose(f);
    else{
        free(tempPack);
        return-1;
    }
    int ret;
    if(useLz)ret=codeFile_LZ((char*)outFile,tempPack,1);
    else ret=codeFile((char*)outFile,tempPack);
    remove(tempPack);
    free(tempPack);
    return ret;
}
 // 解包并解压
static int unpackDecompress(const char*inFile,const char*outDir,int useLz){
    char tempFile[MAX_PATH];
    GetTempPathA(MAX_PATH,tempFile);
    GetTempFileNameA(tempFile,"UN",0,tempFile);
    int ret;
    if(useLz)ret=decodeFile_Hybrid(inFile,tempFile);
    else ret=decodeFile(inFile,tempFile);
    if(ret!=0){
        remove(tempFile);
        return-1;
    }
    FILE*f=fopen(tempFile,"rb");
    if(!f){
        remove(tempFile);
        return-1;
    }
    ensureDirectoryExists(outDir);
    while(1){
        unsigned short pathLen;
        if(fread(&pathLen,2,1,f)!=1)break;
        if(pathLen==0)break;
        char*path=(char*)malloc(pathLen+1);
        if(!path)break;
        fread(path,1,pathLen,f);
        path[pathLen]='\0';
         // 去掉盘符（如 "C:\"）
        char*rel=path;
        if(path[1]==':'&&(path[2]=='\\'||path[2]=='/'))rel=path+3;
        char full[MAX_PATH];
        snprintf(full,MAX_PATH,"%s\\%s",outDir,rel);
        char*last=strrchr(full,'\\');
        if(last){
            *last='\0';
            ensureDirectoryExists(full);
            *last='\\';
        }
        unsigned int contentLen;
        if(fread(&contentLen,4,1,f)!=1){
            free(path);
            break;
        }
        char*content=(char*)malloc(contentLen);
        if(content){
            fread(content,1,contentLen,f);
            FILE*out=fopen(full,"wb");
            if(out){
                fwrite(content,1,contentLen,out);
                fclose(out);
            }
            free(content);
        }
        free(path);
    }
    fclose(f);
    remove(tempFile);
    return 0;
}
#endif // PACK_H
