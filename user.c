#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "constants.h"
#include "types.h"
#include <time.h>

#include "sope.h"
#include "types.h"
#include "constants.h"
#include "log.c"

#define LENGTH_NAME 20

static char hexa[15] = {"123456789abcdef"};
void generateRandomSal(char *sal)
{

    for (int i = 1; i <= SALT_LEN; i++)
    {
        strcat(sal, &hexa[rand() % 15]);
    }
}

int main(int argc, char *argv[]) // USER //ID SENHA ATRASO DE OP OP(NR) STRING
{

   

    int fd = open(SERVER_FIFO_PATH, O_WRONLY);
    if (fd < 0)
    {
        exit(1);
    }

    if (strcmp(argv[1], "0") == 0)
    {
    }

    tlv_request_t request;
    request.type = atoi(argv[4]);

    //fill header
    request.value.header.pid = getpid();
    request.value.header.account_id = atoi(argv[1]);
    strcpy(request.value.header.password, argv[2]);
    request.value.header.op_delay_ms = atoi(argv[3]);

    uint32_t id;

    switch (request.type)
    {
    case OP_CREATE_ACCOUNT:
        id = atoi(strtok(argv[5], " "));
        request.value.create.account_id = id;
        uint32_t balance = atoi(strtok(argv[5], " "));
        request.value.create.balance = balance;
        strcpy(request.value.create.password, argv[5]);
        //request.length = 13;
        break;
    case OP_BALANCE:
        break;
    case OP_TRANSFER:
        id = atoi(strtok(argv[5], " "));
        request.value.transfer.account_id = id;
        request.value.transfer.amount = atoi(argv[5]);
        break;
    case OP_SHUTDOWN:
        break;
    }

    write(fd, &request, sizeof(request));
    logRequest(STDOUT_FILENO, id,&request);
    close(fd);
    // srand(time(NULL));
    // pid_t pid = getpid();
    // char fifoName[20] = USER_FIFO_PATH_PREFIX;
    // bank_account_t account;

    // if (strcmp(argv[1],"0") == 0)
    // {

    //     account.account_id = atoi(argv[1]);
    //     account.balance = atoi(argv[2]);
    // }

    // if (strcat(fifoName, atoi(pid)) == NULL)
    // {
    //     return 1;
    // }
    // if (mkfifo(fifoName, 0444) < 0)
    // {
    //     return 1;
    // }
}