#include "smallsh.h"
struct sigaction act_new;
sigjmp_buf buf;

static char inpbuf[MAXBUF];
char *prompt = "User Shell >> ";
static char symbol[] = { '&' ,'|', '>', '<'};

int userin(char** cmd);
bool issymbol(char c);
int symbolcnt(char symbol, char* cmd);
int tokcline(char* cmd, char* (**toks));
int runcommand(char *cline);
void runpipe(char* c, int i);
int runredir(char *cmd);
void runls(char* keyword);
int commonkeyword(char** keyword);
int runtab(char* cline, char** answer, int* tabcnt);
void sig_handler(int signo){
	if(signo == SIGINT || signo == SIGQUIT)
		siglongjmp(buf, 1);
}
int main()
{
	char* cmd;
	act_new.sa_handler = sig_handler;//시그널 핸들러 지정
	sigemptyset(&act_new.sa_mask);//시그널 처리 중 블록될 시그널은 없음
	sigaction(SIGINT, &act_new, NULL);
	sigaction(SIGQUIT, &act_new, NULL);
	if (!sigsetjmp(buf, 1)) printf("\n");
	else printf("\n");
	while (1) {
		if(userin(&cmd)==0) continue;
		if(symbolcnt('|', cmd)>0)
			runpipe(cmd, 0);
		else{
			runcommand(cmd);
		}
	}
}
int userin(char** cmd){ 
	int tabcnt=0;
	int c, count;
	memset (inpbuf, 0, MAXBUF);
	*cmd = inpbuf;
	printf("%s", prompt); 
	count = 0;
	while (1) {
		c =getch();
		if( c == '\t'){
			if(count==0) {printf("\033[128D%s", prompt);continue; }
			char* temp = (char *)malloc(sizeof(char)*strlen(*cmd)+32);
			int tabtype = runtab(*cmd, &temp, &tabcnt);
			strcpy(*cmd, temp);
			printf("\033[128D%s", prompt); 
			if(tabtype == -1) continue;
			printf("%s", inpbuf);
			if(tabtype  == 0||tabtype  == 1)
				continue;
			if(tabtype == 2||tabtype  == 3){
				count=strlen(inpbuf);
				continue;
			}
		}
		if(tabcnt == 10)
			tabcnt = 0;
		if (c == '\n' && count < MAXBUF){
            inpbuf[count] = '\0';
			if(strcmp(inpbuf, "q")==0){
				exit(1);
			}
            return count;
        }
        if (c == '\n') { //줄이 너무 길면 재시작한다.
            printf("smallsh: input line too long\n");
            count = 0;
            printf("%s",prompt);
        }
		
		if (count < MAXBUF){
			memset(inpbuf+count, 0, MAXBUF-count);
			if(issymbol(c)){
				inpbuf[count++] = ' ';
				inpbuf[count++] = c;
				inpbuf[count++] = ' ';
			}
            else inpbuf[count++] = c;
		}
	}
}
bool issymbol(char c) { 
	char *wrk;
	for (wrk = symbol; *wrk; wrk++)
	{
		if (c == *wrk)
			return true;
	}
	return false;
}
int symbolcnt(char symbol, char* cmd){
	int answer = 0;
	while((*cmd++)!='\0'){
		if(*cmd==symbol) ++answer;
	}
	return answer;
}
int tokcline(char* cmd, char* (**toks))
{
	char *cline = cmd;
	char *arg[MAXARG + 1]; 
	int narg = 0; 
	int type = FG; 
	
	arg[narg++] = strtok(cline, " ");
	while((arg[narg++] = strtok(NULL, " "))!=NULL);
	if (*(arg[narg-2])=='&'){
		type = BG;
		arg[narg-2] = NULL;
	}
	if(toks!=NULL)
		*toks = arg;
	return type;
}
int runcommand(char *cline)
{
	if(symbolcnt('>',cline)+symbolcnt('<',cline) > 0){
			return runredir(cline);
	}
	char** args;
	int where = tokcline(cline, &args);
	pid_t pid;
	int status;
	if ((pid = fork()) == -1) {
		perror("runcommand fork error");
		return(-1);
	}
	if(pid == 0){
		if(strcmp(*args,"ls")==0 && (args[1]==NULL||args[1][0]!='-'))
			runls(args[1]);
		else{
			execvp(*args, args);
			printf("command not found \n");
		}
		exit(1);
	}
	if (where == BG) {
		printf("[Process id %d]\n", pid);
		return pid;
	}
	if (waitpid(pid, &status, 0) == -1)
		return(-1);
	else
		return (status);
}
void runpipe(char* c, int i){
	int fd[2];
	pid_t pid;
	int pre_fd = i;
	char* cmd1 = strtok(c, "|");
	char* cmd2 = strtok(NULL, "\0");
	
	pipe(fd);
	if ((pid = fork()) == -1) {
		printf("fail to call fork()\n");
		exit(1);
	}
	if (pid == 0) {
		dup2(pre_fd, 0);
		if ( cmd2 != NULL){
			dup2(fd[1], 1);
		}
		close(fd[0]);
		runcommand(cmd1);
		exit(0);
	} else {
		wait(NULL);
		close(fd[1]);
		if(cmd2 != NULL)
			runpipe(cmd2, fd[0]);
	}
}
int runredir(char *cmd){
	char *resymbol[2] = {"<", ">"};
	int mode = 0;
	if(symbolcnt('>',cmd)>0)
		mode = 1;
	char* cline = strtok(cmd, resymbol[mode]);
	char* filename = strtok(NULL, "\0");
	filename = strtok(filename, " ");
	char * bg = strtok(NULL, " ");
	int where = FG;
	if(bg!=NULL&&strcmp(bg, "&")==0) where=BG;
	int fd;
	pid_t pid;
	int status;
	if((pid=fork())==-1)
		perror("redirection fork error");
	if(pid == 0){
		if(mode == 1)
			fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		else
			fd = open(filename, O_RDONLY, 0644);
		if (fd == -1) {
			perror("파일 open 오류");exit(1);
		}
		if (dup2(fd, mode) == -1) {
			perror("redirection dup error");
		}
		close(fd);
		runcommand(cline);
		exit(0);
	}
	if (where == BG) {
		printf("[Process id %d]\n", pid);
		return pid;
	}
	if (waitpid(pid, &status, 0) == -1)
		return(-1);
	else
		return (status);
}

void runls(char* keyword){
	int tabcnt = 0;
	DIR *dir;
    struct dirent *ent;
	char* filename = (char *)malloc(sizeof(char)*MAXBUF);
	char* nkey;
    dir = opendir ("./");
	if(keyword==NULL||((keyword)[0]=='.'&&(keyword)[1]=='/')){
		strtok(keyword, "./");
		nkey = strtok(NULL, "\0");
	}
	else
		nkey = strtok(keyword, "\0");
	char pattern[128] = "^";
	if(nkey!=NULL){
		int len = strlen(nkey);
		if(nkey[len-1]=='.'){
			nkey[len-1]='\\';
			strcat(nkey, ".");
		}
		strcat(pattern, nkey);
	}
	else 
		strcat(pattern, "[^.]");
	
	regex_t state;
	regcomp(&state, pattern, REG_EXTENDED);
    if (dir != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			memset(filename, 0, MAXBUF);
			strcpy(filename, ent->d_name);
			
			int status = regexec(&state, filename, 0, NULL, 0);
			
			if(status==0){
				printf("%s\t", filename);
				if(++tabcnt==5){	
					tabcnt=0;
					printf ("\n");
				}
			}
		}
		printf ("\n");
		closedir (dir);
    } else  perror ("EXIT_FAILURE");
}
int commonkeyword(char** keyword){
	DIR *dir;
    struct dirent *ent;
	int searchcnt = 0;
	int thisdir = 0;
	char* temp  = (char *)malloc(sizeof(char)*MAXBUF);
	strcpy(temp,"./");
	char* searchresult = (char *)malloc(sizeof(char)*1024);
    dir = opendir ("./");
	if((*keyword)[0]=='.'&&(*keyword)[1]=='/'){
		thisdir =1;
		 *keyword = strtok(*keyword, "./");
	}
    if (dir != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			int isprint = 0;
			char* filename = (char *)malloc(sizeof(char)*strlen(ent->d_name)+1);
			memcpy(filename, ent->d_name, strlen(ent->d_name)+1);
			if(strcmp(filename, ".")==0||strcmp(filename, "..")==0) continue;
			if(*keyword!=NULL) {
				char* tt = strstr ( filename, *keyword );
				if(tt!=NULL&& *tt==*filename) isprint = 1;
			}
			else isprint = 1;
			
			if(isprint){
				if(searchcnt==0)
					strcpy(searchresult, filename);
				else{
					strcat(searchresult, "\t");
					strcat(searchresult, filename);
				}
				searchcnt++;
			}
			free(filename);
		}
		closedir (dir);
		if(searchcnt==0) return 0;
		else if(searchcnt==1) {
			if(thisdir==1){
				strcat(temp, searchresult);
				strcpy(*keyword, temp);
			}
			else 
				strcpy(*keyword, searchresult);
			return 1;
		}
		else{
			int i=0;
			char keywords[searchcnt][128];
			char *ptr = strtok(searchresult, "\t"); 
			stpcpy(keywords[i++], ptr);
			int minlenidx = 0;
			while (ptr = strtok(NULL, "\t"))
			{
				stpcpy(keywords[i], ptr);
				if(strlen(keywords[minlenidx])>strlen(keywords[i]))
					minlenidx = i;
				i++;
			}
			for(i=strlen(*keyword)-1; i<strlen(keywords[minlenidx]); i++){
				char c0 = keywords[minlenidx][i];
				for(int j=0; j<searchcnt; j++){
					if(c0!=keywords[j][i]){
						char* result = (char *)malloc(sizeof(char)*(i+1));
						strncpy(result, keywords[minlenidx], i);
						result[i]='\0';
						if(thisdir==1){
							strcat(temp, result);
							strcpy(*keyword, temp);
						}
						else stpcpy(*keyword, result);
						
						return 1;
					}
				}
			}
			if(thisdir==1){
				strcpy(searchresult, keywords[minlenidx]);
				strcat(temp, searchresult);
				strcpy(*keyword, temp);
			}
			else stpcpy(*keyword, keywords[minlenidx]);
			return 0;
		}
    } else {
        perror ("EXIT_FAILURE");
		return 0;
    }
}
int runtab(char* cline, char** answer, int* tabcnt){
	//-1: 명령어가 없을때
	// 0: ls명령어 없을때
	// 1: ls명령어 있으나 키워드 없을 때
	// 2: 공동된 부분 혹은 키워드 결과값 있을 때
	// 3: 탭 두번 목록 출력
	strcpy(*answer, cline);
	if(strlen(cline) == 1){
		return -1;
	}
	char *cmd = strtok(cline, " ");  
	char *keyword = strtok(NULL, " "); 
	
	if(strcmp(cmd, "ls")==0){
		if(*tabcnt == 0){
			if((keyword==NULL) || *keyword=='\n' || *keyword==' '){
				(*tabcnt)++;
				strcpy(*answer, cmd);
				return 1;
			}
			else if(strcmp(keyword, "./")==0){
				(*tabcnt)++;
				char* temp = (char *)malloc(sizeof(char)*(strlen(cline))+1);
				strcpy(temp, cmd);
				strcat(temp, " ");
				strcat(temp, keyword);
				strcpy(*answer, temp);
				return 1;
			}
			else{
				(*tabcnt)++;
				commonkeyword(&keyword);
				char* temp = (char *)malloc(sizeof(char)*(strlen(cline))+1);
				strcpy(temp, cmd);
				strcat(temp, " ");
				strcat(temp, keyword);
				strcpy(*answer, temp);
				return 2;
			}
		}
		else if(*tabcnt > 0){
			printf("\n");
			runls(keyword);
			*tabcnt=10;
			return 3;
		}
		
	}
	else
		return 0;	
}