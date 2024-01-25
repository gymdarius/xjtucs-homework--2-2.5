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

    // 使用 system 函数调用 system_call 可执行文件
    system("./system_call");

    printf("thread1 systemcall return\n");

    pthread_exit(NULL);
}

void *thread_function2(void *arg) {
    pid_t pid = getpid();
    pid_t tid = syscall(SYS_gettid);

    printf("thread2 create success!\n");
    printf("thread2 tid = %d, pid = %d\n", tid, pid);

    // 使用 system 函数调用 system_call 可执行文件
    system("./system_call");

    printf("thread2 systemcall return\n");

    pthread_exit(NULL);
}

int main() {
    pthread_t thread1, thread2;

    if (pthread_create(&thread1, NULL, thread_function1, NULL) == 0) {
    } else {
        printf("Thread 1 creation failed.\n");
    }

    if (pthread_create(&thread2, NULL, thread_function2, NULL) == 0) {
    } else {
        printf("Thread 2 creation failed.\n");
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}

