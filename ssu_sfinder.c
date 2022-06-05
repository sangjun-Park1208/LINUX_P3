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
#include <pthread.h>

#define PATH_MAX 4096
#define HASH_SIZE 35
#define BUFSIZE 1024*16
#define BUF_MAX 1024
#define ARG_MAX 11
#define FILE_SIZE 16
#define TIME_BUF 20
#define EXT_BUF 20
#define SIZE_BUF 10

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
	Node* cur;
	Node* tmp1;
	Node* tmp2;
	Node* tmp3;
	Node* tmp_cur;
	struct List* next;
	struct List* prev;
	int count;
}List;

typedef struct Set{ // duplicate file Set
	List* front;
	List* rear;
	List* cur;
	List* tmp1;
	List* tmp2;
	List* tmp3;
	List* tmp_cur;
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
Set set;
Queue queue;
char curr_dir[PATH_MAX-256];
char tmp_buf[PATH_MAX];
char real_path[PATH_MAX];
unsigned char f_hash[HASH_SIZE];
char homedir_path[PATH_MAX-256];
char logfile_path[PATH_MAX];
char trashdir_path[PATH_MAX];
char trashinfo_path[PATH_MAX];

int split(char* string, char* seperator, char* argv[]);
int check_fmd5_opt(int arg_cnt, char* arg_vec[], char* Ext, char* Min, char* Max, char* Target_dir, int* Thread_num);
off_t get_fileSize(char* path);
int get_dupList(char* Ext, char* Min, char* Max, char* Target_dir, Set* set);

int check_targetDir(char* Ext, char* Target_dir);
int check_ext(char* Ext, char* path);
void BFS(Queue queue, Set* set, char* Ext, char* Min, char* Max);

void remove_dup(Set* set);
int check_size(char* Min, char* Max, char* path);
char* toComma(long n, char* com_str);
void print_dupSet(Set* set);
void print_duplist(List* list);
void printList(List* list);
void printSet(Set* set);

/******** [LIST] option *********/
void list_opt(Set* set, int arg_cnt, char* arg_vec[]);
void up_sort_size(Set* set);
void down_sort_size(Set* set);
void up_sort_filename(Set* set);
void down_sort_filename(Set* set);
void up_sort_uid(Set* set);
void down_sort_uid(Set* set);
void up_sort_gid(Set* set);
void down_sort_gid(Set* set);
void up_sort_mode(Set* set);
void down_sort_mode(Set* set);
void up_sort_size_filelist(Set* set);
void down_sort_size_filelist(Set* set);



/******** [TRASH] option *********/
void trash_opt(Set* set, int arg_cnt, char* arg_vec[]);



/******** [RESTORE] option *********/
void restore_opt(Set* set, int RES_IDX);




/******** Delete function *********/
void delete_prompt(Set* set);
void delete_d(int SET_IDX, int LIST_IDX, Set* set);
void d_opt(int SET_IDX, int LIST_IDX, Set* set);
void delete_i(int SET_IDX, Set* set);
void i_opt(int SET_IDX, Set* set);
void delete_f(int SET_IDX, int REC_IDX, Set* set);
void f_opt(int SET_IDX, Set* set);
void delete_t(int SET_IDX, int REC_IDX, Set* set);
void t_opt(int SET_IDX, Set* set);
int get_recentIDX(int SET_IDX, Set* set);

/****** LinkedList for duplicate Set *******/
void init_Set(Set* set);
int isEmpty_Set(Set* set);
void append_Set(Set* set, char* path);

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
	char* Ext = (char*)malloc(EXT_BUF);
	char* Min = (char*)malloc(SIZE_BUF);
	char* Max = (char*)malloc(SIZE_BUF);
	char* Target_dir = (char*)malloc(BUF_MAX);


	struct timeval startTime, endTime;
	
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

			int Thread_num = 1; // default

			if(check_fmd5_opt(arg_cnt, arg_vec, Ext, Min, Max, Target_dir, &Thread_num) < 0){
				fprintf(stdout, "option input error\n");
				continue;
			}
			gettimeofday(&startTime, NULL);

			if(get_dupList(Ext, Min, Max, Target_dir, &set) < 0){
				fprintf(stdout, "input error\n");
				continue;
			}
			
			gettimeofday(&endTime, NULL);
			
			endTime.tv_sec -= startTime.tv_sec;
			if(endTime.tv_usec < startTime.tv_usec){
				endTime.tv_sec--;
				endTime.tv_usec += 1000000;
			}

			if(fmd5_callcnt == 0){
				up_sort_size(&set);
			}
			print_dupSet(&set);

			printf("Searching time: %ld:%lu(sec:usec)\n\n", endTime.tv_sec, endTime.tv_usec);
			delete_prompt(&set);


			fmd5_callcnt++;
		}
		else if(!strcmp(arg_vec[0], "list")){
			if(fmd5_callcnt < 1)
				continue;
			printf("list\n");
			list_opt(&set, arg_cnt, arg_vec);
			print_dupSet(&set);
		}
		else if(!strcmp(arg_vec[0], "trash")){
			printf("trash\n");
			trash_opt(&set, arg_cnt, arg_vec);
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

void append_Set(Set* set, char* path){
	List* newlist = (List*)malloc(sizeof(List));
	memset(newlist, 0, sizeof(List));
	
	newlist->rear = NULL;
	append_List(newlist, path);

	newlist->next = NULL;
	newlist->prev = set->rear;

	if(isEmpty_Set(set)){

		set->front = newlist;
		set->rear = newlist;
		newlist->prev = NULL;
	}
	else{
		set->rear->next = newlist;
		set->rear = newlist;
	}
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
		newNode->prev = NULL;
	}
	else{
		list->rear->next = newNode;
		list->rear = newNode;
	}
	list->count++;
	return;
}

void delete_List(List* list){
	List* tmp = list;
	if(list->prev == NULL){
		list->next->prev = NULL;
		init_List(tmp);
	}
	else if(tmp->next == NULL){
		list->prev->next = NULL;
		init_List(tmp);
	}
	else{
		list->prev->next = list->next;
		list->next->prev = list->prev;
		init_List(tmp);
	}
	return;
}


int isEmpty_dir(Queue* queue){
	return queue->count == 0;
}

void initQueue(Queue* queue){
	queue->front = queue->rear = NULL;
	queue->count = 0;
	return;
}
void enqueue(Queue* queue, char* path){
	Node* newNode = (Node*)malloc(sizeof(Node));
	
	strcpy(newNode->path, path);
	newNode->next = NULL;

	if(isEmpty_dir(queue)){
		queue->front = newNode;
		queue->rear = newNode;
		newNode->prev = NULL;
	}
	else{
		newNode->prev = queue->rear;
		queue->rear->next = newNode;
		queue->rear = newNode;
	}
	queue->count++;
	return;
}

char* dequeue(Queue* queue, char* path){
	Node* ptr = queue->front;
	strcpy(path, queue->front->path);
	queue->front = queue->front->next;
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


	initQueue(&queue);

	if(!strcmp(Target_dir, "/")){
		enqueue(&queue, "/");
		BFS(queue, set, Ext, Min, Max);
	}
	else if(Target_dir[0] == '.'){
		if(realpath(Target_dir, realPath) == NULL){
			fprintf(stderr, "realpath() error\n");
			return -1;
		}
		enqueue(&queue, realPath);
		BFS(queue, set, Ext, Min, Max);
	}
	else{
		if(realpath(Target_dir, realPath) == NULL){
			fprintf(stderr, "realpath() error\n");
			return -1;
		}
		enqueue(&queue, realPath);
		BFS(queue, set, Ext, Min, Max);
	}

	remove_dup(set);
	if(set->count == 0)
		printf("No duplicates in %s\n", realPath);
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

int check_ext(char* Ext, char* path){
	if(!strcmp(Ext, "*"))
		return 0;
	else{
		if(strrchr(path, '.') == NULL)
			return 1;
		if(!strcmp(&Ext[2], strrchr(path, '.') + 1))
			return 0;
		else
			return 1;
	}
}



void BFS(Queue queue, Set* set, char* Ext, char* Min, char* Max){
	struct dirent** namelist;
	struct stat st;

	if(isEmpty_dir(&queue)){
		return;
	}
	else{
		memset(curr_dir, '\0', PATH_MAX-256);
		memset(tmp_buf, '\0', PATH_MAX);
		strcpy(curr_dir, dequeue(&queue, tmp_buf));
		int fileCnt = scandir(curr_dir, &namelist, NULL, alphasort);
		int i;
		for(i=0; i<fileCnt; i++){
			if(!strcmp(namelist[i]->d_name, "."))
				continue;
			if(!strcmp(namelist[i]->d_name, ".."))
				continue;
			memset(real_path, '\0', PATH_MAX);
			if(!strcmp(curr_dir, "/"))
				sprintf(real_path, "%s%s", curr_dir, namelist[i]->d_name);
			else
				sprintf(real_path, "%s/%s", curr_dir, namelist[i]->d_name);
			lstat(real_path, &st);

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


			int f_size = get_fileSize(real_path);

			if(S_ISDIR(st.st_mode)){
				if((strcmp(real_path, "/proc") == 0) || (strcmp(real_path, "/run") == 0) || (strcmp(real_path, "/sys") == 0))
					continue;
				else{
					enqueue(&queue, real_path);
				}
			}
			else if(S_ISREG(st.st_mode)){
				int condition = 0;
				condition += check_ext(Ext, real_path);
				condition += check_size(Min, Max, real_path);
				if(condition == 0){
					if(set->count == 0){
						append_Set(set, real_path);
					}
					else{
						int isFirst = 1;
						set->cur = set->front;
						while(set->cur != NULL){
							if(!strcmp(set->cur->front->hash, f_hash) && (f_size == set->cur->front->size)){ // hash & size
								append_List(set->cur, real_path);
				
								isFirst = 0;
								DUP++;
								break;
							}
							set->cur = set->cur->next;
						}
						if(isFirst == 1){
							append_Set(set, real_path);
							DIF_FILE++;
						}
					}
				}
			}
			else // !DIR && !REG
				continue;
		}
	}
	BFS(queue, set, Ext, Min, Max);
	return;
}


void printList(List* list){
	list->cur = list->front;
	printf("-----------------------------------\n");
	while(list->cur != NULL){
		printf("path : %s\n", list->cur->path);
		list->cur = list->cur->next;
	}
}

void printSet(Set* set){
	set->cur = set->front;
	while(set->cur != NULL){
		printList(set->cur);
		set->cur = set->cur->next;
	}

}


void remove_dup(Set* set){
	set->cur = set->front;
	while(set->cur != NULL){
		if(set->cur->count == 1){
			if(set->cur->prev == NULL){
				set->cur->next->prev = NULL;
				set->front = set->cur->next;
				init_List(set->cur);
			}
			else if(set->cur->next == NULL){
				set->cur->prev->next = NULL;
				set->rear = set->cur->prev;
				init_List(set->cur);
			}
			else{
				set->cur->prev->next = set->cur->next;
				set->cur->next->prev = set->cur->prev;
				init_List(set->cur);
			}
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

		if((IN = fopen(set->cur->front->path, "r")) == NULL){
			printf("In print_dupList fopen(): %s\n", strerror(errno));
		}
		md5(IN, f_hash);
		fclose(IN);
		toComma(set->cur->front->size, fileSize);
		printf("---- Identical files #%d (%s bytes -", i+1, fileSize);
		for(int j=0; j < MD5_DIGEST_LENGTH; j++)
			printf("%02x", f_hash[j]);
		printf(") ----\n");
		print_duplist(set->cur);

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

		printf("[%d] %s (mtime : %d-%02d-%02d %02d:%02d:%02d) (atime: %d-%02d-%02d %02d:%02d:%02d) (uid : %d) (gid : %d) (mode : %o)\n",
				index++, cur->path,
				mT.tm_year+1900, mT.tm_mon+1, mT.tm_mday+1, mT.tm_hour, mT.tm_min, mT.tm_sec,
				aT.tm_year+1900, aT.tm_mon+1, aT.tm_mday+1, aT.tm_hour, aT.tm_min, aT.tm_sec,
				st.st_uid, st.st_gid, st.st_mode);
		
		cur = cur->next;
	}
	printf("\n");
	return;
}

void up_sort_size(Set* set){ // for fileset sorting
	int check=0;
	set->cur = set->front->next;
	set->tmp_cur = set->rear;

	for(int i=0; i<set->count-1; i++)	{
		while(set->cur != set->tmp_cur){
			if(set->cur == set->front){ // first case, A(cur,front)->B->C
				if(set->cur->front->size  >  set->cur->next->front->size){
					set->tmp1 = set->front->next; // tmp1 = B
					set->tmp2 = set->tmp1->next; // tmp2 = C

					set->cur->next = set->tmp2; // A->C
					set->tmp1->next = set->front; // B->A
					
					set->tmp2->prev = set->cur; // C's prev = A
					set->cur->prev = set->tmp1; // A's prev = B
					set->tmp1->prev = NULL;

					set->front = set->tmp1; // move 'front' to B
				}
				else
					set->cur = set->cur->next;
			}
			else if(set->cur->next == set->rear){ // last case, B->C(cur)->D(rear)
				if(set->cur->front->size  >  set->cur->next->front->size){
					check = 1;
					set->tmp1 = set->cur->prev; // tmp1 = B
					set->tmp1->next = set->rear; // B->D
					set->cur->next = NULL; // C->NULL
					set->rear->next = set->cur; // D->C

					set->rear->prev = set->tmp1; // D's prev = B
					set->cur->prev = set->rear; // C's prev = D
					
					set->rear = set->cur; // move 'rear' to C
					set->tmp_cur = set->cur;
					set->tmp_cur->prev = set->cur->prev;
				}
				else
					set->cur = set->cur->next;
			}
			else{ // other case, A->B(cur)->C->D
				if(set->cur->front->size  >  set->cur->next->front->size){
					set->tmp1 = set->cur->prev; // tmp1 = A
					set->tmp2 = set->cur->next; // tmp2 = C
					set->tmp3 = set->cur->next->next; // tmp3 = D

					set->tmp1->next = set->tmp2; // A->C
					set->cur->next = set->tmp3; // B->D
					set->tmp2->next = set->cur; // C->B

					set->tmp2->prev = set->tmp1; // C's prev = A
					set->tmp3->prev = set->cur; // D's prev = B
					set->cur->prev = set->tmp2; // B's prev = C
				}
				else
					set->cur = set->cur->next;
			}
		}
		if(check==0){
			set->tmp_cur = set->tmp_cur->next;
			check=1;
		}
		set->cur = set->front;
	}
	
	return;
}

void down_sort_size(Set* set){ // for fileset sotring
	int check=0;
	set->cur = set->front->next;
	set->tmp_cur = set->rear;

	for(int i=0; i<set->count-1; i++)	{
		while(set->cur != set->tmp_cur){
			if(set->cur == set->front){ // first case, A(cur,front)->B->C
				if(set->cur->front->size  <  set->cur->next->front->size){
					set->tmp1 = set->front->next; // tmp1 = B
					set->tmp2 = set->tmp1->next; // tmp2 = C

					set->cur->next = set->tmp2; // A->C
					set->tmp1->next = set->front; // B->A
					
					set->tmp2->prev = set->cur; // C's prev = A
					set->cur->prev = set->tmp1; // A's prev = B
					set->tmp1->prev = NULL;

					set->front = set->tmp1; // move 'front' to B
				}
				else
					set->cur = set->cur->next;
			}
			else if(set->cur->next == set->rear){ // last case, B->C(cur)->D(rear)
				if(set->cur->front->size  <  set->cur->next->front->size){
					check = 1;
					set->tmp1 = set->cur->prev; // tmp1 = B
					set->tmp1->next = set->rear; // B->D
					set->cur->next = NULL; // C->NULL
					set->rear->next = set->cur; // D->C

					set->rear->prev = set->tmp1; // D's prev = B
					set->cur->prev = set->rear; // C's prev = D
					
					set->rear = set->cur; // move 'rear' to C
					set->tmp_cur = set->cur;
					set->tmp_cur->prev = set->cur->prev;
				}
				else
					set->cur = set->cur->next;
			}
			else{ // other case, A->B(cur)->C->D
				if(set->cur->front->size  <  set->cur->next->front->size){
					set->tmp1 = set->cur->prev; // tmp1 = A
					set->tmp2 = set->cur->next; // tmp2 = C
					set->tmp3 = set->cur->next->next; // tmp3 = D

					set->tmp1->next = set->tmp2; // A->C
					set->cur->next = set->tmp3; // B->D
					set->tmp2->next = set->cur; // C->B

					set->tmp2->prev = set->tmp1; // C's prev = A
					set->tmp3->prev = set->cur; // D's prev = B
					set->cur->prev = set->tmp2; // B's prev = C
				}
				else
					set->cur = set->cur->next;
			}
		}
		if(check==0){
			set->tmp_cur = set->tmp_cur->next;
			check=1;
		}
		set->cur = set->front;
	}
	
	return;
}

void up_sort_filename(Set* set){ // for [filelist] option
	int check=0;
	set->cur = set->front;
	for(int i=0; i< set->cur->count-1; i++){
		set->cur->cur = set->cur->front->next;
		set->cur->tmp_cur = set->cur->rear;
		while(set->cur->cur != set->cur->tmp_cur){
			if(set->cur->cur == set->cur->front){ // in cur, first case : A(cur, front)->B->C
				if(strlen(set->cur->front->path) > strlen(set->cur->front->next->path)){
					set->cur->tmp1 = set->cur->front->next; // tmp1 = B
					set->cur->tmp2 = set->cur->tmp1->next; // tmp2 = C

					set->cur->cur->next = set->cur->tmp2; // A->C
					set->cur->tmp1->next = set->cur->front; // B->A

					set->cur->tmp2->prev = set->cur->cur; // C's prev = A
					set->cur->cur->prev = set->cur->tmp1; // A's prev = B
					set->cur->tmp1->prev = NULL;

					set->cur->front = set->cur->tmp1; // move 'front' to B
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else if(set->cur->cur->next == set->cur->rear){ // in cur, last case : B->C(cur)->D(rear)
				if(strlen(set->cur->cur->path) > strlen(set->cur->cur->next->path)){
					check = 1;
					set->cur->tmp1 = set->cur->cur->prev; // tmp1 = B
					set->cur->tmp1->next = set->cur->rear; // B->D
					set->cur->cur->next = NULL; // C->NULL
					set->cur->rear->next = set->cur->cur; // D->C

					set->cur->rear->prev = set->cur->tmp1; // D's prev = B
					set->cur->cur->prev = set->cur->rear; // C's prev = D

					set->cur->rear = set->cur->cur; // move 'rear' to C
					set->cur->tmp_cur = set->cur->cur;
					set->cur->tmp_cur->prev = set->cur->cur->prev;
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else{ // other case : A->B(cur)->C->D
				if(strlen(set->cur->cur->path) > strlen(set->cur->cur->next->path)){
					set->cur->tmp1 = set->cur->cur->prev; //tmp1 = A
					set->cur->tmp2 = set->cur->cur->next; //tmp2 = C
					set->cur->tmp3 = set->cur->cur->next->next; // tmp3 = D

					set->cur->tmp1->next = set->cur->tmp2; // A->C
					set->cur->cur->next = set->cur->tmp3; // B->D
					set->cur->tmp2->next = set->cur->cur; // C->B

					set->cur->tmp2->prev = set->cur->tmp1; // C's prev = A
					set->cur->tmp3->prev = set->cur->cur; // D's prev = B
					set->cur->cur->prev = set->cur->tmp2; // B's prev = C
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			if(check==0){
				set->cur->tmp_cur = set->cur->tmp_cur->next;
				check=1;
			}	
		}
		set->cur = set->cur->next;
	}
	return;
}

void down_sort_filename(Set* set){
	int check=0;
	set->cur = set->front;
	for(int i=0; i< set->cur->count-1; i++){
		set->cur->cur = set->cur->front->next;
		set->cur->tmp_cur = set->cur->rear;
		while(set->cur->cur != set->cur->tmp_cur){
			if(set->cur->cur == set->cur->front){ // in cur, first case : A(cur, front)->B->C
				if(strlen(set->cur->front->path) < strlen(set->cur->front->next->path)){
					set->cur->tmp1 = set->cur->front->next; // tmp1 = B
					set->cur->tmp2 = set->cur->tmp1->next; // tmp2 = C

					set->cur->cur->next = set->cur->tmp2; // A->C
					set->cur->tmp1->next = set->cur->front; // B->A

					set->cur->tmp2->prev = set->cur->cur; // C's prev = A
					set->cur->cur->prev = set->cur->tmp1; // A's prev = B
					set->cur->tmp1->prev = NULL;

					set->cur->front = set->cur->tmp1; // move 'front' to B
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else if(set->cur->cur->next == set->cur->rear){ // in cur, last case : B->C(cur)->D(rear)
				if(strlen(set->cur->cur->path) < strlen(set->cur->cur->next->path)){
					check = 1;
					set->cur->tmp1 = set->cur->cur->prev; // tmp1 = B
					set->cur->tmp1->next = set->cur->rear; // B->D
					set->cur->cur->next = NULL; // C->NULL
					set->cur->rear->next = set->cur->cur; // D->C

					set->cur->rear->prev = set->cur->tmp1; // D's prev = B
					set->cur->cur->prev = set->cur->rear; // C's prev = D

					set->cur->rear = set->cur->cur; // move 'rear' to C
					set->cur->tmp_cur = set->cur->cur;
					set->cur->tmp_cur->prev = set->cur->cur->prev;
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else{ // other case : A->B(cur)->C->D
				if(strlen(set->cur->cur->path) < strlen(set->cur->cur->next->path)){
					set->cur->tmp1 = set->cur->cur->prev; //tmp1 = A
					set->cur->tmp2 = set->cur->cur->next; //tmp2 = C
					set->cur->tmp3 = set->cur->cur->next->next; // tmp3 = D

					set->cur->tmp1->next = set->cur->tmp2; // A->C
					set->cur->cur->next = set->cur->tmp3; // B->D
					set->cur->tmp2->next = set->cur->cur; // C->B

					set->cur->tmp2->prev = set->cur->tmp1; // C's prev = A
					set->cur->tmp3->prev = set->cur->cur; // D's prev = B
					set->cur->cur->prev = set->cur->tmp2; // B's prev = C
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			if(check==0){
				set->cur->tmp_cur = set->cur->tmp_cur->next;
				check=1;
			}	
		}
		set->cur = set->cur->next;
	}
	return;

}

void up_sort_uid(Set* set){
	struct stat st1;
	struct stat st2;
	int check=0;
	set->cur = set->front;
	for(int i=0; i< set->cur->count-1; i++){
		set->cur->cur = set->cur->front->next;
		set->cur->tmp_cur = set->cur->rear;
		while(set->cur->cur != set->cur->tmp_cur){
			if(set->cur->cur == set->cur->front){ // in cur, first case : A(cur, front)->B->C
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_uid > st2.st_uid){
					set->cur->tmp1 = set->cur->front->next; // tmp1 = B
					set->cur->tmp2 = set->cur->tmp1->next; // tmp2 = C

					set->cur->cur->next = set->cur->tmp2; // A->C
					set->cur->tmp1->next = set->cur->front; // B->A

					set->cur->tmp2->prev = set->cur->cur; // C's prev = A
					set->cur->cur->prev = set->cur->tmp1; // A's prev = B
					set->cur->tmp1->prev = NULL;

					set->cur->front = set->cur->tmp1; // move 'front' to B
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else if(set->cur->cur->next == set->cur->rear){ // in cur, last case : B->C(cur)->D(rear)
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_uid > st2.st_uid){
					check = 1;
					set->cur->tmp1 = set->cur->cur->prev; // tmp1 = B
					set->cur->tmp1->next = set->cur->rear; // B->D
					set->cur->cur->next = NULL; // C->NULL
					set->cur->rear->next = set->cur->cur; // D->C

					set->cur->rear->prev = set->cur->tmp1; // D's prev = B
					set->cur->cur->prev = set->cur->rear; // C's prev = D

					set->cur->rear = set->cur->cur; // move 'rear' to C
					set->cur->tmp_cur = set->cur->cur;
					set->cur->tmp_cur->prev = set->cur->cur->prev;
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else{ // other case : A->B(cur)->C->D
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_uid > st2.st_uid){
					set->cur->tmp1 = set->cur->cur->prev; //tmp1 = A
					set->cur->tmp2 = set->cur->cur->next; //tmp2 = C
					set->cur->tmp3 = set->cur->cur->next->next; // tmp3 = D

					set->cur->tmp1->next = set->cur->tmp2; // A->C
					set->cur->cur->next = set->cur->tmp3; // B->D
					set->cur->tmp2->next = set->cur->cur; // C->B

					set->cur->tmp2->prev = set->cur->tmp1; // C's prev = A
					set->cur->tmp3->prev = set->cur->cur; // D's prev = B
					set->cur->cur->prev = set->cur->tmp2; // B's prev = C
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			if(check==0){
				set->cur->tmp_cur = set->cur->tmp_cur->next;
				check=1;
			}	
		}
		set->cur = set->cur->next;
	}
	return;
	
}

void down_sort_uid(Set* set){
	struct stat st1;
	struct stat st2;
	int check=0;
	set->cur = set->front;
	for(int i=0; i< set->cur->count-1; i++){
		set->cur->cur = set->cur->front->next;
		set->cur->tmp_cur = set->cur->rear;
		while(set->cur->cur != set->cur->tmp_cur){
			if(set->cur->cur == set->cur->front){ // in cur, first case : A(cur, front)->B->C
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_uid < st2.st_uid){
					set->cur->tmp1 = set->cur->front->next; // tmp1 = B
					set->cur->tmp2 = set->cur->tmp1->next; // tmp2 = C

					set->cur->cur->next = set->cur->tmp2; // A->C
					set->cur->tmp1->next = set->cur->front; // B->A

					set->cur->tmp2->prev = set->cur->cur; // C's prev = A
					set->cur->cur->prev = set->cur->tmp1; // A's prev = B
					set->cur->tmp1->prev = NULL;

					set->cur->front = set->cur->tmp1; // move 'front' to B
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else if(set->cur->cur->next == set->cur->rear){ // in cur, last case : B->C(cur)->D(rear)
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_uid < st2.st_uid){
					check = 1;
					set->cur->tmp1 = set->cur->cur->prev; // tmp1 = B
					set->cur->tmp1->next = set->cur->rear; // B->D
					set->cur->cur->next = NULL; // C->NULL
					set->cur->rear->next = set->cur->cur; // D->C

					set->cur->rear->prev = set->cur->tmp1; // D's prev = B
					set->cur->cur->prev = set->cur->rear; // C's prev = D

					set->cur->rear = set->cur->cur; // move 'rear' to C
					set->cur->tmp_cur = set->cur->cur;
					set->cur->tmp_cur->prev = set->cur->cur->prev;
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else{ // other case : A->B(cur)->C->D
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_uid < st2.st_uid){
					set->cur->tmp1 = set->cur->cur->prev; //tmp1 = A
					set->cur->tmp2 = set->cur->cur->next; //tmp2 = C
					set->cur->tmp3 = set->cur->cur->next->next; // tmp3 = D

					set->cur->tmp1->next = set->cur->tmp2; // A->C
					set->cur->cur->next = set->cur->tmp3; // B->D
					set->cur->tmp2->next = set->cur->cur; // C->B

					set->cur->tmp2->prev = set->cur->tmp1; // C's prev = A
					set->cur->tmp3->prev = set->cur->cur; // D's prev = B
					set->cur->cur->prev = set->cur->tmp2; // B's prev = C
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			if(check==0){
				set->cur->tmp_cur = set->cur->tmp_cur->next;
				check=1;
			}	
		}
		set->cur = set->cur->next;
	}
	return;

}


void up_sort_gid(Set* set){
	struct stat st1;
	struct stat st2;
	int check=0;
	set->cur = set->front;
	for(int i=0; i< set->cur->count-1; i++){
		set->cur->cur = set->cur->front->next;
		set->cur->tmp_cur = set->cur->rear;
		while(set->cur->cur != set->cur->tmp_cur){
			if(set->cur->cur == set->cur->front){ // in cur, first case : A(cur, front)->B->C
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_gid > st2.st_gid){
					set->cur->tmp1 = set->cur->front->next; // tmp1 = B
					set->cur->tmp2 = set->cur->tmp1->next; // tmp2 = C

					set->cur->cur->next = set->cur->tmp2; // A->C
					set->cur->tmp1->next = set->cur->front; // B->A

					set->cur->tmp2->prev = set->cur->cur; // C's prev = A
					set->cur->cur->prev = set->cur->tmp1; // A's prev = B
					set->cur->tmp1->prev = NULL;

					set->cur->front = set->cur->tmp1; // move 'front' to B
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else if(set->cur->cur->next == set->cur->rear){ // in cur, last case : B->C(cur)->D(rear)
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_gid > st2.st_gid){
					check = 1;
					set->cur->tmp1 = set->cur->cur->prev; // tmp1 = B
					set->cur->tmp1->next = set->cur->rear; // B->D
					set->cur->cur->next = NULL; // C->NULL
					set->cur->rear->next = set->cur->cur; // D->C

					set->cur->rear->prev = set->cur->tmp1; // D's prev = B
					set->cur->cur->prev = set->cur->rear; // C's prev = D

					set->cur->rear = set->cur->cur; // move 'rear' to C
					set->cur->tmp_cur = set->cur->cur;
					set->cur->tmp_cur->prev = set->cur->cur->prev;
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else{ // other case : A->B(cur)->C->D
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_gid > st2.st_gid){
					set->cur->tmp1 = set->cur->cur->prev; //tmp1 = A
					set->cur->tmp2 = set->cur->cur->next; //tmp2 = C
					set->cur->tmp3 = set->cur->cur->next->next; // tmp3 = D

					set->cur->tmp1->next = set->cur->tmp2; // A->C
					set->cur->cur->next = set->cur->tmp3; // B->D
					set->cur->tmp2->next = set->cur->cur; // C->B

					set->cur->tmp2->prev = set->cur->tmp1; // C's prev = A
					set->cur->tmp3->prev = set->cur->cur; // D's prev = B
					set->cur->cur->prev = set->cur->tmp2; // B's prev = C
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			if(check==0){
				set->cur->tmp_cur = set->cur->tmp_cur->next;
				check=1;
			}	
		}
		set->cur = set->cur->next;
	}
	return;

}

void down_sort_gid(Set* set){
	struct stat st1;
	struct stat st2;
	int check=0;
	set->cur = set->front;
	for(int i=0; i< set->cur->count-1; i++){
		set->cur->cur = set->cur->front->next;
		set->cur->tmp_cur = set->cur->rear;
		while(set->cur->cur != set->cur->tmp_cur){
			if(set->cur->cur == set->cur->front){ // in cur, first case : A(cur, front)->B->C
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_gid < st2.st_gid){
					set->cur->tmp1 = set->cur->front->next; // tmp1 = B
					set->cur->tmp2 = set->cur->tmp1->next; // tmp2 = C

					set->cur->cur->next = set->cur->tmp2; // A->C
					set->cur->tmp1->next = set->cur->front; // B->A

					set->cur->tmp2->prev = set->cur->cur; // C's prev = A
					set->cur->cur->prev = set->cur->tmp1; // A's prev = B
					set->cur->tmp1->prev = NULL;

					set->cur->front = set->cur->tmp1; // move 'front' to B
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else if(set->cur->cur->next == set->cur->rear){ // in cur, last case : B->C(cur)->D(rear)
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_gid < st2.st_gid){
					check = 1;
					set->cur->tmp1 = set->cur->cur->prev; // tmp1 = B
					set->cur->tmp1->next = set->cur->rear; // B->D
					set->cur->cur->next = NULL; // C->NULL
					set->cur->rear->next = set->cur->cur; // D->C

					set->cur->rear->prev = set->cur->tmp1; // D's prev = B
					set->cur->cur->prev = set->cur->rear; // C's prev = D

					set->cur->rear = set->cur->cur; // move 'rear' to C
					set->cur->tmp_cur = set->cur->cur;
					set->cur->tmp_cur->prev = set->cur->cur->prev;
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else{ // other case : A->B(cur)->C->D
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_gid < st2.st_gid){
					set->cur->tmp1 = set->cur->cur->prev; //tmp1 = A
					set->cur->tmp2 = set->cur->cur->next; //tmp2 = C
					set->cur->tmp3 = set->cur->cur->next->next; // tmp3 = D

					set->cur->tmp1->next = set->cur->tmp2; // A->C
					set->cur->cur->next = set->cur->tmp3; // B->D
					set->cur->tmp2->next = set->cur->cur; // C->B

					set->cur->tmp2->prev = set->cur->tmp1; // C's prev = A
					set->cur->tmp3->prev = set->cur->cur; // D's prev = B
					set->cur->cur->prev = set->cur->tmp2; // B's prev = C
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			if(check==0){
				set->cur->tmp_cur = set->cur->tmp_cur->next;
				check=1;
			}	
		}
		set->cur = set->cur->next;
	}
	return;

}

void up_sort_mode(Set* set){
	struct stat st1;
	struct stat st2;
	int check=0;
	set->cur = set->front;
	for(int i=0; i< set->cur->count-1; i++){
		set->cur->cur = set->cur->front->next;
		set->cur->tmp_cur = set->cur->rear;
		while(set->cur->cur != set->cur->tmp_cur){
			if(set->cur->cur == set->cur->front){ // in cur, first case : A(cur, front)->B->C
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_mode > st2.st_mode){
					set->cur->tmp1 = set->cur->front->next; // tmp1 = B
					set->cur->tmp2 = set->cur->tmp1->next; // tmp2 = C

					set->cur->cur->next = set->cur->tmp2; // A->C
					set->cur->tmp1->next = set->cur->front; // B->A

					set->cur->tmp2->prev = set->cur->cur; // C's prev = A
					set->cur->cur->prev = set->cur->tmp1; // A's prev = B
					set->cur->tmp1->prev = NULL;

					set->cur->front = set->cur->tmp1; // move 'front' to B
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else if(set->cur->cur->next == set->cur->rear){ // in cur, last case : B->C(cur)->D(rear)
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_mode > st2.st_mode){
					check = 1;
					set->cur->tmp1 = set->cur->cur->prev; // tmp1 = B
					set->cur->tmp1->next = set->cur->rear; // B->D
					set->cur->cur->next = NULL; // C->NULL
					set->cur->rear->next = set->cur->cur; // D->C

					set->cur->rear->prev = set->cur->tmp1; // D's prev = B
					set->cur->cur->prev = set->cur->rear; // C's prev = D

					set->cur->rear = set->cur->cur; // move 'rear' to C
					set->cur->tmp_cur = set->cur->cur;
					set->cur->tmp_cur->prev = set->cur->cur->prev;
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else{ // other case : A->B(cur)->C->D
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_mode > st2.st_mode){
					set->cur->tmp1 = set->cur->cur->prev; //tmp1 = A
					set->cur->tmp2 = set->cur->cur->next; //tmp2 = C
					set->cur->tmp3 = set->cur->cur->next->next; // tmp3 = D

					set->cur->tmp1->next = set->cur->tmp2; // A->C
					set->cur->cur->next = set->cur->tmp3; // B->D
					set->cur->tmp2->next = set->cur->cur; // C->B

					set->cur->tmp2->prev = set->cur->tmp1; // C's prev = A
					set->cur->tmp3->prev = set->cur->cur; // D's prev = B
					set->cur->cur->prev = set->cur->tmp2; // B's prev = C
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			if(check==0){
				set->cur->tmp_cur = set->cur->tmp_cur->next;
				check=1;
			}	
		}
		set->cur = set->cur->next;
	}
	return;
	
}


void down_sort_mode(Set* set){
	struct stat st1;
	struct stat st2;
	int check=0;
	set->cur = set->front;
	for(int i=0; i< set->cur->count-1; i++){
		set->cur->cur = set->cur->front->next;
		set->cur->tmp_cur = set->cur->rear;
		while(set->cur->cur != set->cur->tmp_cur){
			if(set->cur->cur == set->cur->front){ // in cur, first case : A(cur, front)->B->C
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_mode < st2.st_mode){
					set->cur->tmp1 = set->cur->front->next; // tmp1 = B
					set->cur->tmp2 = set->cur->tmp1->next; // tmp2 = C

					set->cur->cur->next = set->cur->tmp2; // A->C
					set->cur->tmp1->next = set->cur->front; // B->A

					set->cur->tmp2->prev = set->cur->cur; // C's prev = A
					set->cur->cur->prev = set->cur->tmp1; // A's prev = B
					set->cur->tmp1->prev = NULL;

					set->cur->front = set->cur->tmp1; // move 'front' to B
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else if(set->cur->cur->next == set->cur->rear){ // in cur, last case : B->C(cur)->D(rear)
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_mode < st2.st_mode){
					check = 1;
					set->cur->tmp1 = set->cur->cur->prev; // tmp1 = B
					set->cur->tmp1->next = set->cur->rear; // B->D
					set->cur->cur->next = NULL; // C->NULL
					set->cur->rear->next = set->cur->cur; // D->C

					set->cur->rear->prev = set->cur->tmp1; // D's prev = B
					set->cur->cur->prev = set->cur->rear; // C's prev = D

					set->cur->rear = set->cur->cur; // move 'rear' to C
					set->cur->tmp_cur = set->cur->cur;
					set->cur->tmp_cur->prev = set->cur->cur->prev;
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else{ // other case : A->B(cur)->C->D
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_mode < st2.st_mode){
					set->cur->tmp1 = set->cur->cur->prev; //tmp1 = A
					set->cur->tmp2 = set->cur->cur->next; //tmp2 = C
					set->cur->tmp3 = set->cur->cur->next->next; // tmp3 = D

					set->cur->tmp1->next = set->cur->tmp2; // A->C
					set->cur->cur->next = set->cur->tmp3; // B->D
					set->cur->tmp2->next = set->cur->cur; // C->B

					set->cur->tmp2->prev = set->cur->tmp1; // C's prev = A
					set->cur->tmp3->prev = set->cur->cur; // D's prev = B
					set->cur->cur->prev = set->cur->tmp2; // B's prev = C
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			if(check==0){
				set->cur->tmp_cur = set->cur->tmp_cur->next;
				check=1;
			}	
		}
		set->cur = set->cur->next;
	}
	return;

}

void up_sort_size_filelist(Set* set){
	struct stat st1;
	struct stat st2;
	int check=0;
	set->cur = set->front;
	for(int i=0; i< set->cur->count-1; i++){
		set->cur->cur = set->cur->front->next;
		set->cur->tmp_cur = set->cur->rear;
		while(set->cur->cur != set->cur->tmp_cur){
			if(set->cur->cur == set->cur->front){ // in cur, first case : A(cur, front)->B->C
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_size > st2.st_size){
					set->cur->tmp1 = set->cur->front->next; // tmp1 = B
					set->cur->tmp2 = set->cur->tmp1->next; // tmp2 = C

					set->cur->cur->next = set->cur->tmp2; // A->C
					set->cur->tmp1->next = set->cur->front; // B->A

					set->cur->tmp2->prev = set->cur->cur; // C's prev = A
					set->cur->cur->prev = set->cur->tmp1; // A's prev = B
					set->cur->tmp1->prev = NULL;

					set->cur->front = set->cur->tmp1; // move 'front' to B
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else if(set->cur->cur->next == set->cur->rear){ // in cur, last case : B->C(cur)->D(rear)
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_size > st2.st_size){
					check = 1;
					set->cur->tmp1 = set->cur->cur->prev; // tmp1 = B
					set->cur->tmp1->next = set->cur->rear; // B->D
					set->cur->cur->next = NULL; // C->NULL
					set->cur->rear->next = set->cur->cur; // D->C

					set->cur->rear->prev = set->cur->tmp1; // D's prev = B
					set->cur->cur->prev = set->cur->rear; // C's prev = D

					set->cur->rear = set->cur->cur; // move 'rear' to C
					set->cur->tmp_cur = set->cur->cur;
					set->cur->tmp_cur->prev = set->cur->cur->prev;
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else{ // other case : A->B(cur)->C->D
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_size > st2.st_size){
					set->cur->tmp1 = set->cur->cur->prev; //tmp1 = A
					set->cur->tmp2 = set->cur->cur->next; //tmp2 = C
					set->cur->tmp3 = set->cur->cur->next->next; // tmp3 = D

					set->cur->tmp1->next = set->cur->tmp2; // A->C
					set->cur->cur->next = set->cur->tmp3; // B->D
					set->cur->tmp2->next = set->cur->cur; // C->B

					set->cur->tmp2->prev = set->cur->tmp1; // C's prev = A
					set->cur->tmp3->prev = set->cur->cur; // D's prev = B
					set->cur->cur->prev = set->cur->tmp2; // B's prev = C
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			if(check==0){
				set->cur->tmp_cur = set->cur->tmp_cur->next;
				check=1;
			}	
		}
		set->cur = set->cur->next;
	}
	return;


}
void down_sort_size_filelist(Set* set){
	struct stat st1;
	struct stat st2;
	int check=0;
	set->cur = set->front;
	for(int i=0; i< set->cur->count-1; i++){
		set->cur->cur = set->cur->front->next;
		set->cur->tmp_cur = set->cur->rear;
		while(set->cur->cur != set->cur->tmp_cur){
			if(set->cur->cur == set->cur->front){ // in cur, first case : A(cur, front)->B->C
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_size < st2.st_size){
					set->cur->tmp1 = set->cur->front->next; // tmp1 = B
					set->cur->tmp2 = set->cur->tmp1->next; // tmp2 = C

					set->cur->cur->next = set->cur->tmp2; // A->C
					set->cur->tmp1->next = set->cur->front; // B->A

					set->cur->tmp2->prev = set->cur->cur; // C's prev = A
					set->cur->cur->prev = set->cur->tmp1; // A's prev = B
					set->cur->tmp1->prev = NULL;

					set->cur->front = set->cur->tmp1; // move 'front' to B
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else if(set->cur->cur->next == set->cur->rear){ // in cur, last case : B->C(cur)->D(rear)
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_size < st2.st_size){
					check = 1;
					set->cur->tmp1 = set->cur->cur->prev; // tmp1 = B
					set->cur->tmp1->next = set->cur->rear; // B->D
					set->cur->cur->next = NULL; // C->NULL
					set->cur->rear->next = set->cur->cur; // D->C

					set->cur->rear->prev = set->cur->tmp1; // D's prev = B
					set->cur->cur->prev = set->cur->rear; // C's prev = D

					set->cur->rear = set->cur->cur; // move 'rear' to C
					set->cur->tmp_cur = set->cur->cur;
					set->cur->tmp_cur->prev = set->cur->cur->prev;
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			else{ // other case : A->B(cur)->C->D
				lstat(set->cur->cur->path, &st1);
				lstat(set->cur->cur->next->path, &st2);
				if(st1.st_size < st2.st_size){
					set->cur->tmp1 = set->cur->cur->prev; //tmp1 = A
					set->cur->tmp2 = set->cur->cur->next; //tmp2 = C
					set->cur->tmp3 = set->cur->cur->next->next; // tmp3 = D

					set->cur->tmp1->next = set->cur->tmp2; // A->C
					set->cur->cur->next = set->cur->tmp3; // B->D
					set->cur->tmp2->next = set->cur->cur; // C->B

					set->cur->tmp2->prev = set->cur->tmp1; // C's prev = A
					set->cur->tmp3->prev = set->cur->cur; // D's prev = B
					set->cur->cur->prev = set->cur->tmp2; // B's prev = C
				}
				else
					set->cur->cur = set->cur->cur->next;
			}
			if(check==0){
				set->cur->tmp_cur = set->cur->tmp_cur->next;
				check=1;
			}	
		}
		set->cur = set->cur->next;
	}
	return;


}


void delete_prompt(Set* set){
	
	int opt;
	char* param_opt;
	char input[BUF_MAX];
	int arg_cnt = 0;
	char* arg_vec[ARG_MAX];
	int SET_IDX;
	int LIST_IDX;
	int del_option[4] = {0,0,0,0};
	char dotTrash[PATH_MAX-128];
	memset(homedir_path, 0, PATH_MAX-256);
	memset(logfile_path, 0, PATH_MAX);
	memset(trashdir_path, 0, PATH_MAX);
	memset(trashinfo_path, 0, PATH_MAX);
	memset(dotTrash, 0, PATH_MAX-128);
	
	sprintf(homedir_path, "%s", getenv("HOME")); // get Home directory path
	sprintf(logfile_path, "%s/%s", homedir_path, ".duplicate_20182613.log"); // get logfile path
	sprintf(trashdir_path, "%s/.Trash/files", homedir_path); // get trash directory path
	sprintf(trashinfo_path, "%s/.Trash/info", homedir_path); // get trash info path
	sprintf(dotTrash, "%s/.Trash", homedir_path);

	if(access(dotTrash, F_OK) < 0)
		mkdir(dotTrash, 0755);
	if(access(trashdir_path, F_OK) < 0)
		mkdir(trashdir_path, 0755);
	if(access(trashinfo_path, F_OK) < 0)
		mkdir(trashinfo_path, 0755);

	while(1){
		fflush(stdin);
		for(int i=0; i<4; i++)
			del_option[i] = 0;
		
		printf(">> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input)-1] = '\0';
		arg_cnt = split(input, " ", arg_vec);
		optind = 1; // reuse getopt()

		if(arg_cnt == 0)
			continue;
		if(!strcmp(arg_vec[0], "exit")){
			printf(">> Back to Prompt\n");
			return;
		}
		if(strcmp(arg_vec[0], "delete") || arg_cnt < 4 || arg_cnt > 5){
			printf("delete prompt usage :\n>> delete -l [SET_IDX] -d[LIST_IDX] -i -f -t\n");
			continue;		
		}


	
		while((opt = getopt(arg_cnt, arg_vec, "l:d:ift")) != -1){
			switch(opt){
				case 'l':
					SET_IDX = atoi(optarg);
					break;
	
				case 'd':
					LIST_IDX = atoi(optarg);
					del_option[0] = 1;
					break;
	
				case 'i':
					del_option[1] = 1;
					break;
	
				case 'f':
					del_option[2] = 1;
					break;
	
				case 't':
					del_option[3] = 1;
					break;
	
				case '?':
					
				default:
					break;
			}
		}
		if(del_option[0]){ // -d
			if(arg_cnt != 5)
				continue;
			d_opt(SET_IDX, LIST_IDX, set);
		}
		else if(del_option[1]){ // -i
			if(arg_cnt != 4)
				continue;
			i_opt(SET_IDX, set);
		}
		else if(del_option[2]){ // -f
			if(arg_cnt != 4)
				continue;
			f_opt(SET_IDX, set);
		}
		else if(del_option[3]){ // -t
			if(arg_cnt != 4)
				continue;
			t_opt(SET_IDX, set);
		}

		print_dupSet(set);
	}
}


void delete_d(int SET_IDX, int LIST_IDX, Set* set){
	int set_cnt=1;
	int list_cnt=1;
	set->cur = set->front;
	while(set_cnt<SET_IDX){
		set->cur = set->cur->next;
		set_cnt++;
	}

	set->cur->cur = set->cur->front;
	while(list_cnt<LIST_IDX){
		set->cur->cur = set->cur->cur->next;
		list_cnt++;
	}
	
	if(unlink(set->cur->cur->path) < 0){
		fprintf(stderr, "unlink error\n");
		return;
	}
	printf("\"%s\" has been deleted in #%d\n\n", set->cur->cur->path, SET_IDX);

	int fd;
	if((fd=open(logfile_path, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0){
		fprintf(stderr, "open error\n");
		exit(1);
	}

	char buf[PATH_MAX*2];
	memset(buf, 0, PATH_MAX*2);
	
	time_t t = time(NULL);
	struct tm lt;
	localtime_r(&t, &lt);
	char ntime[TIME_BUF];
	memset(ntime, 0, TIME_BUF);
	struct passwd* pwd;
	pwd = getpwuid(getuid());

	sprintf(ntime, "%d-%02d-%02d %02d:%02d:%02d", lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);

	sprintf(buf, "%s %s %s %s\n", "[DELETE]", set->cur->cur->path, ntime, pwd->pw_name);
	
	write(fd, buf, strlen(buf));
	close(fd);

	Node* tmp;
	if(list_cnt == 1){ // delete first node
		tmp = set->cur->front;
		set->cur->front = tmp->next;
		tmp->next->prev = NULL;
		set->cur->count--;
		free(tmp);
	}
	else if(list_cnt == set->cur->count){ // delete last node
		tmp = set->cur->rear;
		set->cur->rear = tmp->prev;
		tmp->prev->next = NULL;
		set->cur->count--;
		free(tmp);
	}
	else{ // else
		tmp = set->cur->cur;
		tmp->next->prev = tmp->prev;
		tmp->prev->next = tmp->next;
		set->cur->count--;
		free(tmp);
	}

	if(set->cur->count == 1){
		delete_List(set->cur);
		set->count--;
	}
	return;
}

void d_opt(int SET_IDX, int LIST_IDX, Set* set){
	if(SET_IDX < 0 || LIST_IDX < 0){
		fprintf(stderr, "[INDEX] input error(non-negative)\n");
		return;
	}
	delete_d(SET_IDX, LIST_IDX, set);
	return;
}


void delete_i(int SET_IDX, Set* set){
	int set_cnt=1;
	int list_cnt=1;
	int input;
	Node* tmp;
	int fd;

	time_t t;
	struct tm lt;

	char buf[PATH_MAX*2];
	char ntime[TIME_BUF];
	struct passwd* pwd;
	pwd = getpwuid(getuid());

	set->cur = set->front;
	while(set_cnt<SET_IDX){
		set->cur = set->cur->next;
		set_cnt++;
	}

	set->cur->cur = set->cur->front;
	while(set->cur->cur != NULL){

		memset(buf, 0, PATH_MAX*2);
		memset(ntime, 0, TIME_BUF);

		printf("Delete \"%s\"? [y/n] ", set->cur->cur->path);
		input = getc(stdin);
		t = time(NULL);
		localtime_r(&t, &lt);

		if(set->cur->cur->prev == NULL){
			if(input == 'n' || input == 'N'){
				set->cur->cur = set->cur->cur->next;
			}
			else if(input == 'y' || input == 'Y'){
				if(unlink(set->cur->cur->path) < 0){
					fprintf(stderr, "unlink error\n");
					return;
				}	
				if((fd=open(logfile_path, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0){
					fprintf(stderr, "open error\n");
					exit(1);
				}
				sprintf(ntime, "%d-%02d-%02d %02d:%02d:%02d", lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);			
				sprintf(buf, "%s %s %s %s\n", "[DELETE]", set->cur->cur->path, ntime, pwd->pw_name);
				write(fd, buf, strlen(buf));
				close(fd);
	
				tmp = set->cur->front;
				set->cur->front = tmp->next;
				tmp->next->prev = NULL;
				set->cur->count--;
				free(tmp);
				set->cur->cur = set->cur->cur->next;
			}
			else{
				printf("Input error (y/n)\n");
				return;
			}
		}
		else if(set->cur->cur->next == NULL){
			if(input == 'n' || input == 'N'){
				set->cur->cur = set->cur->cur->next;
			}
			else if(input == 'y' || input == 'Y'){
				if(unlink(set->cur->cur->path) < 0){
					fprintf(stderr, "unlink error\n");
					return;
				}
				if((fd=open(logfile_path, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0){
					fprintf(stderr, "open error\n");
					exit(1);
				}
				sprintf(ntime, "%d-%02d-%02d %02d:%02d:%02d", lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);			
				sprintf(buf, "%s %s %s %s\n", "[DELETE]", set->cur->cur->path, ntime, pwd->pw_name);
				write(fd, buf, strlen(buf));
				close(fd);

				tmp = set->cur->rear;
				set->cur->rear = tmp->prev;
				tmp->prev->next = NULL;
				set->cur->count--;
				free(tmp);
				set->cur->cur = set->cur->cur->next;
			}
			else{
				printf("Input error (y/n)\n");
				return;
			}
		}
		else{
			if(input == 'n' || input == 'N'){
				set->cur->cur = set->cur->cur->next;
			}
			else if(input == 'y' || input == 'Y'){
				if(unlink(set->cur->cur->path) < 0){
					fprintf(stderr, "unlink error\n");
					return;
				}
				if((fd=open(logfile_path, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0){
					fprintf(stderr, "open error\n");
					exit(1);
				}
				sprintf(ntime, "%d-%02d-%02d %02d:%02d:%02d", lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);			
				sprintf(buf, "%s %s %s %s\n", "[DELETE]", set->cur->cur->path, ntime, pwd->pw_name);
				write(fd, buf, strlen(buf));
				close(fd);

				tmp = set->cur->cur;
				tmp->next->prev = tmp->prev;
				tmp->prev->next = tmp->next;
				set->cur->count--;
				free(tmp);
				set->cur->cur = set->cur->cur->next;
			}
			else{
				printf("Input error (y/n)\n");
				return;
			}
		}
		while(getchar() != '\n')
			continue;
	}
	if(set->cur->count == 1){
		delete_List(set->cur);
		set->count--;
	}
	return;
}

void i_opt(int SET_IDX, Set* set){
	if(SET_IDX < 0){
		fprintf(stderr, "[INDEX] input error(non-negative)\n");
		return;
	}
	delete_i(SET_IDX, set);
	return;
}

void delete_f(int SET_IDX, int REC_IDX, Set* set){
	struct stat st;
	int set_cnt = 1;
	int i = 1;
	
	int input;
	int fd;
	time_t t;
	struct tm lt;
	char buf[PATH_MAX*2];
	char ntime[TIME_BUF];
	struct passwd* pwd;
	pwd = getpwuid(getuid());

	set->cur = set->front;
	while(set_cnt<SET_IDX){
		set->cur = set->cur->next;
		set_cnt++;
	}

	set->cur->cur = set->cur->front;
	while(set->cur->cur != NULL){
		memset(buf, 0, PATH_MAX*2);
		memset(ntime, 0, TIME_BUF);

		if(i == REC_IDX){
			lstat(set->cur->cur->path, &st);
			time_t mt = st.st_mtime;
			struct tm mT;
			localtime_r(&mt, &mT);
			printf("Left file in #%d : %s (%d-%02d-%02d %02d:%02d:%02d)\n\n", SET_IDX+1, set->cur->cur->path,
					mT.tm_year+1900, mT.tm_mon+1, mT.tm_mday+1, mT.tm_hour, mT.tm_hour, mT.tm_min);
		}
		else{
			if(unlink(set->cur->cur->path) < 0){
				fprintf(stderr, "unlink error\n");
				return;
			}
			if((fd=open(logfile_path, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0){
				fprintf(stderr, "open error\n");
				exit(1);
			}

			t = time(NULL);
			localtime_r(&t, &lt);
			sprintf(ntime, "%d-%02d-%02d %02d:%02d:%02d", lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);			
			sprintf(buf, "%s %s %s %s\n", "[DELETE]", set->cur->cur->path, ntime, pwd->pw_name);
			write(fd, buf, strlen(buf));
			close(fd);

			set->cur->count--;
		}
		i++;
		set->cur->cur = set->cur->cur->next;
	}
	delete_List(set->cur);
	set->count--;
	return;
}

void f_opt(int SET_IDX, Set* set){
	if(SET_IDX < 0){
		fprintf(stderr, "[INDEX] input error(non-negative)\n");
		return;
	}
	int REC_IDX = get_recentIDX(SET_IDX, set);
	delete_f(SET_IDX, REC_IDX, set);
	return;
}

void delete_t(int SET_IDX, int REC_IDX, Set* set){
	struct stat st;
	int set_cnt = 1;
	int i=1;
	
	int input;
	int log_fd;
	int trash_fd;
	time_t t;
	struct tm lt;
	char buf[PATH_MAX*2];
	char ntime[TIME_BUF];
	struct passwd* pwd;
	pwd = getpwuid(getuid());

	struct dirent** namelist;
	char newname[PATH_MAX*2];
	char trashinfofile[PATH_MAX+256];
	set->cur = set->front;
	while(set_cnt<SET_IDX){
		set->cur = set->cur->next;
		set_cnt++;
	}

	set->cur->cur = set->cur->front;
	while(set->cur->cur != NULL){
		memset(buf, 0, PATH_MAX*2);
		memset(ntime, 0, TIME_BUF);
		
		if(i == REC_IDX){
			lstat(set->cur->cur->path, &st);
			time_t mt = st.st_mtime;
			struct tm mT;
			localtime_r(&mt, &mT);
			printf("All files in #%d have moved to Trash except \"%s\" (%d-%02d-%02d %02d:%02d:%02d)\n\n", SET_IDX+1, set->cur->cur->path,
					mT.tm_year+1900, mT.tm_mon+1, mT.tm_mday+1, mT.tm_hour, mT.tm_hour, mT.tm_min);
		}
		else{
			memset(newname, 0, PATH_MAX*2);
			sprintf(newname, "%s/%s", trashdir_path, strrchr(set->cur->cur->path, '/')+1);
			if(rename(set->cur->cur->path, newname) < 0){ // move to trash directory
				fprintf(stderr, "rename error\n");
				return;
			}


			memset(trashinfofile, 0, PATH_MAX+256);
			sprintf(trashinfofile, "%s/%s", trashinfo_path, strrchr(set->cur->cur->path, '/')+1); // new info filename
			printf("trashinfofile : %s\n", trashinfofile);
			if((trash_fd = open(trashinfofile, O_WRONLY | O_CREAT)) < 0){
				fprintf(stderr, "open error 1\n");
				exit(1);
			}
			write(trash_fd, set->cur->cur->path, strlen(set->cur->cur->path)); // write to trash info file
			

			if((log_fd = open(logfile_path, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0){ // log file
				fprintf(stderr, "open error 2\n");
				exit(1);
			}
			t = time(NULL);
			localtime_r(&t, &lt);
			sprintf(ntime, "%d-%02d-%02d %02d:%02d:%02d", lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);			
			sprintf(buf, "%s %s %s %s\n", "[REMOVE]", set->cur->cur->path, ntime, pwd->pw_name);
			write(log_fd, buf, strlen(buf)); // write to log file
			write(trash_fd, ntime, strlen(ntime));
			close(trash_fd);
			close(log_fd);
		}
		i++;
		set->cur->cur = set->cur->cur->next;
	}

	delete_List(set->cur);
	set->cur->count--;
	return;
}

void t_opt(int SET_IDX, Set* set){
	if(SET_IDX < 0){
		fprintf(stderr, "[INDEX] input error(non-negative)\n");
		return;
	}
	int REC_IDX = get_recentIDX(SET_IDX, set);
	delete_t(SET_IDX, REC_IDX, set);
	return;
}

int get_recentIDX(int SET_IDX, Set* set){
	int set_cnt = 1;
	int REC_IDX = 1;
	int checkIDX = 1;
	
	set->cur = set->front;
	while(set_cnt<SET_IDX){
		set->cur = set->cur->next;
		set_cnt++;
	}

	struct stat st;
	set->cur->cur = set->cur->front->next;
	lstat(set->cur->front->path, &st);
	time_t REC_TIME = st.st_mtime;
	while(set->cur->cur != NULL){
		checkIDX++;
		lstat(set->cur->cur->path, &st);
		if(REC_TIME < st.st_mtime){
			REC_TIME = st.st_mtime;
			REC_IDX = checkIDX;
		}
		set->cur->cur = set->cur->cur->next;
	}
	return REC_IDX;
}

void list_opt(Set* set, int arg_cnt, char* arg_vec[]){	
	if(arg_cnt > 7){
		fprintf(stdout, "usage: list -l [LIST_TYPE] -c [CATEGORY] -o [ORDER]\n");
		return;
	}

	optind = 1;
	int opt;
	char* param_opt;
	char LIST_TYPE[10];
	char CATEGORY[10];
	int ORDER = 1;

	memset(LIST_TYPE, 0, 10);
	memset(CATEGORY, 0, 10);

	strcpy(LIST_TYPE, "fileset");
	strcpy(CATEGORY, "size");

	while((opt = getopt(arg_cnt, arg_vec, ":l:c:o:")) != -1){
		switch(opt){
			case 'l':
				strcpy(LIST_TYPE, optarg);
				break;

			case 'c':
				strcpy(CATEGORY, optarg);
				break;

			case 'o':
				 ORDER = atoi(optarg);
				break;

			case '?':
				return;

			default:
				return;
		}
	}
	for(int i=0; i<arg_cnt; i++)
		memset(arg_vec[i], 0, strlen(arg_vec[i]));

	if(ORDER == 1){
		if(LIST_TYPE == NULL || !strcmp(LIST_TYPE, "fileset")){ // default val : "fileset"
			if(CATEGORY == NULL || !strcmp(CATEGORY, "size")){ // default val : "size"
				up_sort_size(set);
			}
			else{
				fprintf(stdout, "wrong input\n");
				return;
			}
		}
		else if(!strcmp(LIST_TYPE, "filelist")){
			if(CATEGORY == NULL || !strcmp(CATEGORY, "size")) // default val : "size"
				up_sort_size_filelist(set);
			
			else if(!strcmp(CATEGORY, "filename"))
				up_sort_filename(set);
			
			else if(!strcmp(CATEGORY, "uid"))
				up_sort_uid(set);
			
			else if(!strcmp(CATEGORY, "gid"))
				up_sort_gid(set);

			else if(!strcmp(CATEGORY, "mode"))
				up_sort_mode(set);
			
			else{
				fprintf(stdout, "wrong input\n");
				return;
			}
		}
		else{ 
			fprintf(stdout, "wrong input\n");
			return;
		}
		printf("LISTTYPE : %s, CATEGORY : %s\n", LIST_TYPE, CATEGORY);

	}
	else if(ORDER == -1){
		if(LIST_TYPE == NULL || !strcmp(LIST_TYPE, "fileset")){ // default val : "fileset"
			if(CATEGORY == NULL || !strcmp(CATEGORY, "size")) // default val : "size"
				down_sort_size(set);
			
			else{
				fprintf(stdout, "wrong input\n");
				return;
			}
		}
		else if(!strcmp(LIST_TYPE, "filelist")){
			if(CATEGORY == NULL || !strcmp(CATEGORY, "size")) // default val : "size"
				down_sort_size_filelist(set);
			
			else if(!strcmp(CATEGORY, "filename"))
				down_sort_filename(set);
			
			else if(!strcmp(CATEGORY, "uid"))
				down_sort_uid(set);
			
			else if(!strcmp(CATEGORY, "gid"))
				down_sort_gid(set);
			
			else if(!strcmp(CATEGORY, "mode"))
				down_sort_mode(set);

			else{
				fprintf(stdout, "wrong input\n");
				return;
			}
		}
		else{ 
			fprintf(stdout, "wrong input\n");
			return;
		}

	}
	else{
		fprintf(stdout, "wrong input\n");
		return;
	}
	return;
}



void trash_opt(Set* set, int arg_cnt, char* arg_vec[]){	
	int opt;
	char filebuf[PATH_MAX*2];
	char readbuf[BUF_MAX];
	char tmpbuf[PATH_MAX*2];
	memset(filebuf, 0, PATH_MAX);
	memset(readbuf, 0, BUF_MAX);
	memset(tmpbuf, 0, BUF_MAX);
	struct stat st;
	struct dirent** namelist;
	if(arg_cnt > 5){
		fprintf(stdout, "usage: trash -c [CATEGORY] -o [ORDER]\n");
		return;
	}

	optind = 1;
	int fileCnt;
	char CATEGORY[10];
	int ORDER = 1;

	memset(CATEGORY, 0, 10);

	strcpy(CATEGORY, "filename");

	while((opt = getopt(arg_cnt, arg_vec, ":c:o:")) != -1){
		switch(opt){
			case 'c':
				strcpy(CATEGORY, optarg);
				break;

			case 'o':
				 ORDER = atoi(optarg);
				break;

			case '?':
				return;

			default:
				return;
		}
	}
	for(int i=0; i<arg_cnt; i++)
		memset(arg_vec[i], 0, strlen(arg_vec[i]));


	fileCnt = scandir(trashdir_path, &namelist, NULL,alphasort);
	printf("fileCnt : %d\n", fileCnt);	

	if(fileCnt == 0){
		printf("Trash bin is empty\n");
	}
	else{
		printf("	FILENAME				SIZE			DELETION DATE	DELETION TIME\n");
	
		for(int i=0; i<fileCnt; i++){
			if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")){
				continue;
			}
			sprintf(tmpbuf, "%s/%s/%s", trashinfo_path, "files", namelist[i]->d_name);
			printf("%s\n", tmpbuf);
			FILE* fp;
			if((fp=fopen(tmpbuf, "r")) < 0){
				fprintf(stderr, "open error\n");
				exit(1);
			}
			fgets(readbuf, BUF_MAX, fp);
			fclose(fp);
			printf("readbuf : %s\n", readbuf);
			sprintf(filebuf, "%s/%s", trashdir_path, namelist[i]->d_name);
			lstat(filebuf, &st);
//			printf("[ %d]%s		%d		%s")
			memset(filebuf, 0, PATH_MAX);
			
		}
	}

}


void restore_opt(Set* set, int RES_IDX){

}




























