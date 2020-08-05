#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_SIZE 64

FILE* fptr;
char add[] = "ADD";
char del[] = "DEL";
int CLK = 0;
int next_ts = -1;
char new_msg[MAX_SIZE];
int process_id;
key_t down, up;
char ops[1000][4];
char msgs[1000][MAX_SIZE];
bool sent[1000];
void continue_process(FILE* fptr);

struct msgbuff
{
   long mtype;
   char msg[MAX_SIZE];
   int process_id;
   bool hard_busy;
   msgqnum_t msg_qnum;
};

struct msgbuff process_msg;
struct msgbuff msg_to_kernel;
ssize_t recieve_status;



void readfile(char * filename){
    // read process's orders to kernal 
    fptr = fopen(filename, "r");
    while(!feof(fptr)){
        int ts;
        fscanf(fptr, "%d", &ts);
        fscanf(fptr, "%s", ops[ts]);
        fscanf(fptr, "%s", msgs[ts]);
    }
}


void continue_process(FILE* fptr){
    if(strcmp(msgs[CLK], "\0") != 0 && !sent[CLK]){
        sent[CLK] = 1;
        if(strcmp(ops[CLK], add) == 0){
            new_msg[0] = 'A';
            strcpy(new_msg+1, msgs[CLK]);
        }
        else if (strcmp(ops[CLK], del) == 0){
            new_msg[0] = 'D';
            strcpy(new_msg+1, msgs[CLK]);
        }
        else 
            return;
        msg_to_kernel.process_id = process_id;
        strcpy(msg_to_kernel.msg, new_msg);
        int send_status = msgsnd(up, &msg_to_kernel, sizeof(msg_to_kernel), !IPC_NOWAIT);
        if(send_status == -1){
            printf("cannot send the msg %s \n", msg_to_kernel.msg);
        }
    }
}


void sigusr2Handler(int sig){
    CLK+=1;
    
}

int main(int argc, char* argv[]){
    for(int i = 0 ; i < 1000 ; i++){
        strcpy(msgs[i], "\0");
        sent[i] = 0;
    }
    up = atol(argv[1]);
    char* filename= argv[2];
    process_id = atol(filename);
    readfile(filename);
    printf("Process #%d\n",process_id);
    signal(SIGUSR2, sigusr2Handler);
    msg_to_kernel.mtype = 3;
    while(1){
        
        continue_process(fptr);
    }

}