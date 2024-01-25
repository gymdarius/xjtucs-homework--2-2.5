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
    	//lockf(fd[WRITEEND],F_LOCK,0);   // 锁定管道 
    	int i;
        for(i=1;i<=2000;i++)		        //  分200次每次向管道写入字符’1’ 
            write(fd[WRITEEND],&c1,sizeof(char));
        //sleep(5);                       // 等待读进程读出数据 
        //lockf(fd[WRITEEND],F_ULOCK,0);  // 解除管道的锁定 
        printf("pid1 over\n");
        exit(0);                 		// 结束进程1 
    }

    else                                    
    { 
        while((pid2 = fork()) == -1);       // 若进程2创建不成功,则空循环 
        if(pid2 == 0)                       // 子进程2
        { 
            //lockf(fd[WRITEEND],F_LOCK,0);   // 锁定管道 
            int i;
            for(i=1;i<=2000;i++)		        //  分200次每次向管道写入字符’2’ 
                write(fd[WRITEEND],&c2,sizeof(char));
            //sleep(5);                       // 等待读进程读出数据 
            //lockf(fd[WRITEEND],F_ULOCK,0);  // 解除管道的锁定 
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

