# 1题目完成情况

## 1进程相关编程实验

**步骤一**：编写并多次运行图中代码

运行结果如下：

![image-20231015175511098](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231015175511098.png)

fork()函数通过复制当前进程产生一个子进程，在父进程中，`fork()` 返回子进程的进程 ID，而在子进程中，它返回 0。

getpid()函数返回当前进程的pid_t值。

父进程的pid值为子进程的pid值，pid1值为父进程的pid值，父进程的pid值比子进程的pid值要小1.

子进程的pid值为0，pid1值为自己的pid值。

**问题1**

在实验过程中，在printf输出的最后添加\n导致进程调度输出不同

代码修改为：

![image-20231015175822544](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231015175822544.png)

运行结果为：

![image-20231015175757687](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231015175757687.png)

**已解决**

**为了实验效果，后面的实验过程中均不添加/n**

**步骤二**：删去代码中的wait()函数并多次运行程序，分析运行结果。

运行结果为：

![image-20231015180344557](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231015180344557.png)

该运行结果中parent的输出早于child的输出，正好与不删去wait()函数时相反。

解释：wait(NULL)函数会使父进程阻塞，直到它的一个子进程结束为止，一旦子进程终止，父进程将不再阻塞。在删去wait()之前，父进程运行到wait()时阻塞，等到子结束、输出之后，父进程才可以结束，输出。因此child输出提前于parent。而删去wait()后，则恰好相反，这里需要考虑printf()的特性：**缓冲区**。

**步骤三**：添加一个全局变量并在父进程和子进程中对这个变量做不同操作，输出操作结果并解释。

代码为：

![image-20231015190434041](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231015190434041.png)

运行结果为：

![image-20231015190412646](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231015190412646.png)

操作：定义一个全局变量，在子进程和父进程中进行不同的操作，并且输出其地址。

可以看到子进程和父进程并不共享全局变量，但是由于COW（写时复制）机制，子进程和父进程中的全局变量仍然指向相同的物理内存页。

**步骤四**：在步骤三基础上，在return前增加对全局变量的操作，并输出结果，观察并解释所做操作和输出结果。

代码为：

![image-20231015191637931](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231015191637931.png)

输出结果为：

![image-20231015191620572](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231015191620572.png)

操作：在return前加上了一句printf()函数，用来输出value的值和地址。

子进程先结束先输出，父进程由于阻塞，后结束后输出。进一步说明了父进程和子进程并不共享全局变量。

**步骤五**：修改图中程序，在子进程中调用system()与exec族函数。

**system()**

代码：

```c
#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = getpid();
    printf("system_call PID: %d\n", pid);
    return 0;
}

```



```c
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
        // 使用 system 函数调用 system_call 可执行文件
        fflush(stdout);
        system("./system_call");
        printf("child process PID: %d\n", pid1);
    } else {
        pid1 = getpid();
        printf("parent process PID: %d\n", pid1);
        wait(NULL);
    }
    return 0;
}

```

运行结果：

首先输出父进程的PID，然后父进程中由于wait(NULL)阻塞、等待子进程结束，子进程中输出子进程PID，然后用system()函数调用system_call函数，输出当前进程system_call进程PID。由于`system()` 函数不会替换当前进程，而是在当前进程的上下文中启动一个新的shell，并在该shell中执行指定的命令。执行完命令后，shell进程结束，控制返回到原始进程。**所以system()执行完后还会执行输出子进程PID的代码**，而`exec` 函数族会完全替换当前进程的代码和数据，而不会启动一个新的shell。它会从新程序的 `main` 函数开始执行，当前进程的代码和数据将不再存在。

**exec族函数** 代码为：

```c
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

```

# ？进程号来判断、进程数来判断 system()/exec()族

运行结果为：

![image-20231016090433125](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231016090433125.png)

## 2线程相关实验

**步骤一**：设计程序，创建两个子线程，两线程分别对同一个共享变量进行多次操作，观察输出结果。代码中定义共享变量初始值为0，两线程分别对其进行100000次 +/-操作，最终在主进程中输出处理后的变量值。

代码为：

```c
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

```

运行结果为：

![image-20231016081817577](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231016081817577.png)

没有使用互斥锁或其它同步机制。这将导致两个线程同时访问和修改`shared_variable`，由于竞争条件，输出结果可能是不确定的，并且在每次运行时都可能不同。

**问题2**：gcc编译时没有链接成功，报错`undefined reference to `pthread_create'`，因此编译时使用 -pthread 标志来链接 pthread 库



**步骤二**：修改程序， 定义信号量 signal，使用 PV 操作实现共享变量的访问与互斥。运行程序，观察最终共享变量的值。  

定义了一个信号量，初值赋为0，在进程1的循环增值操作后进行V操作，在进程2的循环减值操作前进行P操作，确保同一时间内只有一个线程在工作。所以最终结果输出为0。

代码为：

```c
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
```

运行结果为：

![image-20231016083333668](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231016083333668.png)



**步骤三：**在第一部分实验了解了 system()与 exec 族函数的基础上，将这两个函数的调用改为在线程中实现，输出进程 PID 和线程的 TID 进行分析。  

在主函数中新建两个线程，两线程分别输出tid和pid并且调用system()或exec族函数来调用./system_call

调用system()

代码为：

```c
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

```

运行结果为：

![image-20231016094921365](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231016094921365.png)

调用exec族函数

代码为：

```c
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
```

**进程和线程中调用exec族函数的区别**

输出结果为：

![image-20231016170927936](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231016170927936.png)



调整代码结构后，输出结果为：

![image-20231016171117275](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231016171117275.png)

## 3自旋锁相关实验

共享变量的输出结果为10000，说明我们已经使用自旋锁来实现线程间的同步 。

代码为：

```c
#include <stdio.h>
#include <pthread.h>// 定义自旋锁结构体
typedef struct {
		int flag;
	} spinlock_t;
// 初始化自旋锁
void spinlock_init(spinlock_t *lock) {
	lock->flag = 0;
}
// 获取自旋锁
void spinlock_lock(spinlock_t *lock) {
	while (__sync_lock_test_and_set(&lock->flag, 1)) {
		// 自旋等待
	}
}
// 释放自旋锁
void spinlock_unlock(spinlock_t *lock) {
	__sync_lock_release(&lock->flag);
}
// 共享变量
int shared_value = 0;
// 线程函数
void *thread_function(void *arg) {
	spinlock_t *lock = (spinlock_t *)arg;
	for (int i = 0; i < 5000; ++i) {
		spinlock_lock(lock);
		shared_value++;
		spinlock_unlock(lock);
	}
	return NULL;
}
int main() {
	pthread_t thread1, thread2;
	spinlock_t lock;
	// 输出共享变量的值
	printf("shared_value = %d\n",shared_value);
	// 初始化自旋锁
	spinlock_init(&lock);
	// 创建两个线程
	 if (pthread_create(&thread1, NULL, thread_function, &lock) == 0) {
        printf("Thread 1 created successfully.\n");
    } else {
        printf("Thread 1 creation failed.\n");
    }
    
    if (pthread_create(&thread2, NULL, thread_function, &lock) == 0) {
        printf("Thread 2 created successfully.\n");
    } else {
        printf("Thread 2 creation failed.\n");
    }
	// 等待线程结束
	pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
	// 输出共享变量的值
	printf("shared_value = %d\n", shared_value);
	return 0;
}
```

输出结果为：

![image-20231016162744572](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231016162744572.png)

# 2题目完成过程中遇到的问题及解决方法

## 问题1

### 1.问题描述

父子进程调度输出时，\n添加与否导致进程调度输出不同

### 2.解决过程

printf 函数是一个行缓冲函数，先将内容写到缓冲区，满足一定条件后，才会将内容写入对应的文件或流中。满足条件如下：

1. 缓冲区填满。
2. 写入的字符中有‘\n’ '\r'。
3. 调用 fflush 或 stdout 手动刷新缓冲区。
4. 调用 scanf 等要从缓冲区中读取数据时，也会将缓冲区内的数据刷新。
5. 程序结束时。

不加\n 时，即使 parent 进程先执行输出函数，但由于该进程未结束，所以内容仍然保留在缓冲区中。而子进程由于先于父进程结束，所以子进程的输出先行输出到屏幕上，造成看上去是先调用子进程，后调用父进程的效果。

lab1.0的代码输出结果（没有注释wait(NULL),没有添加/n）

![image-20231014215811034](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231014215811034.png)

而加入\n 之后， \n 强制刷新，每 printf 输出一次就显示在屏幕上一次，这个顺序是真实的进程调度顺序

添加/n

![image-20231014220107514](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231014220107514.png)



## **问题2**：

gcc编译时没有链接成功，报错`undefined reference to `pthread_create'`，因此编译时使用 -pthread 标志来链接 pthread 库

# 3题目完成过程中最得意的和收获最大内容

## 1最得意的部分

跟着实验指导书一步步做下来，第一次做会感觉有些地方的设计很奇怪，没有感受到其中的深意，后面通过上网查找资料发现了指导书如此设计的目的，并成功实现了对应的效果。

## 2收获

1. 父子进程运行的先后顺序由 wait()函数控制，不加 wait 时由 CPU 调度运行。

2. 父子进程间全局变量不共享，但子进程在创建时复制父进程的值

3. system()函数可以调用系统调用

4. exec族函数可以实现进程替换，用另一个进程替换掉当前运行的进程

5. 多线程之间需要互斥访问临界资源防止竞争条件 



切换上下文时间不同，信号量/互斥锁

