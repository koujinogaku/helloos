#ifndef MW_LOCK_H
#define MW_LOCK_H
/*
 * Critical section locking definitions for Microwindows
 * Copyright (c) 2002, 2003 by Greg Haerr <greg@censoft.com>
 *
 * The current implementation uses pthreads included in libc
 *
 * It's currently required that any locking mechanism
 * allow multiple locks on the same thread (ie. recursive calls)
 * This is necessary since routines nest calls on
 * LOCK(&nxGlobalLock). (nanox/client.c and nanox/nxproto.c)
 */

/* no locking support - dummy macros*/
#if !THREADSAFE
typedef int		MWMUTEX;

#define LOCK_DECLARE(name)	MWMUTEX name
#define LOCK_EXTERN(name)	extern MWMUTEX name
#define LOCK_INIT(m)
#define LOCK_FREE(m)
#define LOCK(m)
#define UNLOCK(m)
#endif /* !THREADSAFE*/

#endif
