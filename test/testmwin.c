#include "device.h"
#include "syscall.h"
#include "string.h"
#include "windows.h"
#include "wintern.h"
#include "display.h"

#define FIRSTUSERPALENTRY	24  /* first writable pal entry over 16 color*/
extern int	gr_firstuserpalentry;

int test_engine(void);
int test_mouse_key(void);
void GsSelect(void);
int tstColorGd=1;
char s[64];
PSD psd;
extern MWPALENTRY	mwstdpal8[256];

int start()
{
  //test_driver();
  //test_engine();
  //test_engine2();
  test_mouse_key();
  return 0;
}
#define PRINT_ERROR(x) display_puts(x)

/*
int gr_mode=0;
MWPIXELVAL gr_background=0;
MWPALENTRY gr_palette[256];
int GdError(const char *fmt, ...)
{
  syscall_puts(fmt);
}
*/
int test_driver()
{
  int x1,x2,y1,y2,c;
  MWSCREENINFO  sinfo;
  MWPALENTRY *stdpal;

  psd = scrdev.Open(&scrdev);
  if(!psd)
    return NULL;

  GdGetScreenInfo(psd, &sinfo);
  sint2dec(sinfo.pixtype,s);
  display_puts("pixtype=");
  display_puts(s);
  display_puts("\nncolors=");
  sint2dec(sinfo.ncolors,s);
  display_puts(s);
  display_puts("\npsd.ncolors=");
  sint2dec((int)psd->ncolors,s);
  display_puts(s);
  display_puts("\n");

  //gr_firstuserpalentry = FIRSTUSERPALENTRY;
  GdResetPalette();
  stdpal = mwstdpal8;
  GdSetPalette(psd, 0, (int)psd->ncolors, stdpal);

  //scrdev.FillRect(&scrdev,0,0,639,479,0);
  //x1=100; x2=540; y1=100; y2= 380; c=7;
  //scrdev.DrawHorzLine(&scrdev,x1,x2,y1,c);
  //scrdev.DrawVertLine(&scrdev,x2,y1,y2,c);
  //scrdev.DrawHorzLine(&scrdev,x1,x2,y2,c);
  //scrdev.DrawVertLine(&scrdev,x1,y1,y2,c);

  //syscall_wait(100);

  scrdev.Close(&scrdev);

  return 0;
}

int test_engine2()
{
  psd = GdOpenScreen();
  GdCloseScreen(psd);
  return 0;
}


int test_engine()
{
  MWCOORD x1,x2,y1,y2;
  unsigned long mask=0;
  int count=0;
  int y;

  psd = GdOpenScreen();
  if(psd==NULL) {
    syscall_puts("GdOpenScreen error\n");
    return 0;
  }

  GdSetForegroundPixelVal(psd, 7);
  GdSetBackgroundPixelVal(psd, 0);
  GdSetDash(&mask, &count);

  x1=100; x2=540; y1=100; y2= 380;
  GdLine(psd, x1, y1, x2, y1, 0);
  GdLine(psd, x2, y1, x2, y2, 0);
  GdLine(psd, x1, y2, x2, y2, 0);
  GdLine(psd, x1, y1, x1, y2, 0);

  for(y=0;y<8;y++) {
    GdSetForegroundPixelVal(psd, y);
    GdLine(psd, x1, y1+y, x2, y1+y, 0);
  }

  GdSetForegroundPixelVal(psd, 7);
  GdLine(psd, x1, y1, x2, y2, 0);

  syscall_wait(1000);
  
  GdCloseScreen(psd);

  return 0;
}

int		keyb_fd;		// the keyboard file descriptor
int		mouse_fd;		// the mouse file descriptor

int test_mouse_key()
{
	unsigned long mask=0;
	int count=0;

display_puts("key open\n");
	if ((keyb_fd = GdOpenKeyboard()) < 0) {
		PRINT_ERROR("Cannot initialise keyboard");
		return -1;
	}

	psd = GdOpenScreen();
	if(psd==NULL) {
		syscall_puts("GdOpenScreen error\n");
		GdCloseKeyboard();
		return -1;
	}

display_puts("mouse open\n");
	if ((mouse_fd = GdOpenMouse()) < 0) {
		PRINT_ERROR("Cannot initialise mouse");
		GdCloseScreen(psd);
		GdCloseKeyboard();
		return -1;
	}

	GdSetForegroundPixelVal(psd, 7);
	GdSetBackgroundPixelVal(psd, 0);
	GdSetDash(&mask, &count);

	for(;;) {
		GsSelect();
	}
	return 0;
}
void
GsTerminate(void)
{
	GdCloseMouse();
	GdCloseScreen(psd);
	GdCloseKeyboard();
	exit(0);
}
//
// Update mouse status and issue events on it if necessary.
// This function doesn't block, but is normally only called when
// there is known to be some data waiting to be read from the mouse.
//
BOOL GsCheckMouseEvent(void)
{
	MWCOORD		rootx;		// latest mouse x position
	MWCOORD		rooty;		// latest mouse y position
	int		newbuttons;	// latest buttons
	int		mousestatus;	// latest mouse status

	// Read the latest mouse status: 
display_puts("mouse read\n");
	mousestatus = GdReadMouse(&rootx, &rooty, &newbuttons);
	if(mousestatus < 0) {
		display_puts("Mouse error\n");
		return FALSE;
	} else if(mousestatus) { // Deliver events as appropriate: 
/*
		display_puts("mouse ");
		sint2dec(rootx,s);
		display_puts(s);
		display_puts(",");
		sint2dec(rooty,s);
		display_puts(s);
		display_puts(",");
		long2hex(newbuttons,s);
		display_puts(s);
		display_puts("\n");
*/
		if(newbuttons&0x1)
			tstColorGd++;
		GdSetForegroundPixelVal(psd, tstColorGd);
		GdPoint(psd, rootx, rooty);
		return TRUE;
	}
	return FALSE;
}

//
// Update keyboard status and issue events on it if necessary.
// This function doesn't block, but is normally only called when
// there is known to be some data waiting to be read from the keyboard.
//
BOOL GsCheckKeyboardEvent(void)
{
	MWKEY		ch;		// latest character
	MWKEYMOD	modifiers;	// latest modifiers 
	MWSCANCODE	scancode;	// latest modifiers 
	int		keystatus;	// latest keyboard status 

	// Read the latest keyboard status: 
display_puts("key read\n");
	keystatus = GdReadKeyboard(&ch, &modifiers, &scancode);
	if(keystatus < 0) {
		if(keystatus == -2)	// special case for ESC pressed
			GsTerminate();
		display_puts("Kbd error\n");
		return FALSE;
	} else if(keystatus) { // Deliver events as appropriate: 
		byte2hex(modifiers,s);
		display_puts("kbd '");
		display_putc(ch);
		display_puts("',");
		display_puts(s);
		display_puts("\n");
		return TRUE;
	}
	return FALSE;
}
void
GsSelect(void)
{
	int m,k;
	// If mouse data present, service it
	m=mousedev.Poll();
	if(m)
		GsCheckMouseEvent();

	// If keyboard data present, service it
	k=kbddev.Poll();
	if(k)
		GsCheckKeyboardEvent();

	if((!m) && (!k)) {
		syscall_wait(1);
	}

}
