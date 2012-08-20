/*
 * Copyright (c) 1999, 2000, 2001, 2003, 2004 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 by Koninklijke Philips Electronics N.V.
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Main module of graphics server.
 */
#define MWINCLUDECOLORS		1
#include "portunixstd.h"
#include "memory.h"
#include "bucket.h"
#include "message.h"
#include "print.h"
#include "mouse.h"
#include "keyboard.h"
#include "stdlib.h"

#include "serv.h"
#include "mwtypes.h"

/*
 * External definitions defined here.
 */
GR_WINDOW_ID	cachewindowid=0;		/* cached window id */
GR_WINDOW_ID    cachepixmapid=0;         /* cached pixmap id */
GR_GC_ID	cachegcid=0;		/* cached graphics context id */
GR_WINDOW	*cachewp=0;		/* cached window pointer */
GR_GC		*cachegcp=0;		/* cached graphics context */
GR_PIXMAP       *cachepp=0;               /* cached pixmap */
GR_PIXMAP       *listpp=0;                /* List of all pixmaps */
GR_WINDOW	*listwp=0;		/* list of all windows */
GR_WINDOW	*rootwp=0;		/* root window pointer */
GR_GC		*listgcp=0;		/* list of all gc */
GR_REGION	*listregionp=0;		/* list of all regions */
GR_FONT		*listfontp=0;		/* list of all fonts */
#if MW_FEATURE_IMAGES
GR_IMAGE	*listimagep=0;		/* list of all images */
#endif
GR_CURSOR	*listcursorp=0;		/* list of all cursors */
GR_CURSOR	*stdcursor=0;		/* root window cursor */
GR_GC		*curgcp=0;		/* currently enabled gc */
GR_WINDOW	*clipwp=0;		/* window clipping is set for */
GR_WINDOW	*focuswp=0;		/* focus window for keyboard */
GR_WINDOW	*mousewp=0;		/* window mouse is currently in */
GR_WINDOW	*grabbuttonwp=0;		/* window grabbed by button */
GR_CURSOR	*curcursor=0;		/* currently enabled cursor */
GR_COORD	cursorx=0;		/* current x position of cursor */
GR_COORD	cursory=0;		/* current y position of cursor */
GR_BUTTON	curbuttons=0;		/* current state of buttons */
GR_CLIENT	*curclient=0;		/* client currently executing for */
GR_EVENT_LIST	*eventfree=0;		/* list of free events */
GR_BOOL		focusfixed=0;		/* TRUE if focus is fixed on a window */
PMWFONT		stdfont=0;		/* default font*/
int		escape_quits = TRUE;	/* terminate when pressing ESC */
char		*progname=0;		/* Name of this program.. */

int		current_fd=0;		/* the fd of the client talking to */
int		connectcount = 0;	/* number of connections to server */
GR_CLIENT	*root_client=0;		/* root entry of the client table */
GR_CLIENT	*current_client=0;	/* the client we are currently talking*/
char		*current_shm_cmds=0;
int		current_shm_cmds_size=0;
static int	keyb_fd=0;		/* the keyboard file descriptor */
#ifdef MW_FEATURE_TWO_KEYBOARDS
static int	keyb2_fd=0;		/* the keyboard file descriptor */
#endif
static int	mouse_fd=0;		/* the mouse file descriptor */
char		*curfunc=0;		/* the name of the current server func*/
GR_BOOL		screensaver_active=0;	/* time before screensaver activates */
GR_SELECTIONOWNER selection_owner;	/* the selection owner and typelist */
GR_TIMEOUT	startTicks=0;		/* ms time server started*/
int		autoportrait = FALSE;	/* auto portrait mode switching*/
MWCOORD		nxres=0;			/* requested server x resolution*/
MWCOORD		nyres=0;			/* requested server y resolution*/

#if MW_FEATURE_TIMERS
GR_TIMEOUT	screensaver_delay=0;	/* time before screensaver activates */
GR_TIMER_ID     cache_timer_id=0;         /* cached timer ID */
GR_TIMER        *cache_timer=0;           /* cached timer */
GR_TIMER        *list_timer=0;            /* list of all timers */
#endif /* MW_FEATURE_TIMERS */
GR_GRABBED_KEY  *list_grabbed_keys = NULL;     /* list of all grabbed keys */

static int	persistent_mode = FALSE;
static int	portraitmode = MWPORTRAIT_NONE;

SERVER_LOCK_DECLARE /* Mutex for all public functions (only if NONETWORK and THREADSAFE) */

#if !NONETWORK
int		un_sock=0;		/* the server socket descriptor */

static void
usage(void)
{
	print_format("Usage: %s [-e] [-p] [-A] [-NLRD] [-x#] [-y#] [-c <fontconfig-file> ...]\n",
		progname);
	exit(1);
}

/*
 * This is the main server loop which initialises the server, services
 * the clients, and shuts the server down when there are no more clients.
 */
int
#if HELLOOS
start(int argc, char *argv[])
#else
main(int argc, char *argv[])
#endif
{
	int t;

	progname = argv[0];

	t = 1;
	while ( t < argc ) {
		if ( !strncmp("-e",argv[t],3)) {
			escape_quits = FALSE;
			++t;
			continue;
		}
		if ( !strncmp("-p",argv[t],3) ) {
			persistent_mode = TRUE;
			++t;
			continue;
		}
		if ( !strncmp("-A",argv[t],3) ) {
			autoportrait = TRUE;
			++t;
			continue;
		}
		if ( !strncmp("-N",argv[t],3) ) {
			portraitmode = MWPORTRAIT_NONE;
			++t;
			continue;
		}
		if ( !strncmp("-L",argv[t],3) ) {
			portraitmode = MWPORTRAIT_LEFT;
			++t;
			continue;
		}
		if ( !strncmp("-R",argv[t],3) ) {
			portraitmode = MWPORTRAIT_RIGHT;
			++t;
			continue;
		}
		if ( !strncmp("-D",argv[t],3) ) {
			portraitmode = MWPORTRAIT_DOWN;
			++t;
			continue;
		}
		if ( !strncmp("-x",argv[t],3) ) {
			if (++t >= argc)
				usage();
			nxres = atoi(argv[t]);
			++t;
			continue;
		}
		if ( !strncmp("-y",argv[t],3) ) {
			if (++t >= argc)
				usage();
			nyres = atoi(argv[t]);
			++t;
			continue;
		}
#if FONTMAPPER
		if ( !strncmp("-c",argv[t],3) ) {
			int read_configfile(char *file);

			if (++t >= argc)
				usage();
			read_configfile(argv[t]);
			++t;
			continue;
		}
#endif
		usage();
	}

	/* Attempt to initialise the server*/
	if(GsInitialize() < 0)
		exit(1);

	while(1)
		GsSelect(0L);
	return 0;
}
#endif

void
GsAcceptClientFd(int i)
{
	GR_CLIENT *client, *cl;

	if(!(client = malloc(sizeof(GR_CLIENT)))) {
#if NONETWORK && UNIX
		close(i);
#endif
		return;
	}

	client->id = i;
	client->eventhead = NULL;
	client->eventtail = NULL;
	/*client->errorevent.type = GR_EVENT_TYPE_NONE;*/
	client->next = NULL;
	client->prev = NULL;
	client->waiting_for_event = FALSE;
	client->shm_cmds = 0;

	if(connectcount++ == 0)
		root_client = client;
	else {
		cl = root_client;
			while(cl->next)
				cl = cl->next;
		client->prev = cl;
		cl->next = client;
	}
}

/*
 * Open a connection from a new client to the server.
 * Returns -1 on failure.
 */
int
GrOpen(void)
{
#if NONETWORK
	SERVER_LOCK();
	escape_quits = 1;

	/* Client calls this routine once.  We 
	 * init everything here
	 */
	if (connectcount <= 0) {
		if(GsInitialize() < 0) {
			SERVER_UNLOCK();
			return -1;
		}
		GsAcceptClientFd(999);
		curclient = root_client;
	}
	SERVER_UNLOCK();
#endif
        return 1;
}

/*
 * Close the current connection to the server.
 */
void
GrClose(void)
{
	SERVER_LOCK();
	GsClose(current_fd);
	SERVER_UNLOCK();
}

/*
 * Drop a specific server connection.
 */
void
GsClose(int fd)
{
	GsDropClient(fd);
	if(!persistent_mode && connectcount == 0)
		GsTerminate();
}

#if NONETWORK
/* client/server GsDropClient is in srvnet.c*/
void
GsDropClient(int fd)
{
	--connectcount;
}
#endif

#if UNIX
#if NONETWORK && defined(HAVESELECT)
/*
 * Register the specified file descriptor to return an event
 * when input is ready.
 */

static int regfdmax = -1;
static fd_set regfdset;

void
GrRegisterInput(int fd)
{
	SERVER_LOCK();
	FD_SET(fd, &regfdset);
	if (fd >= regfdmax) regfdmax = fd + 1;
	SERVER_UNLOCK();
}

void
GrUnregisterInput(int fd)
{
	int i, max;

	SERVER_LOCK();

	/* unregister all inputs if the FD is -1 */
	if (fd == -1) {
		FD_ZERO(&regfdset);
		regfdmax = -1;
		SERVER_UNLOCK();
		return;
	}

	FD_CLR(fd, &regfdset);
	/* recalculate the max file descriptor */
	for (i = 0, max = regfdmax, regfdmax = -1; i < max; i++)
		if (FD_ISSET(i, &regfdset))
			regfdmax = i + 1;

	SERVER_UNLOCK();
}

#endif /* NONETWORK && HAVESELECT */
#endif /* UNIX */

/*
 * This function suspends execution of the program for the specified
 * number of milliseconds.
 */
void
GrDelay(GR_TIMEOUT msecs)
{
#if UNIX && defined(HAVESELECT)
	struct timeval timeval;

	timeval.tv_sec = msecs / 1000;
	timeval.tv_usec = (msecs % 1000) * 1000;
	select(0, NULL, NULL, NULL, &timeval);
#endif
}

#if NONETWORK
void
GrFlush(void)
{
}

void
GrMainLoop(GR_FNCALLBACKEVENT fncb)
{
	GR_EVENT event;

	for(;;) {
		GrGetNextEvent(&event);
		fncb(&event);
	}
}

void
GrReqShmCmds(long shmsize)
{
	/* no action required, no client/server*/
}
#endif


#if MSDOS | _MINIX 

void
GsSelect(GR_TIMEOUT timeout)
{
	/* If mouse data present, service it*/
	if(mousedev.Poll())
		while(GsCheckMouseEvent())
			continue;

	/* If keyboard data present, service it*/
	if(kbddev.Poll())
		while(GsCheckKeyboardEvent())
			continue;

}

#elif UNIX && defined(HAVESELECT)

void
GsSelect(GR_TIMEOUT timeout)
{
	fd_set	rfds;
	int 	e;
	int	setsize = 0;
	struct timeval tout;
	struct timeval *to;
#if NONETWORK
	int	fd;
#endif

	/* perform pre-select duties, if any*/
	if(rootwp->psd->PreSelect)
		rootwp->psd->PreSelect(rootwp->psd);

	/* Set up the FDs for use in the main select(): */
	FD_ZERO(&rfds);
	if(mouse_fd >= 0) {
		FD_SET(mouse_fd, &rfds);
		if (mouse_fd > setsize)
			setsize = mouse_fd;
	}
	if(keyb_fd >= 0) {
		FD_SET(keyb_fd, &rfds);
		if (keyb_fd > setsize)
			setsize = keyb_fd;
	}
#ifdef MW_FEATURE_TWO_KEYBOARDS
	if(keyb2_fd >= 0) {
		FD_SET(keyb2_fd, &rfds);
		if (keyb2_fd > setsize)
			setsize = keyb2_fd;
	}
#endif
#if NONETWORK
	/* handle registered input file descriptors*/
	for (fd = 0; fd < regfdmax; fd++) {
		if (!FD_ISSET(fd, &regfdset))
			continue;

		FD_SET(fd, &rfds);
		if (fd > setsize) setsize = fd;
	}
#else /* not NONETWORK */
	/* handle client socket connections*/
	FD_SET(un_sock, &rfds);
	if (un_sock > setsize) setsize = un_sock;
	curclient = root_client;
	while(curclient) {
		if(curclient->waiting_for_event && curclient->eventhead) {
			curclient->waiting_for_event = FALSE;
			GrGetNextEventWrapperFinish(curclient->id);
			return;
		}
		FD_SET(curclient->id, &rfds);
		if(curclient->id > setsize) setsize = curclient->id;
		curclient = curclient->next;
	}
#endif /* NONETWORK */
	/* Set up the timeout for the main select(): */
	if (timeout == (GR_TIMEOUT) -1L) {
		/* poll*/
		tout.tv_sec = 0;
		tout.tv_usec = 0;
		to = &tout;
	} else {
#if MW_FEATURE_TIMERS
		if(GdGetNextTimeout(&tout, timeout) == TRUE)
			to = &tout;
		else
#endif /* MW_FEATURE_TIMERS */
			to = NULL;
	}

	/* Wait for some input on any of the fds in the set or a timeout: */
	if((e = select(setsize+1, &rfds, NULL, NULL, to)) > 0) {
		/* If data is present on the mouse fd, service it: */
		if(mouse_fd >= 0 && FD_ISSET(mouse_fd, &rfds))
			while(GsCheckMouseEvent())
				continue;

		/* If data is present on the keyboard fd, service it: */
		if( (keyb_fd >= 0 && FD_ISSET(keyb_fd, &rfds))
#ifdef MW_FEATURE_TWO_KEYBOARDS
		 || (keyb2_fd >= 0 && FD_ISSET(keyb2_fd, &rfds))
#endif
		  )
			while(GsCheckKeyboardEvent())
				continue;

#if NONETWORK
		/* check for input on registered file descriptors */
		for (fd = 0; fd < regfdmax; fd++) {
			GR_EVENT_FDINPUT *	gp;

			if (!FD_ISSET(fd, &regfdset)  ||  !FD_ISSET(fd, &rfds))
				continue;

			gp =(GR_EVENT_FDINPUT *)GsAllocEvent(curclient);
			if(gp) {
				gp->type = GR_EVENT_TYPE_FDINPUT;
				gp->fd = fd;
			}
		}
#else /* not NONETWORK */

		/* If a client is trying to connect, accept it: */
		if(FD_ISSET(un_sock, &rfds))
			GsAcceptClient();

		/* If a client is sending us a command, handle it: */
		curclient = root_client;
		while(curclient) {
			GR_CLIENT *curclient_next;

			/* curclient may be freed in GsDropClient*/
			curclient_next = curclient->next;
			if(FD_ISSET(curclient->id, &rfds))
				GsHandleClient(curclient->id);
			curclient = curclient_next;
		}
#endif /* NONETWORK */
	} 
	else if (e == 0) {
#if NONETWORK
		/* 
		 * Timeout has occured.  Currently return
		 * a timeout event regardless of whether
		 * client has selected for it.
		 * Note: this will be changed back to GR_EVENT_TYPE_NONE
		 * for the GrCheckNextEvent/LINK_APP_TO_SERVER case
		 */
#if MW_FEATURE_TIMERS
		if(GdTimeout() == TRUE)
#endif
		{
			GR_EVENT_GENERAL *	gp;
			gp = (GR_EVENT_GENERAL *)GsAllocEvent(curclient);
			if(gp)
				gp->type = GR_EVENT_TYPE_TIMEOUT;
		}
#else /* not NONETWORK */
#if MW_FEATURE_TIMERS
		GdTimeout();
#endif
#endif /* NONETWORK */
	} else
		if(errno != EINTR)
			EPRINTF("Select() call in main failed\n");
}



#if NONETWORK
/*
 * Prepare for the client to call select().  Asks the server to send the next
 * event but does not wait around for it to arrive.  Initializes the
 * specified fd_set structure with the client/server socket descriptor and any
 * previously registered external file descriptors.  Also compares the current
 * contents of maxfd, the client/server socket descriptor, and the previously
 * registered external file descriptors, and returns the highest of them in
 * maxfd.
 *
 * Usually used in conjunction with GrServiceSelect().
 *
 * Note that in a multithreaded client, the application must ensure that
 * no Nano-X calls are made between the calls to GrPrepareSelect() and
 * GrServiceSelect(), else there will be race conditions.
 *
 * @param maxfd  Pointer to a variable which the highest in use fd will be
 *               written to.  Must contain a valid value on input - will only
 *               be overwritten if the new value is higher than the old
 *               value.
 * @param rfdset Pointer to the file descriptor set structure to use.  Must
 *               be valid on input - file descriptors will be added to this
 *               set without clearing the previous contents.
 */
void
GrPrepareSelect(int *maxfd, void *rfdset)
{
	fd_set *rfds = (fd_set *) rfdset;
	int fd;

	SERVER_LOCK();

	/* perform pre-select duties, if any*/
	if(rootwp->psd->PreSelect)
		rootwp->psd->PreSelect(rootwp->psd);

	if(mouse_fd >= 0) {
		FD_SET(mouse_fd, rfds);
		if (mouse_fd > *maxfd)
			*maxfd = mouse_fd;
	}
	if(keyb_fd >= 0) {
		FD_SET(keyb_fd, rfds);
		if (keyb_fd > *maxfd)
			*maxfd = keyb_fd;
	}
#ifdef MW_FEATURE_TWO_KEYBOARDS
	if(keyb2_fd >= 0) {
		FD_SET(keyb2_fd, rfds);
		if (keyb2_fd > *maxfd)
			*maxfd = keyb2_fd;
	}
#endif

	/* handle registered input file descriptors*/
	for (fd = 0; fd < regfdmax; fd++) {
		if (!FD_ISSET(fd, &regfdset))
			continue;

		FD_SET(fd, rfds);
		if (fd > *maxfd)
			*maxfd = fd;
	}

	SERVER_UNLOCK();
}

/*
 * Handles events after the client has done a select() call.
 *
 * Calls the specified callback function is an event has arrived, or if
 * there is data waiting on an external fd specified by GrRegisterInput().
 *
 * Used by GrMainLoop().
 *
 * @param rfdset Pointer to the file descriptor set containing those file
 *               descriptors that are ready for reading.
 * @param fncb   Pointer to the function to call when an event needs handling.
 */
void
GrServiceSelect(void *rfdset, GR_FNCALLBACKEVENT fncb)
{
	fd_set *	rfds = rfdset;
	GR_EVENT_LIST *	elp;
	GR_EVENT 	ev;
	int fd;

	SERVER_LOCK();

	/* If data is present on the mouse fd, service it: */
	if(mouse_fd >= 0 && FD_ISSET(mouse_fd, rfds))
		while(GsCheckMouseEvent())
			continue;

	/* If data is present on the keyboard fd, service it: */
	if( (keyb_fd >= 0 && FD_ISSET(keyb_fd, rfds))
#ifdef MW_FEATURE_TWO_KEYBOARDS
	 || (keyb2_fd >= 0 && FD_ISSET(keyb2_fd, rfds))
#endif
	  )
		while(GsCheckKeyboardEvent())
			continue;

	/* Dispatch all queued events */
	while((elp = curclient->eventhead) != NULL) {

		ev = elp->event;

		/* Remove first event from queue*/
		curclient->eventhead = elp->next;
		if (curclient->eventtail == elp)
			curclient->eventtail = NULL;

		elp->next = eventfree;
		eventfree = elp;

		fncb(&ev);
	}

	/* check for input on registered file descriptors */
	for (fd = 0; fd < regfdmax; fd++) {
		if (!FD_ISSET(fd, &regfdset) || !FD_ISSET(fd, rfds))
			continue;

		ev.type = GR_EVENT_TYPE_FDINPUT;
		ev.fdinput.fd = fd;
		fncb(&ev);
	}

	SERVER_UNLOCK();
}

#endif /* NONETWORK */

#endif /* UNIX && defined(HAVESELECT)*/


// HELLO OS NONETWORK
#if HELLOOS && NONETWORK
void
GsSelect(GR_TIMEOUT timeout)
{
	/* If mouse data present, service it*/
	while(mousedev.Poll()) {
		GsCheckMouseEvent();
		continue;
	}
	/* If keyboard data present, service it*/
	while(kbddev.Poll()) {
		GsCheckKeyboardEvent();
		continue;
	}
	syscall_wait(1);
}
#elif HELLOOS && !NONETWORK && defined(HAVESELECT)/* BUCKET Network */
void
GsSelect(GR_TIMEOUT timeout)
{
	fd_set	rfds;
	int 	e;
	int	setsize = 0;
	int r;
#if 0
	struct msg_head bucketmsg;
#endif
	unsigned long timeout_for_select;
#if MW_FEATURE_TIMERS
	struct timeval tout;
#endif /* MW_FEATURE_TIMERS */

	/* perform pre-select duties, if any*/
	if(rootwp->psd->PreSelect)
		rootwp->psd->PreSelect(rootwp->psd);

	/* Set up the FDs for use in the main select(): */
	FD_ZERO(&rfds);
	/* Set up requester for use in the main message_receive(): */
	r=keyboard_request_key();

	/* handle client socket connections*/
	FD_SET(un_sock, &rfds);
	if (un_sock > setsize) setsize = un_sock;

	/* handle client connections*/
	curclient = root_client;
	while(curclient) {
		if(curclient->waiting_for_event && curclient->eventhead) {
			curclient->waiting_for_event = FALSE;
			GrGetNextEventWrapperFinish(curclient->id);
			return;
		}
		FD_SET(curclient->id, &rfds);
		if(curclient->id > setsize) setsize = curclient->id;
		curclient = curclient->next;
	}

#if MW_FEATURE_TIMERS
	/* Set up the timeout for the main select(): */
	if (timeout == (GR_TIMEOUT) -1L) {
		/* poll*/
		timeout_for_select=1;
	} else {
		if(GdGetNextTimeout(&tout, timeout) == TRUE) {
			timeout_for_select = tout.tv_sec*1000 + tout.tv_usec/1000;
			if(timeout_for_select==0)
				timeout_for_select=1;
		}
		else {
			timeout_for_select=0;
		}
	}
#else
	timeout_for_select=0;
#endif /* MW_FEATURE_TIMERS */
	/* Wait for some input on any of the fds in the set or a timeout: */
	if((e = bucket_select(setsize+1,  &rfds, timeout_for_select)) > 0) {
		if(mousedev.Poll()) {
			/* If data is present on the mouse fd, service it: */
			while(GsCheckMouseEvent()) {
				continue;
			}
		}

		/* If data is present on the keyboard fd, service it: */
		while(kbddev.Poll()) {
			GsCheckKeyboardEvent();
		}

		if(e==BUCKET_SELECT_DATA) {
			if(FD_ISSET(un_sock,&rfds)) {
				/* If a client is trying to connect, accept it: */
				GsAcceptClient();
			}
			else {
				/* If a client is sending us a command, handle it: */
				/* msg->command ==> source qid ==> client id, becouse command-field can be used to select */

				/* If a client is sending us a command, handle it: */
				curclient = root_client;
				while(curclient) {
					GR_CLIENT *curclient_next;
					/* curclient may be freed in GsDropClient*/
					curclient_next = curclient->next;
					if(FD_ISSET(curclient->id,&rfds)) {
						GsHandleClient(curclient->id);
					}
					curclient = curclient_next;
				}
			}
		}
#if 0
		else {
			bucketmsg.size=sizeof(struct msg_head);
			r=message_poll(MESSAGE_MODE_TRY, 0, 0, &bucketmsg);
			if(r>0 && bucketmsg.service!=KBD_SRV_KEYBOARD && bucketmsg.service!=MOU_SRV_MOUSE) {
				r=message_receive(MESSAGE_MODE_TRY, 0, 0, &bucketmsg);
				EPRINTF("Receive a dust block srv=%x,cmd=%x\n",bucketmsg.service,bucketmsg.command);
			}
		}
#endif
	}
	else if(e==0) {
#if MW_FEATURE_TIMERS
		GdTimeout();
#endif
	}
	else
		EPRINTF("Select() call in main failed\n");


}
#elif HELLOOS && !NONETWORK && !defined(HAVESELECT)/* BUCKET Network */
void
GsSelect(GR_TIMEOUT timeout)
{
	int r,fd;
	struct msg_head bucketmsg;
#if MW_FEATURE_TIMERS
	unsigned long timeout_for_select;
	int timeout_alarm=0;
	struct timeval tout;
#endif /* MW_FEATURE_TIMERS */

	/* perform pre-select duties, if any*/
	if(rootwp->psd->PreSelect)
		rootwp->psd->PreSelect(rootwp->psd);

	/* Set up requester for use in the main message_receive(): */
	r=keyboard_request_key();

	/* handle client connections*/
	curclient = root_client;
	while(curclient) {
		if(curclient->waiting_for_event && curclient->eventhead) {
			curclient->waiting_for_event = FALSE;
			GrGetNextEventWrapperFinish(curclient->id);
			return;
		}
		curclient = curclient->next;
	}

	/* If data is present on the mouse fd, service it: */
	if(mousedev.Poll()) {
		GsCheckMouseEvent();
	}

	/* If data is present on the keyboard fd, service it: */
	while(kbddev.Poll()) {
		GsCheckKeyboardEvent();
	}

#if MW_FEATURE_TIMERS
	/* Set up the timeout for the main select(): */
	timeout_for_select=0;
	if (timeout == (GR_TIMEOUT) -1L) {
		/* poll*/
		timeout_for_select=1;
	} else {
		if(GdGetNextTimeout(&tout, timeout) == TRUE) {
			timeout_for_select = tout.tv_sec*1000 + tout.tv_usec/1000;
			if(timeout_for_select==0)
				timeout_for_select=1;
		}
	}

	if(timeout_for_select != 0) {
		timeout_alarm = syscall_alarm_set(timeout_for_select/10,environment_getqueid(), BUCKET_ALARM_COMMAND<<16);
	}
#endif /* MW_FEATURE_TIMERS */

	bucketmsg.size=sizeof(struct msg_head);
	r = message_poll(MESSAGE_MODE_WAIT, 0, 0, &bucketmsg);
	if(r<0) {
		EPRINTF("Select() call in main failed\n");
		return;
	}
#if MW_FEATURE_TIMERS
	if(timeout_for_select != 0) {
		if(bucketmsg.service==MSG_SRV_ALARM && bucketmsg.command==BUCKET_ALARM_COMMAND) {
			bucketmsg.size=sizeof(struct msg_head);
			r=message_receive(MESSAGE_MODE_TRY, MSG_SRV_ALARM, BUCKET_ALARM_COMMAND, &bucketmsg);
		}
		else {
			syscall_alarm_unset(timeout_alarm,environment_getqueid());
		}
	}
#endif /* MW_FEATURE_TIMERS */

	if(bucketmsg.service==WNX_SRV_WINDOW) {
		fd=bucket_find_dsc(bucketmsg.service, bucketmsg.command);
		if(fd<0) {
			EPRINTF("Receive a dust block srv=%x,cmd=%x\n",bucketmsg.service,bucketmsg.command);
			bucketmsg.size=sizeof(struct msg_head);
			r=message_receive(MESSAGE_MODE_TRY, bucketmsg.service, bucketmsg.command, &bucketmsg);
		}
		else if(fd==un_sock) {
				/* If a client is trying to connect, accept it: */
				GsAcceptClient();
		}
		else {
				/* If a client is sending us a command, handle it: */
				/* msg->command ==> source qid ==> client id, becouse command-field can be used to select */

				/* If a client is sending us a command, handle it: */
				curclient = root_client;
				while(curclient) {
					GR_CLIENT *curclient_next;
					/* curclient may be freed in GsDropClient*/
					curclient_next = curclient->next;
					if(fd==curclient->id) {
						for(;;) {
							GsHandleClient(curclient->id);
							if(bucket_has_block(fd)<=0)
								break;
						}
					}
					curclient = curclient_next;
				}
		}
	}
	else if(bucketmsg.service==MSG_SRV_ALARM) {
#if MW_FEATURE_TIMERS
		GdTimeout();
#endif
	}
	else if(bucketmsg.service!=KBD_SRV_KEYBOARD && bucketmsg.service!=MOU_SRV_MOUSE){
			EPRINTF("Receive a dust block srv=%x,cmd=%x\n",bucketmsg.service,bucketmsg.command);
			bucketmsg.size=sizeof(struct msg_head);
			r=message_receive(MESSAGE_MODE_TRY, 0, 0, &bucketmsg);
	}
}
#endif /* HELLOOS && !NONETWORK && !defined(HAVESELECT) */

#if VTSWITCH
static void
CheckVtChange(void *arg)
{
	if(MwCheckVtChange())
		GsRedrawScreen();
	GdAddTimer(50, CheckVtChange, NULL);
}
#endif
  
/*
 * Initialize the graphics and mouse devices at startup.
 * Returns nonzero with a message printed if the initialization failed.
 */
int
GsInitialize(void)
{
	GR_WINDOW	*wp;		/* root window */
	PSD		psd;
	GR_CURSOR_ID	cid;
	static MWIMAGEBITS cursorbits[16] = {
	      0xe000, 0x9800, 0x8600, 0x4180,
	      0x4060, 0x2018, 0x2004, 0x107c,
	      0x1020, 0x0910, 0x0988, 0x0544,
	      0x0522, 0x0211, 0x000a, 0x0004
	};
	static MWIMAGEBITS cursormask[16] = {
	      0xe000, 0xf800, 0xfe00, 0x7f80,
	      0x7fe0, 0x3ff8, 0x3ffc, 0x1ffc,
	      0x1fe0, 0x0ff0, 0x0ff8, 0x077c,
	      0x073e, 0x021f, 0x000e, 0x0004
	};

	/* If needed, initialize the server mutex. */
	SERVER_LOCK_INIT();

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	wp = (GR_WINDOW *) malloc(sizeof(GR_WINDOW));
	if (wp == NULL) {
		EPRINTF("Cannot allocate root window\n");
		return -1;
	}

	startTicks = GsGetTickCount();

#ifndef MW_NOSIGNALS
	/* catch terminate signal to restore tty state*/
	signal(SIGTERM, (void *)GsTerminate);
#endif

#if MW_FEATURE_TIMERS
	screensaver_delay = 0;
#endif
	screensaver_active = GR_FALSE;

	selection_owner.wid = 0;
	selection_owner.typelist = NULL;

#if !NONETWORK
#ifndef MW_NOSIGNALS
	/* ignore pipe signal, sent when clients exit*/
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
#endif

	if (GsOpenSocket() < 0) {
		EPRINTF("Cannot bind to named socket\n");
		free(wp);
		return -1;
	}
#endif

	if ((keyb_fd = GdOpenKeyboard()) == -1) {
		EPRINTF("Cannot initialise keyboard\n");
		/*GsCloseSocket();*/
		free(wp);
		return -1;
	}

#ifdef MW_FEATURE_TWO_KEYBOARDS
	if ((keyb2_fd = GdOpenKeyboard2()) == -1) {
		EPRINTF("Cannot initialise second keyboard\n");
		/*GsCloseSocket();*/
		free(wp);
		return -1;
	}
#endif

	if ((psd = GdOpenScreen()) == NULL) {
		EPRINTF("Cannot initialise screen\n");
		/*GsCloseSocket();*/
		GdCloseKeyboard();
		free(wp);
		return -1;
	}
	GdSetPortraitMode(psd, portraitmode);

	if ((mouse_fd = GdOpenMouse()) == -1) {
		EPRINTF("Cannot initialise mouse\n");
		/*GsCloseSocket();*/
		GdCloseScreen(psd);
		GdCloseKeyboard();
		free(wp);
		return -1;
	}

	/*
	 * Create std font.
	 */
#if (HAVE_BIG5_SUPPORT | HAVE_GB2312_SUPPORT | HAVE_JISX0213_SUPPORT | HAVE_KSC5601_SUPPORT)
	/* system fixed font looks better when mixed with builtin fixed fonts*/
	stdfont = GdCreateFont(psd, (char *)MWFONT_SYSTEM_FIXED, 0, NULL);
#else
	stdfont = GdCreateFont(psd, (char *)MWFONT_SYSTEM_VAR, 0, NULL);
#endif

	/*
	 * Initialize the root window.
	 */
	wp->psd = psd;
	wp->id = GR_ROOT_WINDOW_ID;
	wp->parent = NULL;		/* changed: was = NULL*/
	wp->owner = NULL;
	wp->children = NULL;
	wp->siblings = NULL;
	wp->next = NULL;
	wp->x = 0;
	wp->y = 0;
	wp->width = psd->xvirtres;
	wp->height = psd->yvirtres;
	wp->bordersize = 0;
	wp->background = BLACK;
	wp->bordercolor = wp->background;
	wp->nopropmask = 0;
	wp->bgpixmap = NULL;
	wp->bgpixmapflags = GR_BACKGROUND_TILE;
	wp->eventclients = NULL;
	wp->cursorid = 0;
	wp->mapped = GR_TRUE;
	wp->realized = GR_TRUE;
	wp->output = GR_TRUE;
	wp->props = 0;
	wp->title = NULL;
	wp->clipregion = NULL;

        listpp = NULL;
	listwp = wp;
	rootwp = wp;
	focuswp = wp;
	mousewp = wp;
	focusfixed = GR_FALSE;

	/*
	 * Initialize and position the default cursor.
	 */
	curcursor = NULL;
	cursorx = -1;
	cursory = -1;
	GdShowCursor(psd);
	GrMoveCursor(psd->xvirtres / 2, psd->yvirtres / 2);
	cid = GrNewCursor(16, 16, 0, 0, WHITE, BLACK, cursorbits, cursormask);
	GrSetWindowCursor(GR_ROOT_WINDOW_ID, cid);
	stdcursor = GsFindCursor(cid);

#if VTSWITCH
	MwInitVt();
	/* Check for VT change every 50 ms: */
	GdAddTimer(50, CheckVtChange, NULL);
#endif
	psd->FillRect(psd, 0, 0, psd->xvirtres-1, psd->yvirtres-1,
		GdFindColor(psd, wp->background));

	/*
	 * Tell the mouse driver some things.
	 */
	curbuttons = 0;
	GdRestrictMouse(0, 0, psd->xvirtres - 1, psd->yvirtres - 1);
	GdMoveMouse(psd->xvirtres / 2, psd->yvirtres / 2);

	/* Force root window screen paint*/
	GsRedrawScreen();

	/* 
	 * Force the cursor to appear on the screen at startup.
	 * (not required with above GsRedrawScreen)
	GdHideCursor(psd);
	GdShowCursor(psd);
	 */

	/*
	 * All done.
	 */
	connectcount = 0;
	return 0;
}

/*
 * Here to close down the server.
 */
void
GsTerminate(void)
{
#if !NONETWORK
	GsCloseSocket();
#endif
	GdCloseScreen(rootwp->psd);
	GdCloseMouse();
	GdCloseKeyboard();
#if VTSWITCH
	MwRedrawVt(mwvterm);
#endif
	exit(0);
}

/*
 * Return # milliseconds elapsed since start of Microwindows
 * Granularity is 25 msec
 */
GR_TIMEOUT
GsGetTickCount(void)
{
#if MSDOS
#include <time.h>
	return (unsigned long)(clock() * 1000 / CLOCKS_PER_SEC);
#else
#if _MINIX
	struct tms	t;
	
	return (unsigned long)times(&t) * 16;
#else
#if UNIX
	struct timeval t;

	gettimeofday(&t, NULL);
	return ((t.tv_sec * 1000) + (t.tv_usec / 25000) * 25) - startTicks;
#else
	return 0L;
#endif
#endif
#endif
}

void
GrBell(void)
{
	//SERVER_LOCK();
	//write(2, "\7", 1);
	//SERVER_UNLOCK();
}
