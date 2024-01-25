#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>
int value=100;
int main(){
	pid_t pid,pid1;
	
	pid=fork();
	if(pid<0){
		fprintf(stderr,"Fork Failed");
		return 1;
	}
	else if(pid==0){
		printf("child:value=%d",value);
		value+=1;
		printf("child:value=%d",value);
		printf("child:*value=%p",&value);
	}
	else{
		wait(NULL);
		printf("parent:value=%d",value);
		value*=value;
		printf("parent:value=%d",value);
		printf("parent:*value=%p",&value);
	}
	printf("before return value=%d *value=%p",value,&value);
	return 0;
} 
