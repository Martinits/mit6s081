#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define ERROR_MSG1 "ERROR: Required exactly one argument for the time you want to sleep\n"
#define ERROR_MSG2 "ERROR: call sysproc sleep failed\n"
#define WARNING_MSG "WARNING: Ignore redundant arguments\n"

int
main(int argc, char * argv[])
{
	if(argc <= 1){
		write(2, ERROR_MSG1, strlen(ERROR_MSG1));
		exit(1);
	}
	if(argc > 2){
		write(2, WARNING_MSG, strlen(WARNING_MSG));
	}
	int seconds = atoi(argv[1]);
	if(0 != sleep(seconds)){
		write(2, ERROR_MSG2, strlen(ERROR_MSG2));
		exit(1);
	}
	exit(0);
}
