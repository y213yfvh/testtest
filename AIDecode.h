//MADE BY DEEPSEEK-V3
// decode.c
#ifndef AIDECODE_H
#define AIDECODE_H
// decode.c
// 与你的编码器完全匹配的解码器（无 overwrite 参数，始终覆盖）
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <limits.h>
#include "codeMan.h"   // 你的哈夫曼树头文件（包含 buildTree, freeTree）
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
    int weight[256] = {0};
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
#endif
