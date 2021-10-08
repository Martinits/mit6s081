#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int subprocess(int readpipe){
    int p=0, n=0, pid = -1, pipes[2]={-1, -1};
    while(1){
        int red = read(readpipe, &n, 4);
        if(red == 0) break;
        if(red != 4){
            write(2, "read not 4 bytes\n", 17);
            exit(1);
        }
        if(p == 0){
            p = n;
            printf("prime %d\n", p);
            n = 0;
        }else if(n%p != 0){
            if(pid == -1){
                pipe(pipes);
                pid = fork();
                if(pid == 0){
                    close(pipes[1]);
                    subprocess(pipes[0]);
                    exit(1);
                }else if(pid > 0){
                    close(pipes[0]);
                }else{
                    write(2, "fork failed\n", 12);
                    exit(1);
                }
            }
            write(pipes[1], &n, 4);
        }
    }
    close(readpipe);
    if(pipes[1] != -1) close(pipes[1]);
    wait((int*)0);
    exit(0);
}

int
main(int argc, char *argv[]){
    int pipes[2];
    pipe(pipes);
    int pid = fork();
    if(pid == 0){
        close(pipes[1]);
        subprocess(pipes[0]);
        exit(1);
    }else if(pid > 0){
        close(pipes[0]);
        for(int i=2;i<=35;i++){
            write(pipes[1], &i, 4);
        }
        close(pipes[1]);
        wait((int*)0);
        exit(0);
    }else{
        write(2, "fork failed\n", 12);
        exit(1);
    }
    exit(0);
}
