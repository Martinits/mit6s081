#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    char *buf[MAXARG+1]={0};
    int bufi=argc-1;
    for(int i=1;i<argc;i++) buf[i-1]=argv[i];
    if(bufi == 0) buf[bufi++]="echo";
    char c, tmp[200]={0};
    int tmpi=0;
    while(1){
        tmpi=0;
        while(1){
            if(read(0, &c, 1) != 1) break;
            if(c=='\n') break;
            tmp[tmpi++]=c;
        }
        if(tmpi==0) break;
        tmp[tmpi]=0;
        int i=0, j=bufi;
        while(i < tmpi){
            while(i<tmpi && tmp[i]==' ') i++;
            if(i>=tmpi) break;
            int ii=i;
            while(i<tmpi && tmp[i]!=' ') i++;
            if(i==ii) break;
            tmp[i]=0;
            buf[j++]=tmp+ii;
            i++;
        }
        int pid = fork();
        if(pid == 0){
            exec(buf[0], buf);
            exit(0);
        }else if(pid > 0){
            wait((int*)0);
        }else{
            fprintf(2, "cannot fork");
            exit(1);
        }
    }
    exit(0);
}
