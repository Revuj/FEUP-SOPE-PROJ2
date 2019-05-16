#ifndef SYNCH_H___
#define SYNCH_H___

int initializeMutex(int nr);

int destroyMutex(int nr);

int bankAccountLock(int id) ;

int bankAccountUnlock(int id);

int initializeSemNotFull(int semsNo);

int initializeSemNotEmpty(int semsNo);

int waitNotFull();

int waitNotEmpty();

int postNotFull();

int postNotEmpty();

int getvalueNotFull();

int getvalueNotEmpty();

int destroySems();

#endif 