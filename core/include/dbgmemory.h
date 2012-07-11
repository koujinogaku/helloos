#ifndef DBGMEMORY_H
#define DBGMEMORY_H

void *dbg_malloc(unsigned long size);
void dbg_mfree(void *mm);

#endif /* DBGMEMORY_H */
