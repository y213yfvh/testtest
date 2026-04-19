#ifndef LZ77_H
#define LZ77_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<windows.h>
#include<stdbool.h>
#include"folderCode.h"

#define WSIZE 32768
#define HSIZE 32768
#define MIN_MATCH 3
#define MAX_MATCH 258
#define MAX_CHAIN 256
#define LA_SIZE (MAX_MATCH+3)

struct window{
    unsigned char win[WSIZE];
    unsigned int begin;
};
typedef struct window window;

struct LZ77State{
    FILE* rfp;
    FILE* wfp;
    window win;
    unsigned int head[HSIZE];
    unsigned int prev[WSIZE];
    unsigned char la[LA_SIZE];
    int la_len;
    unsigned int curPos;
};
typedef struct LZ77State LZ77State;

inline void clearWindow(window* p){
    p->begin=0;
    memset(p->win,0,WSIZE);
}

void pushWindow(window* p,char ch){
    p->win[(p->begin)&(WSIZE-1)]=ch;
    p->begin++;
}

char getWindowChar(const window* p,unsigned int curPos,unsigned int dist){
    unsigned int index=(curPos-dist)&(WSIZE-1);
    return p->win[index];
}

static inline unsigned int hash3(const unsigned char* a){
    unsigned int h=(a[0]*123456)^(a[1]*789)^(a[2]*4567);
    return h&(HSIZE-1);
}

static void fill_la(LZ77State* st,int need){
    while(st->la_len<need){
        int c=fgetc(st->rfp);
        if(c==EOF)break;
        st->la[st->la_len++]=(unsigned char)c;
    }
}

static void consume(LZ77State* st, int n) {
    for (int i = 0; i < n; i++) {
        pushWindow(&st->win, st->la[i]);
        st->curPos++;
        if (st->curPos >= 3) {
            unsigned int pos = st->curPos - 3;
            unsigned char tmp[3];
            tmp[0] = st->win.win[(pos)     & (WSIZE - 1)];
            tmp[1] = st->win.win[(pos + 1) & (WSIZE - 1)];
            tmp[2] = st->win.win[(pos + 2) & (WSIZE - 1)];
            unsigned int h = hash3(tmp);
            unsigned int idx = pos & (WSIZE - 1);
            st->prev[idx] = st->head[h];
            st->head[h] = pos;
        }
    }
    memmove(st->la, st->la + n, st->la_len - n);
    st->la_len -= n;
}

static unsigned int find_match(LZ77State* st,unsigned int* dist){
    if(st->la_len<MIN_MATCH)return 0;
    unsigned int h=hash3(st->la);
    unsigned int best_len=0;
    unsigned int best_pos=0;
    int steps=0;
    for(unsigned int p=st->head[h];p&&steps<MAX_CHAIN;p=st->prev[p&(WSIZE-1)],steps++){
        if(st->curPos-p>WSIZE)continue;
        unsigned int len=0;
        while(len<MAX_MATCH&&len<st->la_len&&p+len<st->curPos){
			unsigned char a=getWindowChar(&st->win,p+len,0);
			unsigned char b=st->la[len];
			if(a!=b)break;
            len++;
        }
        if(len>best_len){
            best_len=len;
            best_pos=p;
            if(best_len>=MAX_MATCH)break;
        }
    }
    if(best_len>=MIN_MATCH){
        *dist=st->curPos-best_pos;
        return best_len;
    }
    return 0;
}

// LZ77 压缩单个文件，生成 .mylz
int LZ77(char* rFilePath){
    LZ77State st;
    st.rfp=fopen(rFilePath,"rb");
    if(st.rfp==NULL){
        printf("读取：打开文件错误。\n");
        return -1;
    }
    char wFilePath[MAX_PATH];
    strcpy(wFilePath,rFilePath);
    strcat(wFilePath,".mylz");
    st.wfp=fopen(wFilePath,"wb");
    if(st.wfp==NULL){
        printf("写入：打开文件错误。\n");
        fclose(st.rfp);
        return -1;
    }
    clearWindow(&st.win);
    memset(st.head,0,sizeof(st.head));
    memset(st.prev,0,sizeof(st.prev));
    st.la_len=0;
    st.curPos=0;

    fill_la(&st,MIN_MATCH);
    if(st.la_len<MIN_MATCH){
        for(int i=0;i<st.la_len;i++){
            unsigned char flag=0;
            fwrite(&flag,1,1,st.wfp);
            fwrite(&st.la[i],1,1,st.wfp);
        }
        fclose(st.rfp);
        fclose(st.wfp);
        return 0;
    }
    while(st.la_len>0){
        unsigned int match_dist=0;
        unsigned int match_len=find_match(&st,&match_dist);
        if(match_len >= MIN_MATCH){
		    // 1. 限制长度防止4位字段溢出
		    if(match_len > MIN_MATCH + 15)
		        match_len = MIN_MATCH + 15;
		    // 2. 限制长度不超过前瞻缓冲区剩余字节数（防止越界）
		    if(match_len > st.la_len)
		        match_len = st.la_len;
		
		    unsigned char flag = 1;
		    fwrite(&flag, 1, 1, st.wfp);
		    unsigned int off = match_dist - 1;
		    unsigned int len_code = match_len - MIN_MATCH;
		    // 原代码：
			// unsigned short packed = (off & 0xFFF) | ((len_code & 0xF) << 12);
			// fwrite(&packed, 2, 1, st.wfp);
			
			// 改为：
			unsigned char buf[3];
			buf[0] = off & 0xFF;                      // 距离低8位
			buf[1] = ((off >> 8) & 0x7F) |            // 距离高7位
			         ((len_code & 0x01) << 7);        // 长度最低1位
			buf[2] = (len_code >> 1) & 0x07;          // 长度高3位（len_code ≤ 15）
			fwrite(buf, 3, 1, st.wfp);
		    consume(&st, match_len);
		}else{
            unsigned char flag=0;
            fwrite(&flag,1,1,st.wfp);
            fwrite(&st.la[0],1,1,st.wfp);
            consume(&st,1);
        }
        fill_la(&st, LA_SIZE);
    }
    fclose(st.rfp);
    fclose(st.wfp);
    return 0;
}

static void push_char(unsigned char* w, unsigned int* b, unsigned char ch){
    w[(*b)&(WSIZE-1)]=ch;
    (*b)++;
}
static unsigned char get_char(unsigned char* w, unsigned int cur, unsigned int dist){
    unsigned int idx=(cur-dist)&(WSIZE-1);
    return w[idx];
}


// ========== 混合编码：目录递归 LZ77 + 哈夫曼 ==========
static int directoryCode_LZ(FILE* wfp, char* rFilePath){
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
        snprintf(searchPath,MAX_PATH,"%s\\*",rFilePath);
        WIN32_FIND_DATA findData;
        HANDLE hFind=FindFirstFile(searchPath,&findData);
        if(hFind==INVALID_HANDLE_VALUE){
            printf("无法访问目录：%s\n",rFilePath);
            return -1;
        }
        do{
            if(strcmp(findData.cFileName,".")==0||strcmp(findData.cFileName,"..")==0)
                continue;
            char fullPath[MAX_PATH];
            snprintf(fullPath,MAX_PATH,"%s\\%s",rFilePath,findData.cFileName);
            if(findData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
                if(directoryCode_LZ(wfp,fullPath)<0){
                    FindClose(hFind);
                    return -1;
                }
            }else{
                if(LZ77(fullPath)<0){
                    FindClose(hFind);
                    return -1;
                }
                char ffullPath[MAX_PATH];
                strcpy(ffullPath,fullPath);
                strcat(ffullPath,".mylz");
                if(normalCode(wfp,ffullPath)<0){
                    FindClose(hFind);
                    remove(ffullPath);
                    return -1;
                }
                remove(ffullPath);
            }
        }while(FindNextFile(hFind,&findData));
        FindClose(hFind);
    }else{
        if(LZ77(rFilePath)<0) return -1;
        char rrFilePath[MAX_PATH];
        strcpy(rrFilePath,rFilePath);
        strcat(rrFilePath,".mylz");
        if(normalCode(wfp,rrFilePath)<0){
            remove(rrFilePath);
            return -1;
        }
        remove(rrFilePath);
    }
    t=2;
    if(fwrite(&t,sizeof(t),1,wfp)!=1){
        printf("写入错误\n");
        return -1;
    }
    return 0;
}

// 混合编码对外接口：clear=1 清空输出文件，=0 追加
int codeFile_LZ(char* wFilePath, char* rFilePath, bool clear){
    FILE* wfp;
    if(clear){
        wfp=fopen(wFilePath,"wb");
        if(wfp) fclose(wfp);
    }
    wfp=fopen(wFilePath,"ab");
    if(!wfp) return -1;
    DWORD attri=GetFileAttributesA(rFilePath);
    if(attri==INVALID_FILE_ATTRIBUTES){
        fclose(wfp);
        return 1;
    }
    int ret;
    if(attri&FILE_ATTRIBUTE_DIRECTORY){
        ret=directoryCode_LZ(wfp,rFilePath);
    }else{
        if(LZ77(rFilePath)<0){
            fclose(wfp);
            return -1;
        }
        char rrFilePath[MAX_PATH];
        strcpy(rrFilePath,rFilePath);
        strcat(rrFilePath,".mylz");
        ret=normalCode(wfp,rrFilePath);
        remove(rrFilePath);
    }
    fclose(wfp);
    return ret;
}

// ========== LZ77 解压（新增） ==========
int LZ77Decompress(FILE* in, const char* outPath) {
    FILE* out = fopen(outPath, "wb");
    if (!out) {
        printf("LZ77解压：无法创建输出文件 %s\n", outPath);
        return -1;
    }

    unsigned char window[WSIZE] = {0};
    unsigned int curPos = 0;

    unsigned char flag;
    while (fread(&flag, 1, 1, in) == 1) {
        if (flag == 0) {
            unsigned char ch;
            if (fread(&ch, 1, 1, in) != 1) break;
            fputc(ch, out);
            window[curPos & (WSIZE - 1)] = ch;
            curPos++;
        } else if (flag == 1) {
            // 原代码：
			// unsigned short packed;
			// if (fread(&packed, 2, 1, in) != 1) break;
			// unsigned int dist = (packed & 0xFFF) + 1;
			// unsigned int len = (packed >> 12) + MIN_MATCH;
			
			// 改为：
			unsigned char buf[3];
			if (fread(buf, 3, 1, in) != 1) break;
			
			unsigned int dist = (buf[0] | ((buf[1] & 0x7F) << 8)) + 1;
			unsigned int len = ((buf[1] >> 7) | ((buf[2] & 0x07) << 1)) + MIN_MATCH;
            for (unsigned int i = 0; i < len; i++) {
                unsigned char ch = window[(curPos - dist + i) & (WSIZE - 1)];
                fputc(ch, out);
                window[(curPos + i) & (WSIZE - 1)] = ch;
            }
            curPos += len;
        } else {
            fclose(out);
            return -1;
        }
    }

    fclose(out);
    return 0;
}
#endif
