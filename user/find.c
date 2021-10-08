#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find_recur(const char* from, const char* key){
    int fd = open(from, 0);
    if(fd < 0){
        fprintf(2, "cannot open %s\n", from);
        exit(1);
    }
    struct stat st;
    if(fstat(fd, &st) < 0){
        fprintf(2, "canno stat %s\n", from);
        exit(1);
    }
    switch(st.type){
        case T_FILE:{
            const char *p=from+strlen(from);
            while(p>=from && *p != '/') p--;
            p++;
            if(!strcmp(p, key)){
                printf("%s\n", from);
            }
            break;
        }
        case T_DIR:{
            struct dirent de;
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0) continue;
                if(!strcmp(de.name, ".")) continue;
                if(!strcmp(de.name, "..")) continue;
                char buf[100]={0};
                strcpy(buf, from);
                char *p=buf+strlen(buf);
                if(*(p-1) != '/') *p++='/';
                memmove(p, de.name, strlen(de.name));
                p[strlen(de.name)] = 0;
                find_recur(buf, key);
            }
            break;
        }
    }
    close(fd);
}

int main(int argc, char* argv[]){
    if(argc < 3){
        write(2, "Need more args\n", 15);
        exit(1);
    }
    if(argc > 3){
        write(2, "Ignore redundant agrs\n", 22);
    }
    find_recur(argv[1], argv[2]);
    exit(0);
}
