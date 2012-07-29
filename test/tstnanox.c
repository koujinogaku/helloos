#define MWINCLUDECOLORS		1

#include "syscall.h"
#include "string.h"
#include "display.h"
#include "nano-X.h"
#include "device.h"
#include "print.h"
#include "stdlib.h"

int test_nanox(void);

static char s[64];

int start()
{
  test_nanox();
  return 0;
}

int test_nanox()
{
	GR_WINDOW_ID 	w;
	//GR_WINDOW_ID 	w2;
	GR_GC_ID	gc;
	GR_EVENT 	event;
	//GR_WM_PROPERTIES props;
	GR_FONT_ID	font;

	if (GrOpen() < 0) {
		display_puts("Cannot open graphics\n");
		exit(0);
	}

	GrSetErrorHandler(NULL);

#define WIDTH	320
#define HEIGHT	256
	w = GrNewWindow(GR_ROOT_WINDOW_ID, 20, 20, WIDTH, HEIGHT,
		0, GrGetSysColor(GR_COLOR_APPWINDOW), GrGetSysColor(GR_COLOR_WINDOWFRAME));

//	w2 = GrNewWindow(w, 20, 20, 40, 40, 0, BLUE, BLACK);

	//props.flags = GR_WM_FLAGS_PROPS | GR_WM_FLAGS_TITLE;
	//props.props = GR_WM_PROPS_NOBACKGROUND;
	//props.title = NULL;//"Nano-X Demo2";
	//GrSetWMProperties(w, &props);

	gc = GrNewGC();

	/*font = GrCreateFont("/tmp/lubI24.fnt", 0, NULL);*/
	//font = GrCreateFont("fonts/pcf/lubI24.pcf", 0, NULL);
	//font = GrCreateFont("system.fon", 0, NULL);
	font = GrCreateFont(GR_FONT_SYSTEM_VAR, 0, NULL);
	//font = GrCreateFont((GR_CHAR *)"system", 0, NULL);
	GrSetGCFont(gc, font);

	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ
		| GR_EVENT_MASK_BUTTON_DOWN
		| GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP);

	GrSelectEvents(w, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ
		| GR_EVENT_MASK_BUTTON_DOWN
		| GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP);

//	GrMapWindow(w2);
	GrMapWindow(w);
//	GrSetFocus(w2);
	GrSetFocus(w);



	for (;;) {
		/*GR_EVENT_KEYSTROKE *kev;*/

		GrGetNextEvent(&event);
		switch (event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			//GrSetGCForeground(gc,GrGetSysColor(GR_COLOR_APPWINDOW));
			//GrFillRect(w, gc, event.exposure.x, event.exposure.y,
			//	event.exposure.width, event.exposure.height);
			GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_APPTEXT));
			GrSetGCUseBackground(gc, GR_FALSE);
			GrText(w, gc, 10, 30, "Hello World !", -1, GR_TFASCII);
			GrSetGCForeground(gc,GREEN);
			GrRect(w, gc, 5, 5, 300, 60);
			GrSetGCForeground(gc,BLUE);
			GrEllipse(w, gc, 100, 100,  50, 30);

//GrSetGCForeground(gc,WHITE);
//GrLine(w, gc, 0,1,WIDTH-1,1);
//GrSetGCForeground(gc,BLACK);
//GrLine(w, gc, 0,2,WIDTH-1,2);
//print_format("white=%x black=%x ", WHITE, BLACK);

			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
			GrClose();
display_puts("close event\n");
			exit(0);
			break;
		case GR_EVENT_TYPE_ERROR:
			print_format("demo2: Error (%s) ", event.error.name);
display_puts("{");
display_puts(event.error.name);
display_puts("}");
			print_format("(code=%d)",event.error.code);
			print_format(nxErrorStrings[event.error.code],event.error.id);

			break;
#if 0
		case GR_EVENT_TYPE_BUTTON_DOWN:
			GrUnmapWindow(w);
			GrFlush();
			syscall_wait(1);
			GrMapWindow(w);
			break;
#endif
#if 1
		case GR_EVENT_TYPE_BUTTON_DOWN:
			/* test server error on bad  syscall*/
			//GrMapWindow(w2);
			//GrMoveWindow(GR_ROOT_WINDOW_ID, 0, 0);
			GrMapWindow(w);
			GrMoveWindow(w, event.button.rootx, event.button.rooty);
			{ GR_SCREEN_INFO sinfo; GrGetScreenInfo(&sinfo); }
			break;
#endif
#if 0
		case GR_EVENT_TYPE_KEY_DOWN:
			kev = (GR_EVENT_KEYSTROKE *)&event;
			printf("DOWN %d (%04x) %04x\n",
				kev->ch, kev->ch, kev->modifiers);
			break;
		case GR_EVENT_TYPE_KEY_UP:
			kev = (GR_EVENT_KEYSTROKE *)&event;
			printf("UP %d (%04x) %04x\n",
				kev->ch, kev->ch, kev->modifiers);
			break;
#endif
		}
	}

	syscall_wait(100);
	GrClose();
	return 0;
}
/*
void dummy()
{
	GR_WINDOW_ID 	w;
	//GR_WINDOW_ID 	w2;
	//GR_GC_ID	gc;
	//GR_EVENT 	event;
	//GR_WM_PROPERTIES props;
	//GR_FONT_ID	font;


	GrMapWindow(w);
	GrSetFocus(w);
	//serious bug here: when wm running, w2 is mapped anyway!!
	//GrMapWindow(w2);


	GrClose();
	return ;
}
*/

