#ifndef SYNCH_H___
#define SYNCH_H___

int initializeMutex(int nr);

int destroyMutex(int nr);

int bankAccountLock(int id) ;

int bankAccountUnlock(int id);

int intializeSems() ;

int waitNotFull();

int waitNotEmpty();

int postNotFull();

int postNotEmpty();

int destroySems();

#endif 