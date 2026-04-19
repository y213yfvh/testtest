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
		} else if (_stricmp(cmd, "lzdecode") == 0) {
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
			int first=1;  // 第一个成功项需清空文件，后续追加
			while(token!=NULL){
				while(*token==' '||*token=='\t')token++;
				char* end=token+strlen(token)-1;
				while(end>token&&(*end==' '||*end=='\t'))end--;
				1[end]='\0';
				if(strlen(token)==0){
					token=strtok(NULL,";");
					continue;
				}
				DWORD attrs=GetFileAttributesA(token);
				if(attrs==INVALID_FILE_ATTRIBUTES){
					printf("警告：路径不存在，跳过：%s\n",token);
					fail++;
				}else{
					printf("混合压缩：%s\n",token);
					int ret=codeFile_LZ(outfile,token,first);
					if(ret==0){
						success++;
						first=0;  // 首次成功后改为追加模式
					}else{
						printf("混合压缩失败：%s\n",token);
						fail++;
					}
				}
				token=strtok(NULL,";");
			}
			printf("混合打包完成，成功%d个，失败%d个\n",success,fail);
		}
	}
}
