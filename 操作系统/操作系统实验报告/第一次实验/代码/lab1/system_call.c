#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = getpid();
    printf("system_call PID: %d\n", pid);
    return 0;
}

