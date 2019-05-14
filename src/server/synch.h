#ifndef SYNCH_H___
#define SYNCH_H___

int initializeMutex(int nr);

int destroyMutex(int nr);

int bankOfficeLock(int id) ;

int bankOfficeUnlock(int id);

int intializeSems() ;

int waitNotFull();

int waitNotEmpty();

int postNotFull();

int postNotEmpty();

int destroySems();

#endif 