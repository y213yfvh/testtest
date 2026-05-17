#include<stdio.h>
#include<stdlib.h>
#include<windows.h>
#include<string.h>
#include<direct.h>
#include"pack.h"
#define CMDLINE_MAX 4096
static inline void fixPath(char* p){
    for(;*p;p++)if(*p=='/')*p='\\';
}
 // 这是命令处理函数，太大了
static int processCommand(const char *cmdline,char *path){
    char cmd[100];
    char in[CMDLINE_MAX+100];
    char arg[CMDLINE_MAX];
    strcpy(in,cmdline);
    in[strcspn(in,"\n")]='\0';
    char*p=in;
    while(*p==' '||*p=='\t')p++;
    if(*p=='\0')return 0;
    int n=sscanf(p,"%99s %4095[^\n]",cmd,arg);
    if(n<1)return 0;
    if(_stricmp(cmd,"exit")==0||_stricmp(cmd,"quit")==0){
        return 1;
    }
    if(_stricmp(cmd,"cd")==0){
        if(n<2)return 0;
        if(SetCurrentDirectoryA(arg)){
            if(_getcwd(path,MAX_PATH)==NULL)puts("无法获取当前目录");
        }
        else{
            DWORD error=GetLastError();
            printf("无法切换到目录 \"%s\"。错误码: %lu\n",arg,error);
            if(error==ERROR_FILE_NOT_FOUND)printf("原因：目录不存在。\n");
            else if(error==ERROR_ACCESS_DENIED)printf("原因：访问被拒绝。\n");
        }
        return 0;
    }
    if(_stricmp(cmd,"cls")==0){
        HANDLE hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
        if(hConsole==INVALID_HANDLE_VALUE)return 0;
        CONSOLE_SCREEN_BUFFER_INFO CSBI;
        if(!GetConsoleScreenBufferInfo(hConsole,&CSBI))return 0;
        DWORD cells=CSBI.dwSize.X*CSBI.dwSize.Y;
        COORD startCoord={0,0};
        DWORD written;
        FillConsoleOutputCharacter(hConsole,' ',cells,startCoord,&written);
        SetConsoleCursorPosition(hConsole,startCoord);
        return 0;
    }
    if(_stricmp(cmd,"dir")==0){
        char pathp[MAX_PATH];
        snprintf(pathp,MAX_PATH,"%s\\*",path);
        WIN32_FIND_DATA findData;
        HANDLE hFind;
        hFind=FindFirstFile(pathp,&findData);
        if(hFind==INVALID_HANDLE_VALUE){
            printf("无法访问当前目录\n");
            return 0;
        }
        do{
            printf("%s",findData.cFileName);
            if(findData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)printf("(目录)");
            puts("");
        }
        while(FindNextFile(hFind,&findData));
        FindClose(hFind);
        return 0;
    }
    if(_stricmp(cmd,"huff")==0){
        if(n<2)return 0;
        char path1[MAX_PATH];
        char path2[MAX_PATH];
        int nn=sscanf(arg,"%s %s",path1,path2);
        if(nn==1){
            fixPath(path1);
            puts("未输入输出路径，默认在本路径下输出out.huf");
            FILE*f=fopen("out.huf","wb");
            if(f)fclose(f);
            else{
                printf("错误，无法创建输出文件\n");
                return 0;
            }
            codeFile("out.huf",path1);
        }
        else{
            fixPath(path1);
            fixPath(path2);
            printf("输出位置：%s\n",path2);
            FILE*f=fopen(path2,"wb");
            if(f)fclose(f);
            else{
                printf("错误，无法创建输出文件\n");
                return 0;
            }
            codeFile(path2,path1);
        }
        return 0;
    }
    if(_stricmp(cmd,"decode")==0){
        if(n<2)return 0;
        char path1[MAX_PATH];
        char path2[MAX_PATH];
        int nn=sscanf(arg,"%s %s",path1,path2);
        if(nn==1){
            fixPath(path1);
            puts("未输入输出路径，默认输出到 ./output");
            decodeFile(path1,"output");
        }
        else{
            fixPath(path1);
            fixPath(path2);
            printf("输出位置：%s\n",path2);
            decodeFile(path1,path2);
        }
        return 0;
    }
    if(_stricmp(cmd,"pack")==0){
        char outFile[MAX_PATH],remaining[CMDLINE_MAX];
        if(sscanf(arg,"%s %[^\n]",outFile,remaining)<1){
            printf("错误，无法解析输出文件名\n");
            return 0;
        }
        if(packCompress(outFile,remaining,0)==0)printf("打包压缩完成\n");
        else printf("打包压缩失败\n");
        return 0;
    }
    if(_stricmp(cmd,"lzpack")==0){
        char outFile[MAX_PATH],remaining[CMDLINE_MAX];
        if(sscanf(arg,"%s %[^\n]",outFile,remaining)<1){
            printf("错误，无法解析输出文件名\n");
            return 0;
        }
        if(packCompress(outFile,remaining,1)==0)printf("混合打包压缩完成\n");
        else printf("混合打包压缩失败\n");
        return 0;
    }
    if(_stricmp(cmd,"unpack")==0){
        if(n<2)return 0;
        char path1[MAX_PATH],path2[MAX_PATH];
        int nn=sscanf(arg,"%s %s",path1,path2);
        const char*out=(nn==1)?".":path2;
        if(unpackDecompress(path1,out,0)==0)printf("解包完成\n");
        else printf("解包失败\n");
        return 0;
    }
    if(_stricmp(cmd,"lzunpack")==0){
        if(n<2)return 0;
        char path1[MAX_PATH],path2[MAX_PATH];
        int nn=sscanf(arg,"%s %s",path1,path2);
        const char*out=(nn==1)?".":path2;
        if(unpackDecompress(path1,out,1)==0)printf("混合解包完成\n");
        else printf("混合解包失败\n");
        return 0;
    }
    if(_stricmp(cmd,"lz")==0){
        if(n<2)return 0;
        char path1[MAX_PATH],path2[MAX_PATH];
        int nn=sscanf(arg,"%s %s",path1,path2);
        if(nn==1){
            fixPath(path1);
            puts("未输入输出路径，默认在本路径下输出 out.dflar");
            FILE*f=fopen("out.dflar","wb");
            if(f)fclose(f);
            else{
                printf("错误，无法创建输出文件\n");
                return 0;
            }
            codeFile_LZ("out.dflar",path1,1);
        }
        else{
            fixPath(path1);
            fixPath(path2);
            printf("输出位置：%s\n",path2);
            FILE*f=fopen(path2,"wb");
            if(f)fclose(f);
            else{
                printf("错误，无法创建输出文件\n");
                return 0;
            }
            codeFile_LZ(path2,path1,1);
        }
        return 0;
    }
    if(_stricmp(cmd,"lzdecode")==0){
        if(n<2)return 0;
        char path1[MAX_PATH],path2[MAX_PATH];
        int nn=sscanf(arg,"%s %s",path1,path2);
        if(nn==1){
            fixPath(path1);
            puts("未输入输出路径，默认输出到 ./output");
            decodeFile_Hybrid(path1,"output");
        }
        else{
            fixPath(path1);
            fixPath(path2);
            printf("输出位置：%s\n",path2);
            decodeFile_Hybrid(path1,path2);
        }
        return 0;
    }
    if(strcmp(cmd,"?")==0){
        printf("cd <目录>                      切换工作目录\n");
        printf("exit / quit                    退出程序\n");
        printf("cls                            清空屏幕\n");
        printf("dir                            列出当前目录内容，.是自身，..是上一级目录\n");
        printf("huff <源> [输出文件]           哈夫曼压缩文件/文件夹，默认 out.huf\n");
        printf("decode <压缩文件> [输出目录]   哈夫曼解压，默认当前目录\n");
        printf("lz <源> [输出文件]             LZ77+哈夫曼压缩，默认 out.dflar\n");
        printf("lzdecode <压缩文件> [输出目录] LZ77+哈夫曼解压，默认当前目录\n");
        printf("pack <输出> <源1;源2;...>      哈夫曼打包（分号分隔）\n");
        printf("lzpack <输出> <源1;源2;...>    LZ77+哈夫曼打包（分号分隔）\n");
        printf("\n注意:\n");
        printf("  - 压缩目录时自动递归处理子目录\n");
        printf("  - 解压时输出目录不存在会自动创建\n");
        printf("  - 打包时路径中不能包含分号\n");
        printf("  - 打包时可以使用通配符\n");
        printf("\n示例:\n");
        printf("  huff doc.txt archive.huf\n");
        printf("  lzdecode out.dflar restored\n");
        printf("  pack bundle.huf C:\\Folder;file.txt\n");
        printf("\n");
        return 0;
    }
    printf("未知命令: %s\n",cmd);
    return 0;
}
 // 主函数
int main(int argc,char* argv[]){
    char path[MAX_PATH];
    if(_getcwd(path,MAX_PATH)==NULL){
        puts("获取当前目录失败");
        return-1;
    }
    if(argc>1){
        char cmdline[CMDLINE_MAX+100]={0};
        for(int i=1;i<argc;i++){
            strcat(cmdline,argv[i]);
            if(i<argc-1)strcat(cmdline," ");
        }
        processCommand(cmdline,path);
        return 0;
    }
     // 交互模式
    printf("输入?获取指令大全。\n");
    printf("注意是半角的?。\n");
    printf("注意输入文件名要完整，带后缀。\n");
    char in[CMDLINE_MAX+100];
    while(1){
        printf("%s> ",path);
        if(fgets(in,sizeof(in),stdin)==NULL)break;
        if(processCommand(in,path))break;
    }
    return 0;
}
