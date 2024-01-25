#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

void *thread_function1(void *arg) {
    pid_t pid = getpid();
    pid_t tid = syscall(SYS_gettid);

    printf("thread1 create success!\n");
    printf("thread1 tid = %d, pid = %d\n", tid, pid);

    // 使用 exec 函数调用 system_call 可执行文件
    char *args[] = {"./system_call", NULL};
    execv(args[0], args);

    perror("execv"); // 如果 execv 失败，输出错误信息
    exit(1);
	printf("thread1 systemcall return\n");
    pthread_exit(NULL);
}

void *thread_function2(void *arg) {
    pid_t pid = getpid();
    pid_t tid = syscall(SYS_gettid);

    printf("thread2 create success!\n");
    printf("thread2 tid = %d, pid = %d\n", tid, pid);

    // 使用 exec 函数调用 system_call 可执行文件
    char *args[] = {"./system_call", NULL};
    execv(args[0], args);

    perror("execv"); // 如果 execv 失败，输出错误信息
    exit(1);
	printf("thread2 systemcall return\n");
    pthread_exit(NULL);
}

int main() {
    pthread_t thread1, thread2;

    pthread_create(&thread1, NULL, thread_function1, NULL);
   

    pthread_create(&thread2, NULL, thread_function2, NULL); 


    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}

