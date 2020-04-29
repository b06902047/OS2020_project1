#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include "scheduling.c"


int cmp(const void *p1,const void *p2)
{
const  struct process *pf1 = (const  struct process*) p1;
const  struct process *pf2 = (const  struct process*) p2;
return (int)(pf1->ready_time - pf2->ready_time);
}

int main(int argc, char *argv[]) {
	char method[256];
	scanf("%s",method);
	
	int num_of_proc;
	scanf("%d",&num_of_proc);

	struct process *processes=(struct process *)malloc(num_of_proc*sizeof(struct process));

	for(int i=0;i<num_of_proc;i++){
		scanf("%s %d %d",processes[i].name, &processes[i].ready_time, &processes[i].exec_time);
		processes[i].pid=-1;
	}
	qsort(processes, num_of_proc, sizeof(struct process), cmp);
	if(strcmp(method,"FIFO")==0){
		//printf("Parent: %d\n",getpid());
		FIFO(processes, num_of_proc);
	}

	else if(strcmp(method,"RR")==0){
		RR(processes, num_of_proc);
	}

	else if(strcmp(method,"SJF")==0){
		SJF(processes, num_of_proc);
	}

	else if(strcmp(method,"PSJF")==0){
		PSJF(processes, num_of_proc);
	}

	else
		printf("Invalid Policy!!");

	return 0;
}








