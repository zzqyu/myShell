//smallsh.h
#include<unistd.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include<signal.h> // signal 처리를 위한 헤더
#include<setjmp.h> //setjmp, longjmp
#include<stdbool.h> //bool
#include <dirent.h>
#include <regex.h>
#include <pthread.h>
#include "getch.h"
#define MAXARG 512 //명령인수의 최대수
#define MAXBUF 512//입력줄의 최대의 길이
#define FOREGROUND 0
#define FG 1
#define BG 0
