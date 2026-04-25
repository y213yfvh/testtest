#ifndef AIDECODE_H
#define AIDECODE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <limits.h>
#include "codeMan.h"
#include "LZ77.h"   // 你的哈夫曼树头文件（包含 buildTree, freeTree）
// ---------- 创建多级目录 ----------
static void ensureDirectoryExists(const char *path) {
    char tmp[MAX_PATH];
    strncpy(tmp, path, MAX_PATH - 1);
    tmp[MAX_PATH - 1] = '\0';
    int len = (int)strlen(tmp);
    if (len > 0 && tmp[len - 1] == '\\') tmp[len - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '\\') {
            *p = '\0';
            CreateDirectoryA(tmp, NULL);
            *p = '\\';
        }
    }
    CreateDirectoryA(tmp, NULL);
}

// ---------- 比特读取器（与编码器 bitwrite 匹配） ----------
typedef struct {
    FILE *fp;
    unsigned char buffer;
    int bitsLeft;
    long long totalBits;
    long long bitsRead;
} BitReader;

static void openBitReader(BitReader *br, FILE *fp, long long totalBits) {
    br->fp = fp;
    br->buffer = 0;
    br->bitsLeft = 0;
    br->totalBits = totalBits;
    br->bitsRead = 0;
}

// 读取一个比特（高位优先）
static int readBit(BitReader *br) {
    if (br->bitsRead >= br->totalBits) return -1;
    if (br->bitsLeft == 0) {
        if (fread(&br->buffer, 1, 1, br->fp) != 1) return -1;
        br->bitsLeft = 8;
    }
    int bit = (br->buffer >> (br->bitsLeft - 1)) & 1;
    br->bitsLeft--;
    br->bitsRead++;
    return bit;
}

// ---------- 解码单个文件 ----------
static int decodeOneFile(FILE *in, const char *outputPath) {
    // 读取 cha (short)
    short cha;
    if (fread(&cha, sizeof(short), 1, in) != 1) return -1;
    unsigned int weight[256] = {0};
    for (int i = 0; i < cha; i++) {
        unsigned char ch;
        int w;
        if (fread(&ch, 1, 1, in) != 1) return -1;
        if (fread(&w, sizeof(int), 1, in) != 1) return -1;
        weight[ch] = w;
    }
    // 读取总比特数
    long long totalBits;
    if (fread(&totalBits, sizeof(long long), 1, in) != 1) return -1;

    // 使用与编码器完全相同的建树函数
    tree *root = buildTree(weight);
    if (!root) return -1;
    FILE *out = fopen(outputPath, "wb");
    if (!out) {
        freeTree(root);
        return -1;
    }

    BitReader br;
    openBitReader(&br, in, totalBits);
    tree *cur = root;
    int bit;
    while ((bit = readBit(&br)) != -1) {
        if (bit == 0)
            cur = cur->left;
        else
            cur = cur->right;
        if (!cur->left && !cur->right) {
            fputc(cur->ch, out);
            cur = root;
        }
    }
    fclose(out);
    freeTree(root);
    return 0;
}

// ---------- 递归解码目录 ----------
static int decodeDirectory(FILE *in, const char *outputRoot) {
    while (1) {
        char type;
        if (fread(&type, 1, 1, in) != 1) {
            return 0; // 正常结束
        }
        if (type == 2) {
            return 0; // 目录结束标记
        }
        // 读取路径长度和路径
        int pathLen;
        if (fread(&pathLen, sizeof(int), 1, in) != 1) return -1;
        char *path = (char*)malloc(pathLen + 1);
        if (!path) return -1;
        if (fread(path, 1, pathLen, in) != pathLen) {
            free(path);
            return -1;
        }
        path[pathLen] = '\0';
        // 去掉盘符（例如 "C:\"）
        char *relPath = path;
        if (path[1] == ':' && (path[2] == '\\' || path[2] == '/')) {
            relPath = path + 3;
        }
        char fullPath[MAX_PATH];
        snprintf(fullPath, MAX_PATH, "%s\\%s", outputRoot, relPath);
        if (type == 0) { // 文件
            char *lastSlash = strrchr(fullPath, '\\');
            if (lastSlash) {
                *lastSlash = '\0';
                ensureDirectoryExists(fullPath);
                *lastSlash = '\\';
            }
            if (decodeOneFile(in, fullPath) != 0) {
                free(path);
                return -1;
            }
        } else if (type == 1) { // 目录
            ensureDirectoryExists(fullPath);
            if (decodeDirectory(in, outputRoot) != 0) {
                free(path);
                return -1;
            }
        } else {
            free(path);
            return -1;
        }
        free(path);
    }
}

// ---------- 主解码函数 ----------
// 解码压缩文件，输出到 outputRoot（自动创建目录，覆盖同名文件）
int decodeFile(const char *encodedFile, const char *outputRoot) {
    FILE *in = fopen(encodedFile, "rb");
    if (!in) {
        printf("无法打开压缩文件: %s\n", encodedFile);
        return -1;
    }
    ensureDirectoryExists(outputRoot);
    int ret = decodeDirectory(in, outputRoot);
    fclose(in);
    if (ret == 0)
        printf("解码完成，输出目录: %s\n", outputRoot);
    else
        printf("解码失败\n");
    return ret;
}

// ========== 混合解码（LZ77+哈夫曼）新增函数 ==========

// 混合解码单个文件：先哈夫曼解码得到 LZ77 数据，再 LZ77 解压
static int decodeOneFile_Hybrid(FILE* in, const char* outputPath) {
    // 1. 哈夫曼解码到内存缓冲区
    short cha;
    if (fread(&cha, sizeof(short), 1, in) != 1) return -1;

    unsigned int weight[256] = {0};
    for (int i = 0; i < cha; i++) {
        unsigned char ch;
        int w;
        if (fread(&ch, 1, 1, in) != 1) return -1;
        if (fread(&w, sizeof(int), 1, in) != 1) return -1;
        weight[ch] = w;
    }

    long long totalBits;
    if (fread(&totalBits, sizeof(long long), 1, in) != 1) return -1;

    tree* root = buildTree(weight);
    if (!root) return -1;

    // 动态缓冲区，初始 1MB
    size_t bufCap = 1024 * 1024;
    unsigned char* lz77Data = (unsigned char*)malloc(bufCap);
    if (!lz77Data) {
        freeTree(root);
        return -1;
    }
    size_t dataLen = 0;

    BitReader br;
    openBitReader(&br, in, totalBits);

    tree* cur = root;
    int bit;
    while ((bit = readBit(&br)) != -1) {
        cur = (bit == 0) ? cur->left : cur->right;
        if (!cur->left && !cur->right) {
            if (dataLen >= bufCap) {
                bufCap *= 2;
                unsigned char* newBuf = (unsigned char*)realloc(lz77Data, bufCap);
                if (!newBuf) {
                    free(lz77Data);
                    freeTree(root);
                    return -1;
                }
                lz77Data = newBuf;
            }
            lz77Data[dataLen++] = (unsigned char)cur->ch;
            cur = root;
        }
    }
    freeTree(root);

    // 2. 将 LZ77 数据写入临时文件，再调用 LZ77Decompress
    char tempPath[MAX_PATH];
    snprintf(tempPath, MAX_PATH, "%s.tmp.lz77", outputPath);
    FILE* tmp = fopen(tempPath, "wb");
    if (!tmp) {
        free(lz77Data);
        return -1;
    }
    fwrite(lz77Data, 1, dataLen, tmp);
    fclose(tmp);
    free(lz77Data);

    FILE* lzIn = fopen(tempPath, "rb");
    if (!lzIn) {
        remove(tempPath);
        return -1;
    }
    int ret = LZ77Decompress(lzIn, outputPath);
	fclose(lzIn);
	if (ret != 0) {
	    // 解压失败，保留临时文件以便分析
	    printf("[ERROR] LZ77 解压失败，临时文件保留: %s\n", tempPath);
	    // remove(tempPath);  // 先不删除，方便查看数据
	    return -1;
	}
	remove(tempPath);
	return 0;
}

// 混合解码目录递归（结构与 decodeDirectory 一致，但文件调用混合解码）
static int decodeDirectory_Hybrid(FILE* in, const char* outputRoot) {
    while (1) {
        char type;
        if (fread(&type, 1, 1, in) != 1) return 0;
        if (type == 2) return 0;

        int pathLen;
        if (fread(&pathLen, sizeof(int), 1, in) != 1) return -1;
        char* path = (char*)malloc(pathLen + 1);
        if (!path) return -1;
        if (fread(path, 1, pathLen, in) != pathLen) {
            free(path);
            return -1;
        }
        path[pathLen] = '\0';

        // 如果是文件，先去除可能存在的 .mylz 后缀
        if (type == 0) {
            char* dot = strrchr(path, '.');
            if (dot && strcmp(dot, ".mylz") == 0) {
                *dot = '\0';
            }
        }

        // 去掉盘符
        char* relPath = path;
        if (path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
            relPath = path + 3;

        char fullPath[MAX_PATH];
        snprintf(fullPath, MAX_PATH, "%s\\%s", outputRoot, relPath);

        if (type == 0) { // 文件
            char* lastSlash = strrchr(fullPath, '\\');
            if (lastSlash) {
                *lastSlash = '\0';
                ensureDirectoryExists(fullPath);
                *lastSlash = '\\';
            }
            if (decodeOneFile_Hybrid(in, fullPath) != 0) {
                free(path);
                return -1;
            }
        } else if (type == 1) { // 目录
            ensureDirectoryExists(fullPath);
            if (decodeDirectory_Hybrid(in, outputRoot) != 0) {
                free(path);
                return -1;
            }
        } else {
            free(path);
            return -1;
        }
        free(path);
    }
}

// 对外混合解压接口
int decodeFile_Hybrid(const char* encodedFile, const char* outputRoot) {
    FILE* in = fopen(encodedFile, "rb");
    if (!in) {
        printf("无法打开压缩文件: %s\n", encodedFile);
        return -1;
    }
    ensureDirectoryExists(outputRoot);
    int ret = decodeDirectory_Hybrid(in, outputRoot);
    fclose(in);
    if (ret == 0)
        printf("混合解码完成，输出目录: %s\n", outputRoot);
    else
        printf("混合解码失败\n");
    return ret;
}
#endif
