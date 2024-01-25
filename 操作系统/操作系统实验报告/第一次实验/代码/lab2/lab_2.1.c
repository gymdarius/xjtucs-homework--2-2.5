#include <pthread.h>
#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>
#include <semaphore.h>
// 共享变量
int shared_variable = 0;
// 信号量
sem_t sem2;  // 用于线程操作顺序控制
// 线程函数
void *thread_function1(void *thread_id) {
    int i;
    for (i = 0; i < 100000; i++) {
        shared_variable++;
    }
    sem_post(&sem2); // 释放sem2以允许线程2执行
    pthread_exit(NULL);
}
void *thread_function2(void *thread_id) {
    int i;
    sem_wait(&sem2); // 等待sem2以确保线程1先执行
    for (i = 0; i < 100000; i++) {
        shared_variable--;
    }
    pthread_exit(NULL);
}
int main() {
    pthread_t thread1, thread2;
    // 初始化信号量
    sem_init(&sem2, 0, 0);
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
    // 销毁信号量
    sem_destroy(&sem2);
    printf("Final shared_variable value: %d\n", shared_variable);
    return 0;
}
