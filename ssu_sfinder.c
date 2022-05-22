#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <openssl/md5.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>

#define PATH_MAX 4096
#define HASH_SIZE 35
#define BUFSIZE 1024*16
#define BUF_MAX 1024
#define ARG_MAX 11
#define FILE_SIZE 16

#define KB 1000
#define MB 1000000
#define GB 1000000000
/******  struct declaring ******/

typedef struct Node{
	char path[PATH_MAX];
	struct Node* next;
	struct Node* prev;
	unsigned char hash[HASH_SIZE];
	double size;
}Node;

typedef struct List{ // duplicate file List
	Node* front;
	Node* rear;
	struct List* next;
	struct List* prev;
	int count;
}List;

typedef struct Set{ // duplicate file Set
	List* front;
	List* rear;
	List* cur;
	int count;
}Set;

typedef struct Queue{ // directory Queue
	Node* front;
	Node* rear;
	int count;
}Queue;

/******  Global Variables ******/
unsigned char hashVal[HASH_SIZE];
int fmd5_callcnt;
int DUP;
int DIF_FILE;


int split(char* string, char* seperator, char* argv[]);
int check_fmd5_opt(int arg_cnt, char* arg_vec[], char* Ext, char* Min, char* Max, char* Target_dir, int* Thread_num);
off_t get_fileSize(char* path);
int get_dupList(char* Ext, char* Min, char* Max, char* Target_dir, Set* set);
int check_targetDir(char* Ext, char* Target_dir);
void BFS(Queue queue, Set* set);
void remove_dup(Set* set);
int check_size(char* Min, char* Max, char* path);
char* toComma(long n, char* com_str);
void print_dupSet(Set* set);
void print_duplist(List* list);
void sort_upward();
void sort_downward();


/****** LinkedList for duplicate Set *******/
void init_Set(Set* set);
int isEmpty_Set(Set* set);
void append_Set(Set* set, List* list);

/****** LinkedList for fileList *******/
void init_List(List* list);
int isEmpty_List(List* list);
void append_List(List* list, char* path);

/******  Queue for directory ******/
void initQueue(Queue* queue);
int isEmpty_dir(Queue* queue);
void enqueue(Queue* queue, char* path);
char* dequeue(Queue* queue, char* path);


/******  MD5 function ******/
void md5(FILE* f, unsigned char* hash);
int MD5_Init(MD5_CTX* c);
int MD5_Update(MD5_CTX* c, const void* data, unsigned long len);
int MD5_Final(unsigned char* md, MD5_CTX* c);



int main(int argc, char* argv[]){
	
	char input[BUF_MAX];
	int arg_cnt = 0;
	char* arg_vec[ARG_MAX];
	Set set;

	while(1){
		fflush(stdin);
		printf("20182613> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input)-1] = '\0';
		arg_cnt = split(input, " ", arg_vec);

		optind = 1; // reuse getopt()
		if(arg_cnt == 0)
			continue;
		if(!strcmp(arg_vec[0], "fmd5")){
			if(arg_cnt < 9){
				fprintf(stdout, "Usage: fmd5 -e [ext] -l [min] -h [max] -d [dir] -t [#thread]\n");
				continue;
			}

			char* Ext = (char*)malloc(strlen(arg_vec[2]));
			char* Min = (char*)malloc(strlen(arg_vec[4]));
			char* Max = (char*)malloc(strlen(arg_vec[6]));
			char* Target_dir = (char*)malloc(strlen(arg_vec[8]));
			int Thread_num = 1; // default

			if(check_fmd5_opt(arg_cnt, arg_vec, Ext, Min, Max, Target_dir, &Thread_num) < 0){
				fprintf(stdout, "option input error\n");
				continue;
			}
			
			if(get_dupList(Ext, Min, Max, Target_dir, &set) < 0){
				fprintf(stdout, "input error\n");
				continue;
			}
			

			print_dupSet(&set);


			fmd5_callcnt++;
		}
		else if(!strcmp(arg_vec[0], "list")){
			if(fmd5_callcnt < 1)
				continue;
			printf("list\n");
		}
		else if(!strcmp(arg_vec[0], "trash")){
			printf("trash\n");
		}
		else if(!strcmp(arg_vec[0], "restore")){
			if(fmd5_callcnt < 1)
				continue;
			printf("restore\n");
		}
		else if(!strcmp(arg_vec[0], "exit")){
			printf("Prompt End\n");
			exit(0);
		}
		else{ // "help"
			printf("Usage:\n");
			printf(" > fmd5 -e [FILE_EXTENSION] -l [MINSIZE] -h [MAXSIZE] -d [TARGET_DIRECTORY] -t [THREAD_NUM]\n");
			printf(" 	>> delete -l [SET_INDEX] -d [OPTARG] -i -f -t\n");
			printf(" > trash -c [CATEGORY] -o [ORDER]\n");
			printf(" > restore [RESTORE_INDEX]\n");
			printf(" > help\n");
			printf(" > exit\n\n");
		}

	}


	struct timeval startTime, endTime;
	gettimeofday(&startTime, NULL);
	gettimeofday(&endTime, NULL);

	exit(0);
}


int split(char* string, char* seperator, char* argv[]){
	int argc = 0;
	char* ptr = NULL;
	ptr = strtok(string, seperator);
	while(ptr != NULL){
		argv[argc++] = ptr;
		ptr = strtok(NULL, " ");
	}
	return argc;
}

void init_Set(Set* set){
	set->front = set->rear = NULL;
	set->count = 0;
	return;
}

int isEmpty_Set(Set* set){
	return set->count == 0;
}

void append_Set(Set* set, List* list){
	List* newlist = (List*)malloc(sizeof(List));
	
	newlist->next = NULL;
	newlist->prev = set->rear;

	if(isEmpty_Set(set)){
		set->front = newlist;
		set->rear = newlist;
	}
	else
		set->rear->next = newlist;

	set->rear = newlist;
	set->count++;
	return;
}



void init_List(List* list){
	list->front = list->rear = NULL;
	list->count = 0;
	return;
}

int isEmpty_List(List* list){
	return list->count == 0;
}

void append_List(List* list, char* path){
	Node* newNode = (Node*)malloc(sizeof(Node));
	memset(hashVal, '\0', HASH_SIZE);
	
	FILE* IN;
	if((IN = fopen(path, "r")) == NULL){
		fprintf(stderr, "fopen error in enqueue_Set()\n");
		printf("%s\n", strerror(errno));
		exit(1);
	}

	md5(IN, hashVal);
	fclose(IN);

	newNode->size = get_fileSize(path);
	strcpy(newNode->path, path);
	strcpy(newNode->hash, hashVal);
	newNode->next = NULL;
	newNode->prev = list->rear;

	if(isEmpty_List(list)){
		list->front = newNode;
		list->rear = newNode;
	}
	else
		list->rear->next = newNode;

	list->rear = newNode;
	list->count++;
	return;
}




int isEmpty_dir(Queue* queue){
	return queue->count == 0;
}

void enqueue(Queue* queue, char* path){
	Node* newNode = (Node*)malloc(sizeof(Node));
	
	strcpy(newNode->path, path);
	newNode->next = NULL;
	newNode->prev = queue->rear;

	if(isEmpty_dir(queue)){
		queue->front = newNode;
		queue->rear = newNode;
	}
	else
		queue->rear->next = newNode;

	queue->rear = newNode;
	queue->count++;
	return;
}

char* dequeue(Queue* queue, char* path){
	Node* ptr;
	if(isEmpty_dir(queue))
		return NULL;
	ptr = queue->front;
	strcpy(path, ptr->path);
	queue->front = ptr->next;
	free(ptr);
	queue->count--;
	return path;
}



void md5(FILE* f, unsigned char* hash){
	MD5_CTX c;
	unsigned char md[MD5_DIGEST_LENGTH];
	int fd;
	int i;
	static unsigned char buf[BUFSIZE];
	fd = fileno(f);
	MD5_Init(&c);
	for(;;){
		i = read(fd, buf, BUFSIZE);
		if(i<=0) break;
		MD5_Update(&c, buf, (unsigned long)i);
	}
	MD5_Final(&(md[0]), &c);
	for(int i=0; i<MD5_DIGEST_LENGTH; i++)
		sprintf(hash+(i*2), "%02x", md[i]);
	return;
}

int check_fmd5_opt(int arg_cnt, char* arg_vec[], char* Ext, char* Min, char* Max, char* Target_dir, int* Thread_num){
	int opt;
	char* param_opt;
	int thread_num;
	int i=0;
	while((opt = getopt(arg_cnt, arg_vec, "e:l:h:d:t:")) != -1){
		switch(opt){
			case 'e':
				strcpy(Ext, optarg);
				break;

			case 'l':
				strcpy(Min, optarg);
				break;

			case 'h':
				strcpy(Max, optarg);
				break;

			case 'd':
				strcpy(Target_dir, optarg);
				break;

			case 't':
				*Thread_num = atoi(optarg);
				break;

			case '?':
				return -1;

			default:
				return -1;
		}
	}
	for(int i=0; i<arg_cnt; i++)
		memset(arg_vec[i], 0, strlen(arg_vec[i]));

	return 0;

}

off_t get_fileSize(char* path){
	struct stat st;
	char buf[PATH_MAX];
	memset(buf, '\0', PATH_MAX);
	strcpy(buf, path);
	lstat(buf, &st);

	return st.st_size;
}

int get_dupList(char* Ext, char* Min, char* Max, char* Target_dir, Set* set){
	char realPath[PATH_MAX];
	memset(realPath, '\0', PATH_MAX);
	if(Target_dir[0] == '~'){
		if(strlen(Target_dir) > 1){
			char ptr[PATH_MAX];
			strcpy(ptr, &Target_dir[2]);
			sprintf(Target_dir, "%s/%s", getenv("HOME"), ptr); // get Home directory path
		}
		else
			sprintf(Target_dir, "%s", getenv("HOME"));
	}

	if(check_targetDir(Ext, Target_dir) < 0)
		return -1;


	Queue queue;
	initQueue(&queue);

	if(!strcmp(Target_dir, "/")){
		enqueue(&queue, "/");
		BFS(queue, set);
	}
	else if(Target_dir[0] == '.'){
		if(realpath(Target_dir, realPath) == NULL){
			fprintf(stderr, "realpath() error\n");
			return -1;
		}
		enqueue(&queue, realPath);
		BFS(queue, set);
	}
	else{
		if(realpath(Target_dir, realPath) == NULL){
			fprintf(stderr, "realpath() error\n");
			return -1;
		}
		enqueue(&queue, realPath);
		BFS(queue, set);
	}

	return 0;
}


int check_targetDir(char* Ext, char* Target_dir){
	struct stat st;
	if(lstat(Target_dir, &st) < 0){
		printf("Not a Directory or file\n");
		return -1;
	}
	if(Ext[0] != '*'){
		fprintf(stdout, "Extension error\n");
		return -1;
	}
	if(Ext[strlen(Ext)-1] == '.'){
		fprintf(stdout, "Extension error\n");
		return -1;
	}
	if(strstr(Ext, ".") == NULL && strlen(Ext) != 1){
		fprintf(stdout, "Extension error\n");
		return -1;
	}
	return 0;
}

void BFS(Queue queue, Set* set){
	struct dirent** namelist;
	struct stat st;
	char curr_dir[PATH_MAX-256];
	char tmp_buf[PATH_MAX];

	if(isEmpty_dir(&queue))
		return;
	else{
		memset(curr_dir, '\0', PATH_MAX-256);
		memset(tmp_buf, '\0', PATH_MAX);
		strcpy(curr_dir, dequeue(&queue, tmp_buf));
		int fileCnt = scandir(curr_dir, &namelist, NULL, alphasort);
		for(i=0; i<fileCnt; i++){
			if(!strcmp(namelist[i]->d_name, "."))
				continue;
			if(!strcmp(namelist[i]->d_name, ".."))
				continue;
			char real_path[PATH_MAX];
			memset(real_path, '\0', PATH_MAX);
			if(!strcmp(curr_dir, "/"))
				sprintf(real_path, "%s%s", curr_dir, namelist[i]->d_name);
			else
				sprintf(real_path, "%s/%s", curr_dir, namelist[i]->d_name);
			lstat(real_path, &st);

			unsigned char tmp_sc[HASH_SIZE];
			memset(f_hash, '\0', HASH_SIZE);
			FILE* IN;
			if((IN = fopen(real_path, "r")) == NULL)
				continue;
			if(!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode)){
				fclose(IN);
				continue;
			}

			md5(IN, f_hash);
			fclose(IN);


			int f_Size = get_fileSize(real_path);

			if(S_ISDIR(st.st_mode)){
				if((strcmp(real_path, "/proc") == 0) || (strcmp(real_path, "/run") == 0) || (strcmp(real_path, "/sys") == 0))
					continue;
				else
					enqueue(&queue, real_path);
			}
			else if(S_ISREG(st.st_mode)){
				int condition = 0;
				condition += check_ext(Ext, real_path);
				condition += check_size(Min, Max, real_path);

				if(condition == 0){
					if(set->count == 0){
						List list;
						append_List(&list, real_path);
						append_Set(&set, &list);
					}
					else{
						int isFirst = 1;
						int j;
						set->cur = set->front;
						while(set->cur != NULL){
							if(!strcmp(set->cur->front->hash, f_hash) && (f_size == set->cur->front->size)){ // hash & size
								append_List(&set->cur, real_path);
								isfirst = 0;
								DUP++;
								break;
							}
							set->cur = set->cur->next;
						}
						if(isFirst == 1){
							List list;
							append_List(&list, real_path);
							append_Set(&set, &list);
							DIF_FILE++;
						}
					}
				}
			}
			else // !DIR && !REG
				continue;
		}
	}
	
	if(set->count == 1){}
	else
		set->count--;

	remove_dup(&set);
	return;
}

void remove_dup(Set* set){
	set->cur = set->front;
	while(set->cur != NULL){
		if(set->cur->count == 1){
			init_List(&set->cur);
			set->cur->prev = set->cur->next;
			set->cur->next = set->cur->prev;
			set->count--;
		}
		set->cur = set->cur->next;
	}
	return;
}

int check_size(char* Min, char* Max, char* path){
	struct stat st;
	lstat(path, &st);

	if(!strcmp(Min, "~") && !strcmp(Max, "~"))
		return 0;
	else if(strcmp(Min, "~") != 0 && strcmp(Max, "~") == 0){
		double minsize = atof(Min);
		if(strstr(Min, "kb") != NULL || strstr(Min, "KB") != NULL)
			minsize *= KB;
		if(strstr(Min, "mb") != NULL || strstr(Min, "MB") != NULL)
			minsize *= MB;
		if(strstr(Min, "gb") != NULL || strstr(Min, "GB") != NULL)
			minsize *= GB;
		if(minsize <= st.st_size)
			return 0;
		else
			return 1;
	}
	else if(strcmp(Min, "~") == 0 && strcmp(Max, "~") != 0){
		double maxsize = atof(Max);
		if(strstr(Min, "kb") != NULL || strstr(Min, "KB") != NULL)
			maxsize *= KB;
		if(strstr(Min, "mb") != NULL || strstr(Min, "MB") != NULL)
			maxsize *= MB;
		if(strstr(Min, "gb") != NULL || strstr(Min, "GB") != NULL)
			maxsize *= GB;
		if(st.st_size <= maxsize)
			return 0;
		else
			return 1;
	}
	else{
		double minsize = atof(Min);
		if(strstr(Min, "kb") != NULL || strstr(Min, "KB") != NULL)
			minsize *= KB;
		if(strstr(Min, "mb") != NULL || strstr(Min, "MB") != NULL)
			minsize *= MB;
		if(strstr(Min, "gb") != NULL || strstr(Min, "GB") != NULL)
			minsize *= GB;

		double maxsize = atof(Max);
		if(strstr(Min, "kb") != NULL || strstr(Min, "KB") != NULL)
			maxsize *= KB;
		if(strstr(Min, "mb") != NULL || strstr(Min, "MB") != NULL)
			maxsize *= MB;
		if(strstr(Min, "gb") != NULL || strstr(Min, "GB") != NULL)
			maxsize *= GB;

		if((minsize <= st.st_size) && (st.st_size <= maxsize))
			return 0;
		else
			return 1;
	}

	return 0;
}

char* toComma(long n, char* com_str){
	char str[FILE_SIZE];

	sprintf(str, "%ld", n);
	int len = strlen(str);
	int mod = len % 3;
	int id = 0;

	for(int i=0; i<len; i++){
		if((i!=0) && ((i%3) == mod))
			com_str[id++] = ',';
		com_str[id++] = str[i];
	}
	com_str[id] = 0x00;
	return com_str;
}

void print_dupSet(Set* set){
	unsigned char f_hash[HASH_SIZE];
	char fileSize[FILE_SIZE];
	set->cur = set->front;
	int i=0;
	while(set->cur != NULL){
		memset(f_hash, '\0', HASH_SIZE);
		memset(fileSize, '\0', FILE_SIZE);
		FILE* IN;
		if((IN = fopen(set->cur->front->path, "r")) == NULL)
			printf("In print_dupList fopen(): %s\n", strerror(errno));
		md5(IN, f_hash);
		fclose(IN);
		toComma(set->cur->front->size, fileSize);
		printf("---- Identical files #%d (%s bytes -", i+1, fileSize);
		for(int j=0; j < MD5_DIGEST_LENGTH; j++)
			printf("%02x", tmp[j]);
		printf(") ----\n");
		print_duplist(&set->cur);

		i++;
		set->cur = set->cur->next;
	}
	return;
}

void print_duplist(List* list){
	Node* cur = list->front;
	int index = 1;
	struct stat st;
	while(cur!=NULL){
		lstat(cur->path, &st);
		time_t mt = st.st_mtime;
		time_t at = st.st_atime;
		struct tm mT;
		struct tm aT;
		localtime_r(&mt, &mT);
		localtime_r(&at, &aT);

		printf("[%d] %s (mtime : %d-%02d-%02d %02d:%02d:%02d) (atime: %d-%02d-%02d %02d:%02d:%02d) (uid : %d) (gid : %d)\n",
				index++, cur->path,
				mT.tm_year+1900, mT.tm_mon+1, mT.tm_mday+1, mT.tm_hour, mT.tm_min, mT.tm_sec,
				aT.tm_year+1900, aT.tm_mon+1, aT.tm_mday+1, aT.tm_hour, aT.tm_min, aT.tm_sec,
				getuid(), getgid());
		
		cur = cur->next;
	}
	printf("\n");
	return;
}

void sort_upward(){

}

void sort_downward(){

}
