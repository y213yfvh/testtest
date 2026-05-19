#ifndef CODEMAN_H
#define CODEMAN_H

#include<stdlib.h>
#include<string.h>
#include<limits.h>

//Huffman树头文件，包括树的构建，编码，释放内存

//树节点（带权值）
struct tree{
	struct tree* left;
	char ch;
	unsigned int weight;
	struct tree* right;
};

//这个结构体有点意义不明
struct vcode{
	char* code[256];
	int size;
};
typedef struct tree tree;
typedef struct vcode vcode;
//构建树
tree* buildTree(unsigned int weight[]){//weight[]大小必须是256！！！
	tree* Forest[256]={0};
	int forestSize=0;
	for(int i=0;i<256;i++){//创造树林
		if(weight[i]>0){
			tree* Tr=(tree*)malloc(sizeof(tree));
			Tr->ch=i;
			Tr->left=NULL;
			Tr->right=NULL;
			Tr->weight=weight[i];
			Forest[forestSize]=Tr;
			forestSize++;
		}
	}
	if(forestSize==0){
		return NULL;
	}
	int min1,min2;
	unsigned int minw1,minw2;
	while(forestSize>1){//不断在树林中找权值最小的根，并构建新的树，把根加入其中
		minw1=UINT_MAX,minw2=UINT_MAX;
		for(int i=0;i<256;i++){
			if(Forest[i]&&Forest[i]->weight<minw1){
				min1=i;
				minw1=Forest[i]->weight;
			}
		}
		for(int i=0;i<256;i++){
			if(Forest[i]&&Forest[i]->weight<minw2&&i!=min1){
				min2=i;
				minw2=Forest[i]->weight;
			}
		}
		tree* p1=Forest[min1];
		tree* p2=Forest[min2];
		tree* p=(tree*)malloc(sizeof(tree));
		p->left=p1;
		p->right=p2;
		p->weight=p1->weight+p2->weight;
		Forest[min1]=p;
		Forest[min2]=NULL;
		forestSize--;
	}
	return Forest[min1];//返回根节点
}
//由树递归生成编码
void codeTree(vcode* vco,tree* tr,char* s){
	if(tr!=NULL){
		char ss[257];
		strcpy(ss,s);
		if(tr->left){
			codeTree(vco,tr->left,strcat(ss,"0"));
			ss[strlen(ss)-1]='\0';//往左接0
		}
		if(tr->right){
			codeTree(vco,tr->right,strcat(ss,"1"));
			ss[strlen(ss)-1]='\0';//往右接1
		}
		else {
			int idx=(unsigned char)tr->ch;
			vco->code[idx]=(char*)malloc(strlen(ss)+1);
			strcpy(vco->code[idx], ss);
			vco->size++;
		}
	}
}
//递归释放树的所有节点内存
void freeTree(tree* root){
	if(root!=NULL){
		freeTree(root->left);
		freeTree(root->right);
		free(root);
	}
}
//传入权值数组和字符指针数组，生成编码，把树的创建和释放限制在这里面
void RETcode(unsigned int weight[],char* code[]){//code[]、weight[]大小必须是256！！！
	tree* huffmanTree=buildTree(weight);
	vcode vco;
	vco.size=0;
	for(int i=0;i<256;i++){
		vco.code[i]=NULL;
	}
	char s[257]={0};
	codeTree(&vco,huffmanTree,s);
	memcpy(code,vco.code,256*sizeof(char*));
	freeTree(huffmanTree);
}
//释放字符指针数组的内存，这块设计有点奇特，开始时没想好
//本来应该是直接用257*256的二维数组的，相比于压缩解压部分增加的开销相当小
void freeCode(char* code[]){//code[256]!!!
	for(int i=0;i<256;i++){
		free(code[i]);
		code[i]=NULL;
	}
}
#endif
