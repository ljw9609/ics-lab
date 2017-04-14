/* 515030910036 */
/* Lv Jiawei */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "cachelab.h"

/* funcs */
void usage(char *argv[]);
void getarg(int argc,char *argv[],int *E,int *s,int *b,int *verbose,FILE **file);
void* Hit(void *cache,int s,int E,int b,unsigned long addr);
int setline(void *cache,int s,int E,int b,unsigned long addr);

/* print help message */
void usage(char *argv[]){
	printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n",argv[0]);
	printf("Options:\n");
	printf("  -h         Print this help message.\n");
	printf("  -v         Optional verbose flag.\n");
	printf("  -s <num>   Number of set index bits.\n");
	printf("  -E <num>   Number of lines per set.\n");
	printf("  -b <num>   Number of block offset bits\n");
	printf("  -t <file>  Trace file.\n\n");
	printf("Examples:\n");
	printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n",argv[0]);
	printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n",argv[0]);
}

/* get args for function */
void getarg(int argc,char *argv[],int *E,int *s,int *b,int *verbose,FILE **file){
	char type;
	while((type = getopt(argc, argv, "hvs:E:b:t:")) != -1){
		switch(type){
			case 'h':usage(argv);exit(0);
			case 'v':*verbose = 1;break;
			case 's':*s = atoi(optarg);break;
			case 'E':*E = atoi(optarg);break;
			case 'b':*b = atoi(optarg);break;
			case 't':*file = fopen(optarg,"r");break;
		}
	}
}

/* check if hit target */
void* Hit(void *cache,int s,int E,int b,unsigned long addr){
	int t = 64 - (s + b);	//t = m - (s + b)
	int len = 1 + 1 + 8 + (1 << b); //valid 1byte,LRU 1byte,tag 8byte
	long tag_id = addr >> (s + b);
	int set_id = (addr << t) >> (t + b);

	//cache line pointer,line[0] or valid,line[1] for LRU,line[2] for tag
	void *line = cache + len * E * set_id;	

	for(int i = 0;i < E;i++){
		if(((char*)line)[0] == 0){ //check if valid
			line += len;
			continue;
		}
		((unsigned char*)line)[1] += 1; //LRU
		line += len;
	}
	line = cache + len * E * set_id;
	for(int i = 0;i < E;i++){
		if(((char*)line)[0] == 0){
			line += len;
			continue;
		}
		long tag = ((long*)(line + 1 + 1))[0];
		if(tag_id == tag){
			((unsigned char*)line)[1] = 1;	//LRU
			return line;
		}
		line += len;
	}
	return NULL; 
}

/* if not hit,then set line */
int setline(void *cache,int s,int E,int b,unsigned long addr){
	int t = 64 - (s + b);	//t = m - (s + b)
	int len = 1 + 1 + 8 + (1 << b); //valid 1byte,LRU 1byte,tag 8byte
	long tag_id = addr >> (s + b);
	int set_id = (addr << t) >> (t + b);
	void *line = cache + len * E * set_id;
	unsigned int max = 0,lru = 0;
	
	for(int i = 0;i < E;i++){
		if(((char*)line)[0] == 0){	//write to free block
			((char*)line)[0] = 1;
			((char*)line)[1] = 1;
			((long*)(line + 1 + 1))[0] = tag_id;
			return 1; 
		}
		if(((unsigned char*)line)[1] > max){
			max = ((unsigned char*)line)[1];
			lru = i;
		}
		line += len;
	}
	line = cache + len * (E * set_id + lru);	//rewrite in the place earliest
	((char*)line)[0] = 1;
	((char*)line)[1] = 1;
	((long*)(line + 1 + 1))[0] = tag_id;
	return 0;
}


int main(int argc,char *argv[]){
	/* get args */
	int E = 0,s = 0,b = 0,verbose = 0;
	FILE *file = NULL;
	getarg(argc,argv,&E,&s,&b,&verbose,&file);
	int S = 1 << s,B = 1 << b;
	int len = 1 + 1 + 8 + B; //valid 1Byte,LRU 1Byte,tag 8Byte

	/* initialize cache */
	void *cache = malloc(len * E * S);
	memset(cache,0,len * E * S);
	int Hits = 0,Misses = 0,Evictions = 0;

	char opt[1];
	unsigned long addr;
	int size;
	
	/*
	printf("breakpoint1\n");
	printf("%d ,%d ,%d ,%d\n",s,E,b,verbose);
	*/

	/* get traces infomation */
	while(fscanf(file,"%s%lx,%d",opt,&addr,&size) != EOF){
		if(opt[0] == 'I') continue;
		//printf("breakpoint2\n");
		//printf("%c\n",opt[0]);
		void *line;
		switch(opt[0]){
			case 'L':{
				printf("L %lx,%d",addr,size);
				if((line = Hit(cache,s,E,b,addr))!=NULL){
					Hits += 1;
					if(verbose) printf(" hit\n");
				}
				else{
					Misses += 1;
					if(setline(cache,s,E,b,addr) == 0){	//conflict miss
						Evictions += 1;
						if(verbose) printf(" miss eviction\n");
					}
					else{	//cold miss
						if(verbose) printf(" miss\n");
					}
				}
				break;
			}
			case 'S':{
				printf("S %lx,%d",addr,size);
				if((line = Hit(cache,s,E,b,addr)) != NULL){
					Hits += 1;
					if(verbose) printf(" hit\n");
				}
				else{
					Misses += 1;
					if(setline(cache,s,E,b,addr) == 0){
						Evictions += 1;
						if(verbose) printf(" miss eviction\n");
					}
					else{
						if(verbose) printf(" miss\n");
					}
				}
				break;
			}
			case 'M':{ 	//first load then stor
				printf("M %lx,%d",addr,size);
				if((line = Hit(cache,s,E,b,addr))!=NULL){
					Hits += 2;
					if(verbose) printf(" hit hit\n");
				}
				else{
					Misses += 1;
					Hits += 1;
					if(setline(cache,s,E,b,addr) == 0){
						Evictions += 1;
						if(verbose) printf(" miss eviction hit\n");
					}
					else{
						if(verbose) printf(" miss hit\n");
					}
				}
				break;
			}
		}
	}

    printSummary(Hits, Misses, Evictions);
    free(cache);
    fclose(file);
    return 0;
}
