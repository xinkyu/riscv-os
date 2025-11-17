// user/user.h
#ifndef __USER_H__
#define __USER_H__

// 系统调用
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int getpid(void);
int sleep(int);
int uptime(void);
int write(int, const void*, int);

// 库函数
void printf(const char*, ...);
char* strcpy(char*, const char*);
int strlen(const char*);

#endif