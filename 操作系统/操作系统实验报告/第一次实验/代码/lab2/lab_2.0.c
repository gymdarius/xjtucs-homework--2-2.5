#include <pthread.h>
#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>

// 共享变量
int shared_variable = 0;

// 线程函数
void *thread_function1(void *thread_id) {
    int i;
    for (i = 0; i < 100000; i++) {
        shared_variable++;
    }
    pthread_exit(NULL);
}
void *thread_function2(void *thread_id) {
    int i;
    for (i = 0; i < 100000; i++) {
        shared_variable--;
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t thread1, thread2;

    // 创建两个子线程
    if (pthread_create(&thread1, NULL, thread_function1, (void *)1) == 0) {
        printf("Thread 1 created successfully.\n");
    } else {
        printf("Thread 1 creation failed.\n");
    }

    if (pthread_create(&thread2, NULL, thread_function2, (void *)2) == 0) {
        printf("Thread 2 created successfully.\n");
    } else {
        printf("Thread 2 creation failed.\n");
    }

    // 等待线程结束
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Final shared_variable value: %d\n", shared_variable);

    return 0;
}

