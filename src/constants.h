#pragma once

#define MAX_BANK_OFFICES 99
#define MAX_BANK_ACCOUNTS 4096
#define REQUESTS_QUEUE_LEN 4096
#define MIN_BALANCE 1UL
#define MAX_BALANCE 1000000000UL
#define ADMIN_ACCOUNT_ID 0
#define ADMIN_ACCOUNT_BALLANCE 0
#define MAIN_THREAD_ID 0

#define MIN_PASSWORD_LEN 8
#define MAX_PASSWORD_LEN 20
#define HASH_LEN 64
#define SALT_LEN 64

#define MAX_OP_DELAY_MS 99999

#define WIDTH_ID 5
#define WIDTH_DELAY 5
#define WIDTH_ACCOUNT 4
#define WIDTH_BALANCE 10
#define WIDTH_OP 8
#define WIDTH_RC 12
#define WIDTH_HASH 5
#define WIDTH_TLV_LEN 3
#define WIDTH_STARTEND 5

#define SERVER_LOGFILE "slog.txt"
#define USER_LOGFILE "ulog.txt"

#define SHM_NAME "/shm"
#define SERVER_FIFO_PATH "/tmp/secure_srv"
#define USER_FIFO_PATH_PREFIX "/tmp/secure_"
#define USER_FIFO_PATH_LEN (sizeof(USER_FIFO_PATH_PREFIX) + WIDTH_ID + 1)

#define FIFO_TIMEOUT_SECS 30

#define PASSWORD_ERROR 1
#define ID_ERROR 2
#define OP_CODE_ERROR 3
#define DELAY_ERROR 4
#define THREADS_NO_ERROR 5
#define FIFO_ERROR 6
#define WRITE_ERROR 7
#define BALANCE_ERROR 8
#define ARG_NUM_ERROR 9