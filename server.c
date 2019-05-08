#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#include "sope.h"
#include "types.h"
#include "constants.h"
#include "log.c"

int main(int argc, char **argv)
{
    if (argc != 3 )
    {
        printf("Usage: ./server <number of bank offices> <admin password>");
        return 1;
    }

    if (mkfifo(SERVER_FIFO_PATH, 0666)) {
        if (errno==EEXIST) {
            printf("FIFO '/tmp/requests' already exists\n");
            exit(1);
        } 
        else {
            printf("Can't create FIFO\n");
            exit(2);
        } 
    }
        
    int fd = open(SERVER_FIFO_PATH,0444);
    if (fd < 0) {
        exit(1);
    }

    int bank_offices_no = atoi(argv[1]);

    bank_account_t admin_account;
    admin_account.account_id = ADMIN_ACCOUNT_ID;
    admin_account.balance = ADMIN_ACCOUNT_BALLANCE;
    

    logAccountCreation(STDOUT_FILENO, ADMIN_ACCOUNT_ID, &admin_account);

    for (int i = 1; i <= bank_offices_no; i++) {
         logBankOfficeOpen(STDOUT_FILENO, i, i);
    }


    for (int i = 1; i <= bank_offices_no; i++) {
         logBankOfficeClose(STDOUT_FILENO, i, i);
    }

    if (unlink(SERVER_FIFO_PATH) < 0) {
        printf("Error destryoing fifo\n");
        exit(2);
    }
}