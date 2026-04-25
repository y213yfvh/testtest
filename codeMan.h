#ifndef CODEMAN_H
#define CODEMAN_H

#include<stdlib.h>
#include<string.h>
#include<limits.h>

struct tree{
	struct tree* left;
	char ch;
	unsigned int weight;
	struct tree* right;
};
struct vcode{
	char* code[256];
	int size;
};
typedef struct tree tree;
typedef struct vcode vcode;
tree* buildTree(unsigned int weight[]){//weight[]大小必须是256！！！
	tree* Forest[256]={0};
	int forestSize=0;
	for(int i=0;i<256;i++){
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
	while(forestSize>1){
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
	return Forest[min1];
}
void codeTree(vcode* vco,tree* tr,char* s){
	if(tr!=NULL){
		char ss[257];
		strcpy(ss,s);
		if(tr->left){
			codeTree(vco,tr->left,strcat(ss,"0"));
			ss[strlen(ss)-1]='\0';
		}
		if(tr->right){
			codeTree(vco,tr->right,strcat(ss,"1"));
			ss[strlen(ss)-1]='\0';
		}
		else {
			int idx=(unsigned char)tr->ch;
			vco->code[idx]=(char*)malloc(strlen(ss)+1);
			strcpy(vco->code[idx], ss);
			vco->size++;
		}
	}
}
void freeTree(tree* root){
	if(root!=NULL){
		freeTree(root->left);
		freeTree(root->right);
		free(root);
	}
}
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
void freeCode(char* code[]){//code[256]!!!
	for(int i=0;i<256;i++){
		free(code[i]);
		code[i]=NULL;
	}
}
#endif
