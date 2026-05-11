#include<stdio.h>
#include<stdlib.h>
#include<windows.h>
#include<string.h>
#include<direct.h>
#include"folderCode.h"
#include"AIDecode.h"
#include"LZ77.h"
int main(){
	/*int weight[256]={0};
	char s[120];
	scanf("%s",s);
	countChar(s,weight);
	char* code[256]={0};
	RETcode(weight,code);
	for(int i=0;i<256;i++){
		if(code[i])printf("%d ",i);
		puts(code[i]);
	}
	writeFile("out.huf",code,s,weight);*/
	char path[MAX_PATH];
	char cmd[100];
	#define CMDLINE_MAX 4096
	char in[CMDLINE_MAX+100];
	char arg[CMDLINE_MAX];
	if (_getcwd(path, MAX_PATH)==NULL){
		puts("获取当前目录失败");
		return -1;
	}
	printf("输入?获取指令大全。\n");
	printf("注意是半角的?。\n");
	printf("注意输入文件名要完整，带后缀。\n");
	while(1){
		printf("%s> ",path);
		if(fgets(in,sizeof(in),stdin)==NULL){
			break;
		}
		in[strcspn(in,"\n")]='\0';
		char* p=in;
		while(*p==' '||*p=='\t'){//\t->tab
			p++;
		}
		if(*p=='\0')continue;//写的时候漏了个=，导致总是输出原目录
		int n=sscanf(p,"%99s %4095[^\n]",cmd,arg);
		if(n<1){//[^\n]读取直到遇到\n
			continue;
		}
		if(_stricmp(cmd,"exit")==0||_stricmp(cmd,"quit")==0){
			break;
		}
		if(_stricmp(cmd,"cd")==0){
			if(n<2)continue;
			if(SetCurrentDirectoryA(arg)){//设置当前工作目录
				if(_getcwd(path,MAX_PATH)==NULL){
					puts("无法获取当前目录");
				}
			}else{
				DWORD error=GetLastError();//错误处理，错误处理这部分是AI写的
				printf("无法切换到目录 \"%s\"。错误码: %lu\n", arg, error);
				if(error==ERROR_FILE_NOT_FOUND){
					printf("原因：目录不存在。\n");
				}
				else if(error==ERROR_ACCESS_DENIED){
					printf("原因：访问被拒绝。\n");
				}
			}
		}else if(_stricmp(cmd,"cls")==0){
			HANDLE hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
			if(hConsole==INVALID_HANDLE_VALUE){
				continue;
			}
			CONSOLE_SCREEN_BUFFER_INFO CSBI;
			if(!GetConsoleScreenBufferInfo(hConsole,&CSBI)){
				continue;
			}
			DWORD cells=CSBI.dwSize.X*CSBI.dwSize.Y;//unsigned int
			COORD startCoord={0,0};
			DWORD written;
			FillConsoleOutputCharacter(hConsole,' ',cells,startCoord,&written);
			SetConsoleCursorPosition(hConsole,startCoord);
		}else if(_stricmp(cmd,"dir")==0){
			char pathp[MAX_PATH];
			snprintf(pathp,MAX_PATH,"%s\\*",path);
			WIN32_FIND_DATA findData;
			HANDLE hFind;
			hFind=FindFirstFile(pathp,&findData);
			if(hFind==INVALID_HANDLE_VALUE){
				printf("无法访问当前目录\n");
				continue;
			}
			do{
				printf("%s",findData.cFileName);
				if(findData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
					printf("(目录)");
				}
				puts("");
			}while(FindNextFile(hFind,&findData));
			FindClose(hFind);
		}else if(_stricmp(cmd,"huff")==0){
			if(n<2)continue;
			char path1[MAX_PATH];
			char path2[MAX_PATH];
			int nn=sscanf(arg,"%s %s",path1,path2);
			if(nn==1){
				puts("未输入输出路径，默认在本路径下输出out.huf");
				FILE* f=fopen("out.huf","wb");
				if(f){
					fclose(f);
				}else{
					printf("错误，无法创建输出文件\n");
					continue;
				}
				codeFile("out.huf",path1);
			}else{
				printf("输出位置：%s\n",path2);
				FILE* f=fopen(path2,"wb");
				if(f){
					fclose(f);
				}else{
					printf("错误，无法创建输出文件\n");
					continue;
				}
				codeFile(path2,path1);
			}
		}else if(_stricmp(cmd,"decode")==0){
			if(n<2)continue;
			char path1[MAX_PATH];
			char path2[MAX_PATH];
			int nn=sscanf(arg,"%s %s",path1,path2);
			if(nn==1){
				puts("未输入输出路径，默认输出到 ./output");
				decodeFile(path1,"output");
			}else{
				printf("输出位置：%s\n",path2);
				decodeFile(path1,path2);
			}
		}else if(_stricmp(cmd,"pack")==0){
			char outfile[MAX_PATH];
			char remaining[CMDLINE_MAX];
			if(sscanf(arg,"%s %[^\n]",outfile,remaining)<1){
				printf("错误，无法解析输出文件名\n");
				continue;
			}
			FILE* f=fopen(outfile,"wb");
			if(f){
				fclose(f);
			}else{
				printf("错误，无法创建输出文件\n");
				continue;
			}
			char* token=strtok(remaining,";");
			int success=0,fail=0;
			while(token!=NULL){
				while(*token==' '||*token=='\t')token++;
				char* end=token+strlen(token)-1;
				while(end>token&&(*end==' '||*end=='\t'))end--;
				1[end]='\0';//整活
				if(strlen(token)==0){
					token=strtok(NULL,";");
					continue;
				}
				if(strpbrk(token,"*?")){
					WIN32_FIND_DATA findData;
					HANDLE hFind;
					hFind=FindFirstFile(token,&findData);
					if(hFind==INVALID_HANDLE_VALUE){
						printf("无法访问:%s\n",token);
						fail++;
						continue;
					}
					do{
						if(strcmp(findData.cFileName,".")==0||strcmp(findData.cFileName,"..")==0)continue;
						char fullpath[MAX_PATH];
						char* last=strrchr(token,'\\');
						if(last==NULL)last=strrchr(token,'/');
						if(last!=NULL){
							int dirlen=last-token;//神秘的指针减法
							strncpy(fullpath,token,dirlen);
							fullpath[dirlen]='\0';
							snprintf(fullpath+dirlen,MAX_PATH-dirlen,"\\%s",findData.cFileName);
						}else{
							strcpy(fullpath,findData.cFileName);
						}
						printf("压缩：%s\n",fullpath);
						int ret=codeFile(outfile,fullpath);
						if(ret==0)success++;
						else{
							printf("压缩失败：%s\n",fullpath);
							fail++;
						}
					}while(FindNextFile(hFind,&findData));
					FindClose(hFind);
				}else{
					DWORD attrs=GetFileAttributesA(token);
					if(attrs==INVALID_FILE_ATTRIBUTES){
						printf("警告：路径不存在，跳过：%s\n",token);
						fail++;
					}else{
						printf("压缩：%s\n",token);
						int ret=codeFile(outfile,token);
						if(ret==0)success++;
						else{
							printf("压缩失败：%s\n",token);
							fail++;
						}
					}
				}
				token=strtok(NULL,";");
			}
			printf("打包完成，成功%d个，失败%d个\n",success,fail);
		}else if(_stricmp(cmd,"lz")==0){
			if(n<2) continue;
			char path1[MAX_PATH];
		    char path2[MAX_PATH];
			int nn = sscanf(arg, "%s %s", path1, path2);
		    if(nn == 1){
		        puts("未输入输出路径，默认在本路径下输出 out.dflar");
				FILE* f = fopen("out.dflar", "wb");
		        if(f) fclose(f);
		        else{
		        	printf("错误，无法创建输出文件\n");
		            continue;
		        }
		        codeFile_LZ("out.dflar", path1, 1); // clear=1 表示覆盖写入
		    }else{
		        printf("输出位置：%s\n", path2);
		        FILE* f = fopen(path2, "wb");
		        if(f) fclose(f);
		        else{
		            printf("错误，无法创建输出文件\n");
		            continue;
		        }
		        codeFile_LZ(path2, path1, 1);
		    }
		    // 新增：LZ77+哈夫曼混合解压
		}else if(_stricmp(cmd, "lzdecode") == 0) {
		    if (n < 2) continue;
		    char path1[MAX_PATH], path2[MAX_PATH];
		    int nn = sscanf(arg, "%s %s", path1, path2);
		    if (nn == 1) {
		        puts("未输入输出路径，默认输出到 ./output");
		        decodeFile_Hybrid(path1, "output");
		    } else {
		        printf("输出位置：%s\n", path2);
		        decodeFile_Hybrid(path1, path2);
		    }
		}else if(_stricmp(cmd,"lzpack")==0){
			char outfile[MAX_PATH];
			char remaining[CMDLINE_MAX];
			if(sscanf(arg,"%s %[^\n]",outfile,remaining)<1){
				printf("错误，无法解析输出文件名\n");
				continue;
			}
			FILE* f=fopen(outfile,"wb");
			if(f){
				fclose(f);
			}else{
				printf("错误，无法创建输出文件\n");
				continue;
			}
			char* token=strtok(remaining,";");
			int success=0,fail=0;
			while(token!=NULL){
				while(*token==' '||*token=='\t')token++;
				char* end=token+strlen(token)-1;
				while(end>token&&(*end==' '||*end=='\t'))end--;
				1[end]='\0';//整活
				if(strlen(token)==0){
					token=strtok(NULL,";");
					continue;
				}
				if(strpbrk(token,"*?")){
					WIN32_FIND_DATA findData;
					HANDLE hFind;
					hFind=FindFirstFile(token,&findData);
					if(hFind==INVALID_HANDLE_VALUE){
						printf("无法访问:%s\n",token);
						fail++;
						continue;
					}
					do{
						if(strcmp(findData.cFileName,".")==0||strcmp(findData.cFileName,"..")==0)continue;
						char fullpath[MAX_PATH];
						char* last=strrchr(token,'\\');
						if(last==NULL)last=strrchr(token,'/');//又没考虑这个，导致不能正常压缩，为什么要/\混用啊
						if(last!=NULL){
							int dirlen=last-token;//神秘的指针减法
							strncpy(fullpath,token,dirlen);
							fullpath[dirlen]='\0';
							snprintf(fullpath+dirlen,MAX_PATH-dirlen,"\\%s",findData.cFileName);
						}else{
							strcpy(fullpath,findData.cFileName);
						}
						printf("压缩：%s\n",fullpath);
						int ret=codeFile_LZ(outfile,fullpath,0);
						if(ret==0)success++;
						else{
							printf("压缩失败：%s\n",fullpath);
							fail++;
						}
					}while(FindNextFile(hFind,&findData));
					FindClose(hFind);
				}else{
					DWORD attrs=GetFileAttributesA(token);
					if(attrs==INVALID_FILE_ATTRIBUTES){
						printf("警告：路径不存在，跳过：%s\n",token);
						fail++;
					}else{
						printf("压缩：%s\n",token);
						int ret=codeFile_LZ(outfile,token,0);
						if(ret==0)success++;
						else{
							printf("压缩失败：%s\n",token);
							fail++;
						}
					}
				}
				token=strtok(NULL,";");
			}
			printf("打包完成，成功%d个，失败%d个\n",success,fail);
		}else if(strcmp(cmd,"?")==0){
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
		}
	}
}
