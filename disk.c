#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>


#define MAX_SIZE 64
#define NUM_SLOTS 64


int CLK = 0;
int empty_slots = 10;
int next_valid_slot= 0;
char slots[NUM_SLOTS][MAX_SIZE];
bool isfilled[NUM_SLOTS];
key_t down, up;
int started = 0;

struct msgbuff
{
   long mtype;
   char msg[MAX_SIZE];
   int process_id;
   bool hard_busy;
   msgqnum_t msg_qnum;
};

struct msgbuff disk_instr;
struct msgbuff msg_to_kernel;
bool last_msg_excecuted = 1;
ssize_t recieve_status;


void execute_msg(struct msgbuff msg){
    msg_to_kernel.hard_busy = 1;
    started = CLK;
    printf("start is %d\n",started);
    if(msg.msg[0] == 'A'){
        empty_slots-=1;
        strcpy(slots[next_valid_slot], msg.msg+1);
        isfilled[next_valid_slot] = 1;
        
        if(empty_slots != 0){
            int i = (next_valid_slot + 1) % NUM_SLOTS;
            int num_searches = 0;
            while (num_searches <= NUM_SLOTS){
                if(!isfilled[i]){
                    next_valid_slot = i;
                    break;
                }
                i = (i + 1) % NUM_SLOTS;
                num_searches+=1;
            }
            last_msg_excecuted = 1;
            char to_send[] = "SADD";
            strcpy(msg_to_kernel.msg, to_send);
        }
        while(CLK < started+2){}
        printf("Wrote data into the disk %s clock is %d\n", msg.msg+1,CLK);
        msg_to_kernel.hard_busy = 0;
    }
    else if(msg.msg[0] == 'D'){
        int to_delete = atol(msg.msg+1);
        last_msg_excecuted = 1;
        char to_send[] = "SDEL";
        strcpy(msg_to_kernel.msg, to_send);
        while(CLK < started){}
        msg_to_kernel.hard_busy = 0;
        if( isfilled[to_delete] == 0){
            printf("Nothing to delete clock is %d\n",CLK);
        }
        else{
            printf("Deleted data from the disk %s clock is %d\n ", msg.msg,CLK);
            empty_slots+=1;
        }
        isfilled[to_delete] = 0;
        //printf("Deleted data from the disk %s \n ", msg.msg);
        
    }

}

void wait_for_msg(){
    recieve_status = msgrcv(down, &disk_instr, sizeof(disk_instr), 1 , !IPC_NOWAIT);
    if(recieve_status != -1){
        execute_msg(disk_instr);
    }
}

void sigusr1Handler(int sig){
    char msg[MAX_SIZE];
    snprintf(msg, sizeof(msg), "%d", empty_slots);
    msg_to_kernel.mtype = 2;  
    strcpy(msg_to_kernel.msg, msg);
    int send_status = msgsnd(up, &msg_to_kernel, sizeof(msg_to_kernel), IPC_NOWAIT);
    if (send_status == -1)
        printf("Cannot send the num of empty slots to the kernel\n");
    else{
        last_msg_excecuted = 1;
    }
}

void sigusr2Handler(int sig){
    CLK += 1;
}

int main(int argc , char* argv[]){
    signal(SIGUSR1, sigusr1Handler);
    signal(SIGUSR2, sigusr2Handler);
    up = atol(argv[1]);
    down = atol(argv[2]);
    for(int i = 0 ; i < NUM_SLOTS ; i++)
        isfilled[i] = 0;
    printf("the Disk is here\n");
    while(1){
        wait_for_msg();
    }
    printf("Disk is terminated\n");
}