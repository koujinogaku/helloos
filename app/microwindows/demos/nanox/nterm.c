/*
 * Nano-X terminal emulator
 *
 * Al Riddoch
 * Greg Haerr
 */
#include "portunixstd.h"
#include "environm.h"
#include "print.h"
#include "errno.h"
#include "memory.h"
#include "keyboard.h"
#include "display.h"
#include "syscall.h"
#include "message.h"
#include "bucket.h"

#define MWINCLUDECOLORS
#include "nano-X.h"

#define HAVEBLIT 0		/* set if have bitblit (experimental)*/

#define	_	((unsigned) 0)		/* off bits */
#define	X	((unsigned) 1)		/* on bits */
#define	MASK(a,b,c,d,e,f,g) \
	(((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) \
	+ e) * 2) + f) * 2) + g) << 9)

#if DOS_DJGPP
#define SIGCHLD		17 /* from Linux */
#endif

static GR_WINDOW_ID	w1;	/* id for window */
static GR_GC_ID		gc1;	/* graphics context */
static GR_GC_ID		gc3;	/* graphics context */
static GR_COORD		xpos;	/* x coord for text */
static GR_COORD		ypos;	/* y coord for text */
static GR_SCREEN_INFO	si;	/* screen info */
//static int 		tfdMaster,tfdSlave;

#define KEYBUFFER_SIZE 128
struct keybuff {
	int read_pos;
	int write_pos;
	int buffer[KEYBUFFER_SIZE];
};

static struct keybuff keybuffer;
static struct keybuff keyreq;

static int taskid=0;


void text_init(void);
int term_init(void);
void do_keystroke(GR_EVENT_KEYSTROKE *kp);
void do_focusin(GR_EVENT_GENERAL *gp);
void do_focusout(GR_EVENT_GENERAL *gp);
void do_fdinput(void);
void printg(char * text);
void HandleEvent(GR_EVENT *ep);
void char_del(GR_COORD x, GR_COORD y);
void char_out(GR_CHAR ch);
void sigchild(int signo);

int start(int argc, char ** argv)
{
	GR_BITMAP	bitmap1fg[7];	/* mouse cursor */
	GR_BITMAP	bitmap1bg[7];
	//GR_FONT_ID	font;

	memset(&keybuffer,0,sizeof(keybuffer));
	memset(&keyreq,0,sizeof(keyreq));

	if (GrOpen() < 0) {
		printf("cannot open graphics\n");
		exit(1);
	}
	
	GrGetScreenInfo(&si);

	w1 = GrNewWindow(GR_ROOT_WINDOW_ID, 50, 30, si.cols - 10,
		si.rows - 60, 1, WHITE, LTBLUE);

	GrSelectEvents(w1, GR_EVENT_MASK_BUTTON_DOWN |
		GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_EXPOSURE |
		GR_EVENT_MASK_FOCUS_IN | GR_EVENT_MASK_FOCUS_OUT |
		GR_EVENT_MASK_CLOSE_REQ);

	GrMapWindow(w1);

	gc1 = GrNewGC();
	gc3 = GrNewGC();

	GrSetGCForeground(gc1, GRAY);
	GrSetGCBackground(gc1, LTBLUE);
	GrSetGCFont(gc1, GrCreateFontEx((GR_CHAR*)GR_FONT_SYSTEM_FIXED, 0, 0, NULL));
	//font = GrCreateFontEx(GR_FONT_SYSTEM_VAR, 0, 0, NULL);
	//GrSetGCFont(gc1, font);
	//GrSetGCForeground(gc1, GrGetSysColor(GR_COLOR_APPTEXT));
	//GrSetGCUseBackground(gc1, GR_FALSE);


	/*GrSetGCFont(gc1, GrCreateFontEx(GR_FONT_OEM_FIXED, 0, 0, NULL));*/
	GrSetGCForeground(gc3, WHITE);
	GrSetGCBackground(gc3, BLACK);

	bitmap1fg[0] = MASK(_,_,X,_,X,_,_);
	bitmap1fg[1] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[2] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[3] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[4] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[5] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[6] = MASK(_,_,X,_,X,_,_);

	bitmap1bg[0] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[1] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[2] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[3] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[4] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[5] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[6] = MASK(_,X,X,X,X,X,_);

	GrSetCursor(w1, 7, 7, 3, 3, WHITE, BLACK, bitmap1fg, bitmap1bg);

	/*GrFillRect(GR_ROOT_WINDOW_ID, gc1, 0, 0, si.cols, si.rows);*/

	GrSetGCForeground(gc1, BLACK);
	GrSetGCBackground(gc1, WHITE);
	text_init();
	if (term_init() < 0) {
		GrClose();
		exit(1);
	}
	/* we want tfd events also*/
	//GrRegisterInput(tfdMaster);

	//printg("Hello\n");

#if 1
	GrMainLoop(HandleEvent);
#else
	while(1) {
		GR_EVENT ev;

		GrGetNextEvent(&ev);
		HandleEvent(&ev);
	}
#endif
	/* notreached*/
	return 0;
}

void
HandleEvent(GR_EVENT *ep)
{
	int exitcode;

	switch (ep->type) {
		case GR_EVENT_TYPE_EXPOSURE:
			//GrText(w1, gc1, 10, 30, "Hello World !", -1, GR_TFASCII);
			break;

		case GR_EVENT_TYPE_KEY_DOWN:
			do_keystroke(&ep->keystroke);
			break;

		case GR_EVENT_TYPE_FOCUS_IN:
			do_focusin(&ep->general);
			break;

		case GR_EVENT_TYPE_FOCUS_OUT:
			do_focusout(&ep->general);
			break;

		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
			environment_kill(taskid);
			environment_wait(&exitcode,MESSAGE_MODE_WAIT,0,0);
			exit(0);

		case GR_EVENT_TYPE_FDINPUT:
			do_fdinput();
			break;

	}
}

char * nargv[2] = {"command.out", NULL};

void sigchild(int signo)
{
	printg("We have a signal right now!\n");
	GrClose();
	exit(0);
}

#if HELLOOS
int term_init(void)
{
	char *session;
	int queid;

	session=environment_copy_session();
	queid = environment_getqueid();
	environment_make_args(session, 2, nargv);
	environment_make_display(session, queid);
	environment_make_keyboard(session, queid);

	taskid = environment_exec(nargv[0], session);
	mfree(session);
	if(taskid==ERRNO_NOTEXIST) {
		printf("%s is not found.\n", nargv[0]);
		return -1;
	}
	if(taskid<0) {
		printf("Can't exec %s(%d)\n", nargv[0], -taskid);
		return -1;
	}

	return 0;
}
#else
int term_init(void)
{
	char pty_name[12];
	int n = 0;
	pid_t pid;

again:
	sprintf(pty_name, "/dev/ptyp%d", n);
	if ((tfdMaster = open(pty_name, O_RDWR | O_NONBLOCK)) < 0) {
		if ((errno == EBUSY || errno == EIO) && n < 10) {
			n++;
			goto again;
		}
		fprintf(stderr, "Can't create pty %s\n", pty_name);
		return -1;
	}
	//signal(SIGCHLD, sigchild);
	//signal(SIGINT, sigchild);

#ifdef __uClinux__
#undef fork
#define fork() vfork()
#endif
	if ((pid = fork()) == -1) {
		fprintf(stderr, "No processes\n");
		return -1;
	}
	if (!pid) {
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		close(tfdMaster);
		setsid();
		pty_name[5] = 't';
		if ((tfdSlave = open(pty_name, O_RDWR)) < 0) {
			fprintf(stderr, "Child: Can't open pty %s\n", pty_name);
			exit(1);
		}
		dup2(tfdSlave, STDIN_FILENO);
		dup2(tfdSlave, STDOUT_FILENO);
		dup2(tfdSlave, STDERR_FILENO);
		execv(nargv[0], nargv);
		exit(1);
	}
	return 0;
}
#endif

GR_SIZE		width;		/* width of character */
GR_SIZE		height;		/* height of character */
GR_SIZE		base;		/* height of baseline */

void text_init(void)
{
	GrGetGCTextSize(gc1, "A", 1, GR_TFASCII, &width, &height, &base);
}

void char_del(GR_COORD x, GR_COORD y)
{
	xpos -= width;
	GrFillRect(w1, gc3, x, y /*- height*/ /*+ base*/ + 1, width, height);
}
	
void char_out(GR_CHAR ch)
{
	switch(ch) {
	case '\r':
		xpos = 0;
		return;
	case '\n':
		xpos = 0; // for HelloOS
		ypos += height;
		if(ypos > si.rows - 60 - height) {
			ypos -= height;
#if HAVEBLIT
			bogl_cfb8_blit(50, 30, si.cols-120,
				si.rows-60-height, 50, 30+height);
			GrFillRect(w1, gc3, 50, ypos, si.cols-120, height);
#else
			/* FIXME: changing FALSE to TRUE crashes nano-X*/
			/* clear screen, no scroll*/
			ypos = 0;
			GrClearWindow(w1, GR_FALSE);
#endif
		}
		return;
	case '\007':			/* bel*/
		return;
	case '\t':
		xpos += width;
		while((xpos/width) & 7)
			char_out(' ');
		return;
	case '\b':			/* assumes fixed width font!!*/
		if (xpos <= 0)
			return;
		char_del(xpos, ypos);
		return;
	}
	GrText(w1, gc1, xpos+1, ypos, &ch, 1, GR_TFTOP);
	xpos += width;
}

void printg(char * text)
{
	int i;

	for(i = 0; i < strlen(text); i++) {
		char_out(text[i]);
	}
}


/*
 * Here when a keyboard press occurs.
 */
int keybuffer_inc_pos(int pos)
{
	pos++;
	if(pos>=KEYBUFFER_SIZE)
		pos=0;
	return pos;
}
int key_push(struct keybuff *buff, int ch)
{
	int pos;

	pos = keybuffer_inc_pos(buff->write_pos);
	if(pos==buff->read_pos)
		return 0;
	buff->buffer[buff->write_pos]=ch;
	buff->write_pos=pos;
	return 0;
}
int key_pop(struct keybuff *buff)
{
	int ch;

	if(buff->read_pos==buff->write_pos)
		return -1;
	ch = buff->buffer[buff->read_pos];
	buff->read_pos= keybuffer_inc_pos(buff->read_pos);
	return ch;
}

int key_send(int queid, int ch)
{
	union kbd_msg key;
	int r;

	key.res.h.size=sizeof(key);
	key.res.h.service=KBD_SRV_KEYBOARD;
	key.res.h.command=KBD_CMD_GETCODE;
	key.res.key = ch;
	r=message_send(queid, &key);
	if(r<0)
		printf("Send key error to %d\n", queid);
	return 0;
}

void
do_keystroke(GR_EVENT_KEYSTROKE *kp)
{
	char queid;

	queid = key_pop(&keyreq);
	if(queid<0)
		key_push(&keybuffer,kp->ch);
	else
		key_send(queid, kp->ch);
}

void
msg_keyrequest(struct msg_head *msg)
{
	union kbd_msg key;
	int ch,r;

	key.res.h.size=sizeof(key);
	r=message_receive(MESSAGE_MODE_WAIT, msg->service, msg->command, &key);
	if(r<0) {
		printf("key request error\n");
		return;
	}
	if(key.req.h.command!=KBD_CMD_GETCODE) {
		printf("key request error\n");
		return;
	}
	ch = key_pop(&keybuffer);
	if(ch<0)
		key_push(&keyreq, key.req.queid);
	else
		key_send(key.req.queid, ch);
}

/*
 * Here when a focus in event occurs.
 */
void
do_focusin(GR_EVENT_GENERAL *gp)
{
	if (gp->wid != w1)
		return;
	GrSetBorderColor(w1, LTBLUE);
}

/*
 * Here when a focus out event occurs.
 */
void
do_focusout(GR_EVENT_GENERAL *gp)
{
	if (gp->wid != w1)
		return;
	GrSetBorderColor(w1, GRAY);
}

void
msg_dsprequest(struct msg_head	*msg)
{
	union display_msg dsp;
	int i,r;
	char *str;

	dsp.h.size=sizeof(dsp);
	r=message_receive(MESSAGE_MODE_WAIT, msg->service, msg->command, &dsp);
	if(r<0) {
		printf("dsp request error\n");
		return;
	}
	switch(dsp.h.command) {
	case DISPLAY_CMD_PUTC:
		char_out(dsp.h.arg);
		break;
	case DISPLAY_CMD_PUTS:
		str=dsp.s.s;
		for(i=0; *str!=0 && i<msg->arg; ++i,++str)
			char_out(*str);
		break;
	}
}

void
msg_exitstatus(struct msg_head	*msg)
{
	struct msg_head status;
	int r;

	status.size=sizeof(status);
	r=message_receive(MESSAGE_MODE_WAIT, msg->service, msg->command, &status);
	if(r<0) {
		printf("exit request error\n");
		return;
	}
	r = syscall_pgm_delete(status.arg);
	if(r<0) {
		printf("exit request error\n");
		return;
	}
	sigchild(r);
}
/*
 * receive a message from outside the windows system.
 */
void
do_fdinput(void)
{
	struct msg_head	*msg;
	struct msg_head	foo;

	msg=bucket_selected_msg();

	switch(msg->service) {
	case KBD_SRV_KEYBOARD:
		msg_keyrequest(msg);
		break;
	case DISPLAY_SRV_DISPLAY:
		msg_dsprequest(msg);
		break;
	case MSG_SRV_KERNEL:
		msg_exitstatus(msg);
	default:
		printf("invaid message\n");
		foo.size=sizeof(foo);
		message_receive(MESSAGE_MODE_TRY, msg->service, msg->command, &foo);
	}
}
