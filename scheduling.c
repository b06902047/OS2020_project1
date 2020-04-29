#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <sys/types.h>
#define _PROCESS_H_
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

struct process{
	char name[32];
	int ready_time;
	int last_exec_time;
	int exec_time;
	pid_t pid;
};

void time_unit(){
	volatile unsigned long i; 
	for(int i=0;i<1000000UL;i++)
		;
}

void specify_cpu(int pid, int core){
	cpu_set_t my_set;        //Define your cpu_set bit mask. 
	CPU_ZERO(&my_set);       // Initialize it all to 0
	CPU_SET(core, &my_set);     // set the bit that represents core. 
	sched_setaffinity(pid, sizeof(my_set), &my_set);
	return;
}

static inline int block_proc(int pid){//remember to check return value//趙允祥說這樣比較快
	struct sched_param para;
	para.sched_priority = 0;
	int ret = sched_setscheduler(pid, SCHED_IDLE, &para);
	return ret;
}
static inline int wake_proc(int pid) {//remember to check return value
    struct sched_param para;
    para.sched_priority = 0;
    int ret = sched_setscheduler(pid, SCHED_OTHER, &para);
    return ret;
}
		/*struct timespec {
			time_t   tv_sec;        // seconds 
    		long     tv_nsec;       // nanoseconds 
		};*/
void child_proc(int total_time){
		block_proc(getpid());
		struct timespec start,end;
		char my_message[256];
  		clock_gettime(CLOCK_REALTIME, &start);
		for(int j=0;j<total_time;j++){
			//if(j%100==0) printf("%d %d\n",getpid(),j);
			time_unit();
		}
		clock_gettime(CLOCK_REALTIME, &end);
		sprintf(my_message, "[project1] %d %lu.%09lu %lu.%09lu\n", getpid(), start.tv_sec, start.tv_nsec, end.tv_sec, end.tv_nsec);
		syscall(334, my_message);
		exit(0);
}

int FIFO(struct process* processes, int num_of_proc){
	specify_cpu(getpid(), 0);
	wake_proc(getpid());
	int time_units_now=0;
	int remain_proc=num_of_proc;
	int running=-1;
	int next=0;
	while(1){
		if(running!=-1&&processes[running].exec_time==0){		//someone is ending its work
			waitpid(processes[running].pid, NULL, 0);			//wait for child process to return 
			printf("%s %d\n", processes[running].name, processes[running].pid);
			processes[running].pid=-1;
			running=-1;
			remain_proc--;
			if(remain_proc==0)
				exit(0);
		}
		for(int i=0;i<num_of_proc;i++){
			if(processes[i].ready_time==time_units_now&&processes[i].pid==-1){
				int pid=fork();
				if (pid < 0) {
					perror("fork");
					return -1;
				}
				else if (pid==0){				//Child Process
					child_proc(processes[i].exec_time);
						
				}
				specify_cpu(pid, 1);
				processes[i].pid=pid;
			}
		}
		if(running==-1&&processes[next].pid!=-1){		//no one is working, then assign ready process to cpu
				wake_proc(processes[next].pid);
				specify_cpu(processes[next].pid, 1);
				running=next;
		}
		for(int i=running+1;running!=-1&&i<num_of_proc&&processes[i].pid!=-1;i++)
			block_proc(processes[i].pid);
		if(running!=-1){
			wake_proc(processes[running].pid);
			processes[running].exec_time--;
			next=running+1;
		}
		time_unit();
		time_units_now++;
	}
	return 0;
}

int RR(struct process* processes, int num_of_proc){
	specify_cpu(getpid(), 0);
	wake_proc(getpid());
	int time_units_now=0;
	int remain_proc=num_of_proc;
	int running=-1;
	int round_time=0;
	int next=0;
	while(1){
		for(int i=0;i<num_of_proc;i++){//create new process
			if(processes[i].ready_time==time_units_now){
				int pid;
				pid=fork();
				if (pid < 0) {
					perror("fork");
					return -1;
				}
				else if (pid==0){				//Child Process
					child_proc(processes[i].exec_time);
						
				}			//prevent child process from getting parent's cpu 
				specify_cpu(pid, 1);
				processes[i].pid=pid;
				
			}
		}
		if(running!=-1)
			next=(running+1)%num_of_proc;
		if(running!=-1&&processes[running].exec_time==0){		//someone is ending its work
			
			waitpid(processes[running].pid, NULL, 0);			//wait for child process to return 
			printf("%s %d\n", processes[running].name, processes[running].pid);
			processes[running].pid=-1;
			running=-1;
			remain_proc--;
			if(remain_proc==0)
				exit(0);
		}
		else if(running!=-1 && round_time==0){
			block_proc(processes[running].pid);
			running=-1;
		}
		if(running==-1){		//no one is working, then assign ready process to cpu
			for(int i=0;i<num_of_proc;i++)
				if(processes[(next+i)%num_of_proc].pid!=-1){
					wake_proc(processes[(next+i)%num_of_proc].pid);
					round_time=0;
					//printf("%d  %d\n",(next+i)%num_of_proc,time_units_now);
					specify_cpu(processes[(next+i)%num_of_proc].pid, 1);
					running=(next+i)%num_of_proc;
					break;
				}
		}
		time_unit();
		if(running!=-1)
			processes[running].exec_time--;
		time_units_now++;
		round_time=(round_time+1)%500;
	}
	return 0;
}

int SJF(struct process* processes, int num_of_proc){
	specify_cpu(getpid(), 0);
	wake_proc(getpid());
	int time_units_now=0;
	int remain_proc=num_of_proc;
	int running=-1;
	while(1){
		for(int i=0;i<num_of_proc;i++){
			if(processes[i].ready_time==time_units_now){
				int pid;
				pid=fork();
				if (pid < 0) {
					perror("fork");
					return -1;
				}
				else if (pid==0){				//Child Process
					child_proc(processes[i].exec_time);	
					//specify_cpu(getpid(), 1);	
				}
				specify_cpu(pid, 1);
				processes[i].pid=pid;
			}
		}
		if(running!=-1&&processes[running].exec_time==0){		//someone is ending its work			
			waitpid(processes[running].pid, NULL, 0);			//wait for child process to return 
			printf("%s %d\n", processes[running].name, processes[running].pid);
			processes[running].pid=-1;
			running=-1;
			remain_proc--;

			if(remain_proc==0)
				exit(0);
		}
		if(running==-1){
			int min=10000000;
			for(int i=0;i<num_of_proc;i++){
				if(processes[i].ready_time>time_units_now)
					break;
				if(processes[i].exec_time < min && processes[i].pid!=-1){
					running=i;
					min=processes[i].exec_time;
				}
			}
			if(running!=-1){
				wake_proc(processes[running].pid);
				specify_cpu(processes[running].pid, 1);
			}
		}
		else
			wake_proc(processes[running].pid);
		time_unit();
		if(running!=-1){
			processes[running].exec_time--;
		}
		time_units_now++;
	}
	return 0;

}

int PSJF(struct process* processes, int num_of_proc){
	specify_cpu(getpid(), 0);
	wake_proc(getpid());
	int time_units_now=0;
	int remain_proc=num_of_proc;
	int running=-1;
	while(1){
		for(int i=0;i<num_of_proc;i++){
			if(processes[i].ready_time==time_units_now){
				int pid;
				pid=fork();
				if (pid < 0) {
					perror("fork");
					return -1;
				}
				else if (pid==0){				//Child Process
					child_proc(processes[i].exec_time);	
					//specify_cpu(getpid(), 1);	
				}
				specify_cpu(pid, 1);
				processes[i].pid=pid;
			}
		}
		if(running!=-1&&processes[running].exec_time==0){		//someone is ending its work
			
			waitpid(processes[running].pid, NULL, 0);			//wait for child process to return 
			printf("%s %d\n", processes[running].name, processes[running].pid);
			processes[running].pid=-1;
			running=-1;
			remain_proc--;

			if(remain_proc==0)
				exit(0);
		}

		int min=10000000;
		int old=running;
		for(int i=0;i<num_of_proc;i++){
			if(processes[i].ready_time>time_units_now)
				break;
			if(processes[i].exec_time < min && processes[i].pid!=-1){
				running=i;
				min=processes[i].exec_time;
				//if(time_units_now%500==0)
				//	printf("%s,%d\n",processes[i].name,processes[i].exec_time);
			}
		}
		if(old!=-1 && old!=running && processes[old].pid!=-1) block_proc(processes[old].pid);
		if(running!=-1){
			wake_proc(processes[running].pid);
			specify_cpu(processes[running].pid, 1);
		}
		time_unit();
		if(running!=-1){
			processes[running].exec_time--;
		}
		time_units_now++;
	}
	return 0;
}


