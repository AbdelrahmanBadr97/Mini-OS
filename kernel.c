#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>
struct msgbuff
{
   long mtype;
   char msg[64];
   int process_id;
   bool hard_busy;
   msgqnum_t msg_qnum;
};

pid_t process_ids[1000];
int stat_loc;
int CLK = 0;

void Killall(int sig);
int num_processes;
pid_t piddisk;
key_t up_disk;
 key_t down_disk ;
 key_t up_process;
int main()
{
    
     up_disk = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
     down_disk = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
     up_process = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    

    printf("Enter number of processes \n");
    scanf("%d", &num_processes);
    char up_str[10];
    char down_str[10];
    char up_process_str[10];
    snprintf(up_str, sizeof(up_str), "%d", up_disk);
    snprintf(down_str, sizeof(down_str), "%d", down_disk);
    snprintf(up_process_str, sizeof(up_process_str), "%d", up_process);
    pid_t pid = fork();
    if(pid == 0){
        piddisk=pid;
        char* args[] = {"./Disk", up_str, down_str, NULL};
        execv("./Disk", args);
    }
    else{
        process_ids[0] = pid;
        sleep(1);
        for(int i = 1 ; i <= num_processes ; i++){
            pid = fork();
            if(pid == 0){
                char filename[10];
                snprintf(filename, sizeof(filename), "%d", i);
                char* args[] = {"./Process", up_process_str, filename, NULL};
                execv("./Process", args);
                break;
            }
            else{
                process_ids[i] = pid;
            }
        }
    }
    struct msgbuff msg_to_disk;
    msg_to_disk.mtype = 1;
    struct msgbuff msg_to_process;
    struct msgbuff incoming_disk;
    struct msgbuff incoming_process;
    signal(SIGINT, Killall);



    while(1){
        kill(process_ids[0], SIGUSR1);
        msgrcv(up_disk, &incoming_disk, sizeof(incoming_disk), 0, !IPC_NOWAIT);
        if(!incoming_disk.hard_busy){
            int process_recieve = msgrcv(up_process, &incoming_process, sizeof(incoming_process), 3, IPC_NOWAIT);
            if(process_recieve != -1){
                strcpy(msg_to_disk.msg , incoming_process.msg);
                int send_status = msgsnd(down_disk, &msg_to_disk, sizeof(msg_to_disk), !IPC_NOWAIT);
                if(send_status == -1){
                    printf("Cannot send instruction to disk");
                }
            }
        }
        for(int i=0;i<1000;i++)
            usleep(1000);
        CLK+=1;
        for(int i = 0 ; i <= num_processes; i++){
            kill(process_ids[i], SIGUSR2);
        }
       
    }
    return 0;
}

void Killall(int sig){
  	//Destroy queues
    msgctl(up_disk, IPC_RMID, NULL); 
    msgctl(down_disk, IPC_RMID, NULL); 
    msgctl(up_process, IPC_RMID, NULL); 
    //kill childeren
    for(int i = 0 ; i <= num_processes ; i++){
        kill(process_ids[i],SIGINT);
        wait(&stat_loc);
        if (i==0)printf("\nDisk is Killed\n");
        else
        printf("Process #%d killed\n",process_ids[i]);
    }
    //kill parent
    kill(0,9);
}