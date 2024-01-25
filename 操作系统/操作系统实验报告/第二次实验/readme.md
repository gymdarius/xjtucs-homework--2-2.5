## lab2-1 软中断通信

使用 man 命令查看 fork 、 kill 、 signal、 sleep、 exit 系统调用的帮助手册 ，结果如下所示：

fork

![image-20231113150510572](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113150510572.png)

kill

![image-20231113150433821](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113150433821.png)

signal

![image-20231113150531619](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113150531619.png)

sleep

![image-20231113150611513](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113150611513.png)

exit

![image-20231113150631596](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113150631596.png)

设计思路：

signal(sig,function): 捕捉中断信号 sig 后执行 function 规定的操作。  我定义了void inter_handler(int sign)函数来输出显示捕捉到了哪种信号。

通过pause函数挂起子进程等待父进程发出信号

![image-20231104112521354](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231104112521354.png)

![img](https://gitee.com/du-jianyu1012/img/raw/master/picture/1029215-20171208124229640-1713087570.png)



### 代码：

```c
#include <stdio.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>



void inter_handler(int sign)
{
	switch(sign)
	{
		case SIGINT:
			printf("\n2 stop test\n");
			break;
		case SIGSTKFLT:
			printf("\n16 stop test\n");
			break;
		case SIGCHLD:
			printf("\n17 stop test\n");
			break;
		default:;
	}
}

int main()
{
	pid_t pid1, pid2;
	while((pid1 = fork()) == -1);
	if(pid1 > 0)
	{
		while((pid2 = fork()) == -1);
		if(pid2 > 0)
		{
			signal(2, inter_handler);
			sleep(5);
			kill(pid1, SIGSTKFLT); //kill the first child
			kill(pid2, SIGCHLD); //kill the second child
			wait(NULL);   //wait the first child's finish
			wait(NULL);   //wait the second child's finish
			printf("\nParent process is killed! \n");
			exit(0);
		}
		else
		{
			signal(2, SIG_IGN); //用于忽略SIGINT信号
			signal(17, inter_handler); //the second child is waiting
			pause();    //暂停等待
			printf("\nChild process 2 is killed by parent! \n");
			exit(0);
		}
	}
	else
	{
		signal(2, SIG_IGN); //用于忽略SIGINT信号
		signal(16, inter_handler); //the first child is waiting
		pause();    //暂停等待
		printf("\nChild process 1 is killed by parent! \n");
		exit(0);
	}
	return 0;
}
```

### 1最初认为的运行结果

我最初认为运行结果是 如果5s内从键盘手动输出`Ctrl+C`发出中断信号，那么应当首先输出截获SIGINT信号对应的输出。接下来两个子进程的输出顺序是随机的，但是要求每个子进程需要满足，先输出捕获信号后执行函数对应的输出，然后打印输出子进程被父进程杀死的语句，最后输出父进程被回收的对应语句

**拓扑结构**即为 2 stop test在最前，17 stop test 在 Child process 2 is killed 前，16 stop test 在 Child process 1 is killed 前，parent process is killed 在最后，其余顺序可以随意出现。

### 2实际的运行结果

![image-20231114231525141](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231114231525141.png)

**特点**：

5秒内中断即为上图结果，5秒后中断结果为下图：

![image-20231114231607340](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231114231607340.png)

可以看出 5s内手动发出信号会使得父进程中多捕获一个软中断信号进而比5s后中断的情形多执行一次inter_handle()函数进而得到第一条额外的输出，而5s后父进程不再休眠等待因而不执行这条额外的输出

### 3改为闹钟中断后

代码如下：

```c
#include <stdio.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

void inter_handler(int sign)
{
	switch(sign)
	{
		case SIGINT:
			printf("\n2 stop test\n");
			break;
		case SIGSTKFLT:
			printf("\n16 stop test\n");
			break;
		case SIGCHLD:
			printf("\n17 stop test\n");
			break;
		case SIGALRM:
			printf("\n14 stop test\n");
			break;
		default:;
	}
}

int main()
{
	pid_t pid1, pid2;
	while((pid1 = fork()) == -1);
	if(pid1 > 0)
	{
		while((pid2 = fork()) == -1);
		if(pid2 > 0)
		{
			signal(14, inter_handler);
             printf("after 4 seconds kill the first\n");
			alarm(4);
			sleep(8);       //sleep会被alarm函数发出的信号中断
			kill(pid1, SIGSTKFLT); //kill the first child
			printf("after 6 seconds kill the second\n");
			alarm(6);
			sleep(8);
			kill(pid2, SIGCHLD); //kill the second child
			wait(NULL);   //wait the first child's finish
			wait(NULL);   //wait the second child's finish
			printf("\nParent process is killed! \n");
			exit(0);
		}
		else
		{
			signal(17, inter_handler); //the second child is waiting
			pause();
			printf("\nChild process 2 is killed by parent! \n");
			exit(0);
		}
	}
	else
	{
		signal(16, inter_handler); //the first child is waiting
		pause();
		printf("\nChild process 1 is killed by parent! \n");
		exit(0);
	}
	return 0;
}
```

运行结果如下：

![image-20231114232948133](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231114232948133.png)

运行结果如上图。与之前的不同是，这次的实现设置了alarm()函数和sleep()函数，alarm函数运行设置的秒数后会发出 `SIGALRM` 信号给父进程，而这会中断sleep()语句，进而实现对kill()函数的调用

### 4kill命令的分析

kill命令在程序中使用了两次，作用是向子进程发送中断信号结束此进程，第一次执行后，子进程接受16信号并打印16 stop test，并打印子进程1被杀死的信息，第二次执行同理，打印17 stop test，并打印子进程2被杀死的信息。

### 5kill命令拓展

进程可以通过exit函数来自主退出，进程自主退出的方式会更好一些，进程在退出时，`exit()` 函数会执行一系列清理操作，包括关闭文件、释放内存等。这有助于防止资源泄漏； exit()允许进程向父进程传递一个退出状态。这个退出状态可以被父进程获取，用于了解子进程的退出情况。`kill` 命令不能提供这种方式。使用kill命令在进程外部强制其退出可能导致进程的数据丢失或资源泄漏，但当进程出现异常无法退出时，可能需要kill命令来中止进程。

## lab2-2进程的管道通信

### (1)你最初认为运行结果会怎么样？

加锁时的运行结果应该是先输出1000个1再输出1000个2，与实际结果一样。不加锁时应该12交替输出。

### (2)实际的结果什么样？有什么特点？试对产生该现象的原因进行分析。

**加锁运行状态**

![image-20231113162222799](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113162222799.png)

分析：有锁状态下，同一时间内只有一个进程可以写入，因而是先写入了2000个1而后写入了2000个2

**不加锁运行状态**

![image-20231113162315962](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113162315962.png)

分析：无锁状态下，两进程对InPipe的写入在时序上是随机的，因而打印结果也是1与2交错打印。

### (3)实验中管道通信是怎样实现同步与互斥的？如果不控制同步与互斥会发生什么后果？

同步与互斥的实现：在对管道写入前，先对管道用lockf函数加锁，等管道写入完毕后，在将管道解锁。

不控制的后果：由于多个进程同时向管道中写入数据，那么数据就很容易发生交错和覆盖，导致数据错误。

### 代码：

```c++
#include <unistd.h>                                                                                                       
#include <signal.h>                                                                                                       
#include <stdio.h> 
#include <stdlib.h>   
#include <fcntl.h>                                                                                                    
int pid1,pid2;     // 定义两个进程变量 

//define read/write end code
#define READEND 0
#define WRITEEND 1

int main(int argc, char* argv[])  
{
    int fd[2]; 
    char InPipe[1000];      	        // 定义读缓冲区(buffer)
    char c1='1', c2='2';
    pipe(fd);                           // 创建管道 
    while((pid1 = fork( )) == -1);      // 如果进程1创建不成功,则空循环 

    if(pid1 == 0)                       // 子进程1
    {                  
    	lockf(fd[WRITEEND],F_LOCK,0);   // 锁定管道 
    	int i;
        for(i=1;i<=2000;i++)		        //  分200次每次向管道写入字符’1’ 
            write(fd[WRITEEND],&c1,sizeof(char));
        //sleep(5);                       // 等待读进程读出数据 
        lockf(fd[WRITEEND],F_ULOCK,0);  // 解除管道的锁定 
        printf("pid1 over\n");
        exit(0);                 		// 结束进程1 
    }

    else                                    
    { 
        while((pid2 = fork()) == -1);       // 若进程2创建不成功,则空循环 
        if(pid2 == 0)                       // 子进程2
        { 
            lockf(fd[WRITEEND],F_LOCK,0);   // 锁定管道 
            int i;
            for(i=1;i<=2000;i++)		        //  分200次每次向管道写入字符’2’ 
                write(fd[WRITEEND],&c2,sizeof(char));
            //sleep(5);                       // 等待读进程读出数据 
            lockf(fd[WRITEEND],F_ULOCK,0);  // 解除管道的锁定 
            printf("pid2 over\n");
            exit(0);                 
        } 
        else                                //父进程
        { 
            waitpid(pid1,NULL,0);           // 等待子进程1 结束 
            waitpid(pid2,NULL,0);           // 等待子进程2 结束 
            printf("read\n");
            read(fd[READEND],InPipe,4000*sizeof(char));

            printf("read over\n");
            InPipe[4000]='\0';               //  加字符串结束符 
            printf("%s\n",InPipe);          // 显示读出的数据
            exit(0);                        // 父进程结束 
        }    
    } 
    return 0;
} 


```

## lab2-3内存的分配和回收

### （ 1） 对涉及的 3 个算法进行比较，包括算法思想、算法的优缺点、在实现上如何提高算法的查找性能。

**First Fit (FF):**

+ **思想：** 分配时从内存的起始位置开始找到第一个足够大的空闲块进行分配。
+ **优点：** 简单，实现容易。
+ **缺点：** 可能会导致大块的内存碎片。

**Best Fit (BF):**

+ **思想：** 分配时找到所有足够大的空闲块中最小的一个进行分配。
+ **优点：** 尽量减小内存碎片。
+ **缺点：** 查找最小空闲块可能比较耗时。

**Worst Fit (WF):**

+ **思想：** 分配时找到所有足够大的空闲块中最大的一个进行分配。
+ **优点：** 可以避免产生大量小碎片。
+ **缺点：** 可能导致大块的内存碎片。

在实现上如何提高算法的查找性能：

+ 使用更高效的数据结构，如平衡二叉树或哈希表，来存储空闲块信息，以提高查找效率。
+ 改进排序算法
+ 采取内存紧缩技术以提高内存利用率，减少空闲内存块的个数，从而缩短算法查找时间

### （ 2） 3 种算法的空闲块排序分别是如何实现的。

在代码中，空闲块的排序是通过三个函数实现的：

+ **rearrange_FF()：** 对空闲块按照起始地址进行冒泡排序。
+ **rearrange_BF()：** 对空闲块按照大小进行冒泡排序。
+ **rearrange_WF()：** 对空闲块按照大小进行逆向冒泡排序。

这些函数在每次内存分配或释放后都被调用，以保持空闲块的有序性。

### （ 3）结合实验，举例说明什么是内碎片、外碎片，紧缩功能解决的是什么碎片。

+ **内碎片：** 指已分配给进程的内存块中，没有被进程使用的部分。例如，如果一个进程请求分配100个字节，但系统只能提供102个字节的内存块，那么有2个字节就是内碎片。
+ **外碎片：** 指分配给各进程的内存块之间存在的不可用的、无法分配的小块内存。例如，多次的进程分配和释放导致内存中存在很多不连续的小块，这些小块之间的未分配空间就是外碎片。
+ **紧缩功能解决的碎片：** 当有大量外碎片时，紧缩功能可以将已分配的内存块整理并移动，以便合并这些碎片，形成更大的连续可用空间。这样可以提高内存的利用率，减少外碎片对系统的影响。

### （ 4）在回收内存时，空闲块合并是如何实现的？

在回收内存时，空闲块合并是通过在 `free_mem()` 函数中实现的。具体步骤如下：

1. 将被释放的内存块信息创建为一个新的空闲块节点 `head`。
2. 将这个新的空闲块节点的 `next` 指向当前的空闲块链表的头部（即 `free_block`)。
3. 更新当前的空闲块链表的头部为这个新的空闲块节点 `head`。
4. 对当前空闲块链表进行rearrange_FF()即按照起始地址进行冒泡排序。
5. 对当前的空闲块链表进行遍历，检查是否有相邻的空闲块可以合并。
6. 如果找到相邻的空闲块，则合并它们，更新相应的信息。
7. 继续遍历直到整个空闲块链表。

这样，通过合并相邻的空闲块，可以尽量减小内存碎片，提高内存利用率。

### 运行截图

初始化

![image-20231113201441954](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113201441954.png)

内部碎片

![image-20231113201846340](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113201846340.png)

![image-20231113201903860](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113201903860.png)

外部碎片与内存紧缩

![image-20231113202000863](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113202000863.png)

![image-20231113202018132](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113202018132.png)

![image-20231113202027584](https://gitee.com/du-jianyu1012/img/raw/master/picture/image-20231113202027584.png)

### 代码

```c
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#define PROCESS_NAME_LEN 32
#define MIN_SLICE 10 
#define DEFAULT_MEM_SIZE 1024 
#define DEFAULT_MEM_START 0 
#define MA_FF 1
#define MA_BF 2
#define MA_WF 3
int mem_size=DEFAULT_MEM_SIZE; 
int ma_algorithm = MA_FF; 
static int pid = 0; 
int flag = 0;
struct free_block_type{
    int size;
    int start_addr;
    struct free_block_type *next;
}; 
struct free_block_type *free_block;
struct allocated_block{
    int pid; int size;
    int start_addr;
    char process_name[PROCESS_NAME_LEN];
    struct allocated_block *next;
 };
struct allocated_block *allocated_block_head = NULL;
struct free_block_type *init_free_block(int mem_size){
    struct free_block_type *fb;
    fb=(struct free_block_type *)malloc(sizeof(struct free_block_type));
    if(fb==NULL){
        printf("No mem\n");
        return NULL;
    }
    fb->size = mem_size;
    fb->start_addr = DEFAULT_MEM_START;
    fb->next = NULL;
    return fb;
}
void do_exit(){
	while(free_block->next!= NULL&&free_block !=NULL){
		struct free_block_type *temp = free_block;
		free_block = free_block->next;
		free(temp);
	}
	if(free_block)
		free(free_block);
}
void display_menu(){
    printf("\n");
    printf("1 - Set memory size (default=%d)\n", DEFAULT_MEM_SIZE);
    printf("2 - Select memory allocation algorithm\n");
    printf("3 - New process \n");
    printf("4 - Terminate a process \n");
    printf("5 - Display memory usage \n");
    printf("0 - Exit\n");
}
int set_mem_size(){
    int size;
    if(flag!=0){ 
    printf("Cannot set memory size again\n");
    return 0;
    }
    printf("Total memory size =");
    scanf("%d", &size);
    if(size>0) {
        mem_size = size;
        free_block->size = mem_size;
    }
    flag=1; return 1;
}
int rearrange_FF(){     //对起始地址进行冒泡排序
    struct free_block_type *head = free_block;
    struct free_block_type *pre;
     while(head->next != NULL){
        pre = head->next;
        while(pre != NULL){
            if(head->start_addr > pre->start_addr){
            
                int temp_size = head->size;
                int temp_start = head->start_addr;
                head->size = pre->size;
                head->start_addr = pre->start_addr;
                pre->size = temp_size;
                pre->start_addr = temp_start;
            }
            pre = pre->next;
        }
        head = head->next;
     }
     return 1;
     
}
int rearrange_BF(){ //对大小进行冒泡排序
     struct free_block_type *head = free_block;
     
     struct free_block_type *pre;
     while(head->next != NULL){
         pre = head->next;
         while(pre != NULL){
             if(head->size > pre->size){
               
                 int temp_size = head->size;
                 int temp_start = head->start_addr;
                 head->size = pre->size;
                 head->start_addr = pre->start_addr;
                 pre->size = temp_size;
                 pre->start_addr = temp_start;
             }
             pre = pre->next;
         }
         head = head->next;
     }
	return 1;
}
int rearrange_WF(){ //对大小进行逆向冒泡排序
     struct free_block_type *head = free_block;
     struct free_block_type *pre;
     while(head->next != NULL){
         pre = head->next;
         while(pre != NULL){
             if(head->size < pre->size){
                 int temp_size = head->size;
                 int temp_start = head->start_addr;
                 head->size = pre->size;
                 head->start_addr = pre->start_addr;
                 pre->size = temp_size;
                 pre->start_addr = temp_start;
             }
             pre = pre->next;
         }
         head = head->next;
     }
     return 1;
}
int rearrange(int algorithm){
    switch(algorithm){
    case MA_FF: return rearrange_FF(); break;
    case MA_BF: return rearrange_BF(); break;
    case MA_WF: return rearrange_WF(); break;
 }
}
void set_algorithm(){
    int algorithm;
    printf("\t1 - First Fit\n");
    printf("\t2 - Best Fit \n");
    printf("\t3 - Worst Fit \n");
    scanf("%d", &algorithm);
    if(algorithm>=1 && algorithm <=3) 
        ma_algorithm=algorithm;
    rearrange(ma_algorithm); 
}
int allocate_mem(struct allocated_block *ab){//按照链表free_block进行遍历，如果第一轮不行的话。就重新分配后再次尝试分配
 	struct free_block_type *head, *pre;
 	int request_size=ab->size;
 	head = pre = free_block;
	while(head != NULL){
		if(head->size >= request_size){ 
			if(head->size - request_size <= MIN_SLICE){ 
		  		ab->start_addr = head->start_addr;
		 		ab->size = head->size;
				if(head == free_block){ 
					free_block = head->next; 
				}
				else{
					 pre->next = head->next; 
				}
			    free(head); 
		}
			else{ 
			    ab->start_addr = head->start_addr;
			    ab->size = request_size;
			    head->start_addr += request_size; 
			    head->size -= request_size; 
		    }
		rearrange(ma_algorithm);
	    return 1; 
	   }
    pre = head; 
	head = head->next; 
 	}
	if(rearrange(ma_algorithm) == 1){ 
		pre = head = free_block; 
		while(head != NULL){
			if(head->size >= request_size){ 
			    if(head->size - request_size <= MIN_SLICE){ 
		  		    ab->start_addr = head->start_addr;
		 		    ab->size = head->size;
				    if(head == free_block){ 
					    free_block = head->next; 
					}else{
					    pre->next = head->next; 
					}
			        free(head);
			    }else{ 
				    ab->start_addr = head->start_addr;
			        ab->size = request_size;
			        head->start_addr += request_size; 
				    head->size -= request_size; 
			    }
			    rearrange(ma_algorithm);
			    return 1;
		    }
            else if(head->size < MIN_SLICE) return -1; 
		    pre = head; 
		    head = head->next; 
	  	}	
 	}
 return -1; 
}
int new_process(){
    struct allocated_block *ab;
    int size; int ret;
    ab=(struct allocated_block *)malloc(sizeof(struct allocated_block));
    if(!ab) exit(-5);
    ab->next = NULL;
    pid++;
    sprintf(ab->process_name, "PROCESS-%02d", pid);
    ab->pid = pid; 
    printf("Memory for %s:", ab->process_name);
    scanf("%d", &size);
    if(size>0) ab->size=size;
    ret = allocate_mem(ab);
    if((ret==1) &&(allocated_block_head == NULL)){ 
        allocated_block_head=ab;
        return 1; }
    else if (ret==1) {
        ab->next=allocated_block_head;
        allocated_block_head=ab;
        return 2; }
    else if(ret==-1){ 
    	pid--;
        printf("Allocation fail\n");
        free(ab);
        return -1; 
    }
    return 3;
}
struct allocated_block *find_process(int pid){
    struct allocated_block *ab = allocated_block_head; 
    while(ab != NULL){ 
        if(ab->pid == pid){ 
            return ab; 
        }
        ab = ab->next; 
    }
    return NULL; 
}
int free_mem(struct allocated_block *ab){
    int algorithm = ma_algorithm;
    struct free_block_type *head, *pre;
    head=(struct free_block_type*) malloc(sizeof(struct free_block_type));
    if(!head) return -1;    //分配失败
    head->size=ab->size;
    head->start_addr=ab->start_addr;
    head->next=free_block;
    free_block = head;
	rearrange_FF(); //按照起始地址进行冒泡排序
	pre = free_block;
	struct free_block_type *next = pre->next;
    while(next != NULL){  
        if(pre->start_addr + pre->size != next->start_addr){ 
            pre = pre->next; 
            next = next->next;  
        }
        else{ 
            pre->size += next->size; 
            struct free_block_type *temp = next;
            pre->next = next->next; 
            free(temp); 
            next = pre->next;  
        }
    }
    rearrange(ma_algorithm);
    return 0;
}
int dispose(struct allocated_block *free_ab){
    struct allocated_block *pre, *ab;
    if(free_ab == allocated_block_head) { 
    allocated_block_head = allocated_block_head->next;
    free(free_ab);
    return 1;
    }
    pre = allocated_block_head; 
    ab = allocated_block_head->next;
    while(ab!=free_ab){ pre = ab; ab = ab->next; }
    pre->next = ab->next;
    free(ab);
    return 2;
 }
void kill_process(){
    struct allocated_block *ab;
    int pid;
    printf("Kill Process, pid=");
    scanf("%d", &pid);
	ab = find_process(pid);
	if(ab!=NULL){
        free_mem(ab); 
        dispose(ab);
        printf("\nProcess %d has been killed\n", pid); 
    }
    else
        printf("\nThere is no Process %d\n",pid);
}
int display_mem_usage(){
    struct free_block_type *fbt= free_block;
    struct allocated_block *ab=allocated_block_head;
    printf("----------------------------------------------------------\n");
    printf("Free Memory:\n");
    printf("%20s %20s\n", " start_addr", " size");
    while(fbt!=NULL){
        printf("%20d %20d\n", fbt->start_addr, fbt->size);
        fbt=fbt->next;
 } 
 printf("\nUsed Memory:\n");
 printf("%10s %20s %10s %10s\n", "PID", "ProcessName", "start_addr", " size");
 while(ab!=NULL){
    printf("%10d %20s %10d %10d\n", ab->pid, ab->process_name, 
    ab->start_addr, ab->size);
    ab=ab->next;
 }
 printf("----------------------------------------------------------\n");
 return 0;
 }
int main(){
    char choice; pid=0;
    free_block = init_free_block(mem_size); 
    while(1) {
    display_menu(); 
        fflush(stdin);
        choice=getchar(); 
        switch(choice){
            case '1': set_mem_size(); break; 
            case '2': set_algorithm();flag=1; break;
            case '3': new_process(); flag=1;break;
            case '4': kill_process(); flag=1; break;
            case '5': display_mem_usage(); flag=1; break; 
            case '0': do_exit(); exit(0); 
            default: break; 
            } 
        } 
    return 0;
}
```

