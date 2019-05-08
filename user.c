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

    int fd = open(SERVER_FIFO_PATH, 0660);
    if (fd < 0)
    {
        exit(1);
    }

    if (strcmp(argv[1], "0") == 0)
    {
    }

    tlv_request_t tlv;
    tlv.type = atoi(argv[4]);

    //fill header
    tlv.value.header.pid = getpid();
    tlv.value.header.account_id = atoi(argv[1]);
    strcpy(tlv.value.header.password, argv[2]);
    tlv.value.header.op_delay_ms = atoi(argv[3]);

    uint32_t id;

    switch (tlv.type)
    {
    case OP_CREATE_ACCOUNT:
        id = atoi(strtok(argv[5], " "));
        tlv.value.create.account_id = id;
        uint32_t balance = atoi(strtok(argv[5], " "));
        tlv.value.create.balance = balance;
        strcpy(tlv.value.create.password, argv[5]);
        //tlv.length = 13;
        break;
    case OP_BALANCE:
        break;
    case OP_TRANSFER:
        id = atoi(strtok(argv[5], " "));
        tlv.value.transfer.account_id = id;
        tlv.value.transfer.amount = atoi(argv[5]);
        break;
    case OP_SHUTDOWN:
        break;
    }

    write(fd, &tlv, sizeof(tlv));

    unlink(SERVER_FIFO_PATH);
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