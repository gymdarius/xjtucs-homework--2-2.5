#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
int main() {
    pid_t pid, pid1;
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork Failed\n");
        return 1;
    } else if (pid == 0) {
        pid1 = getpid();
        printf("child process1 PID: %d\n", pid1);
        // 使用 exec 函数调用 system_call 可执行文件
        char *args[] = {"./system_call", NULL};
        execv(args[0], args);
        // 如果 execv 失败，输出错误信息
        perror("execv");
        exit(1);
        printf("child process PID: %d\n", pid1);
    } else {
        pid1 = getpid();
        printf("parent process PID: %d\n", pid1);
        wait(NULL);
    }
    return 0;
}

