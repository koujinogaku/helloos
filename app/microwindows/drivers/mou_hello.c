/*
 * Author: Tony Rogvall <tony@bluetail.com>
 *
 */
#include "string.h"
#include "mouse.h"

#include "device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  	MOU_Open(MOUSEDEVICE *pmd);
static void 	MOU_Close(void);
static int  	MOU_GetButtonInfo(void);
static void	MOU_GetDefaultAccel(int *pscale,int *pthresh);
static int  	MOU_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);
static int	MOU_Poll(void);

MOUSEDEVICE mousedev = {
    MOU_Open,
    MOU_Close,
    MOU_GetButtonInfo,
    MOU_GetDefaultAccel,
    MOU_Read,
    MOU_Poll,
    MOUSE_NORMAL	/* flags*/
};

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int MOU_origmode;
static int MOU_Open(MOUSEDEVICE *pmd)
{
    if (mouse_init() < 0)
		return -1;
    /* return the x11 file descriptor for select */
    if(mouse_request_code(TRUE) < 0)
	return -1;
    MOU_origmode=mouse_set_wait(FALSE);

    return 1;}

/*
 * Close the mouse device.
 */
static void
MOU_Close(void)
{
    mouse_request_code(FALSE);
    mouse_set_wait(MOU_origmode);
}

/*
 * Get mouse buttons supported
 */
static int
MOU_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_R;
}

/*
 * Get default mouse acceleration settings
 */
static void
MOU_GetDefaultAccel(int *pscale,int *pthresh)
{
    *pscale = SCALE;
    *pthresh = THRESH;
}

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */
static int
MOU_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
    int button=0,mdx=0,mdy=0;
    int buttons=0;
    int r;

    if((r=mouse_getcode(&button, &mdx, &mdy))<0)
        return -1;

    if(r==0)
      return 0;

    *dx = mdx;
    *dy = mdy;
    *dz = 0;

    if(button & 0x1)
        buttons |= MWBUTTON_L;
    if(button & 0x2)
        buttons |= MWBUTTON_R;
    *bp = buttons;

    return 1;		/* absolute position returned*/
}
int
MOU_Poll(void)
{
	if(mouse_poll()>0)
		return 1;
	else
		return 0;
}
