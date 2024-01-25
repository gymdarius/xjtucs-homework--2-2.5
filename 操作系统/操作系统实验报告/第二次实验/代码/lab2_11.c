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