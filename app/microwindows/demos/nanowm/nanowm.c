/*
 * NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2000, 2010 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 * Parts based on npanel.c Copyright (C) 1999 Alistair Riddoch.
 */
#include "portunixstd.h"
#include "print.h"

#include "nano-X.h"
#include "nanowm.h"

int start(int argc, char *argv[])
{
	GR_EVENT event;

	if(GrOpen() < 0) {
		printf("nanowm: Couldn't connect to Nano-X server!\n");
		exit(1);
	}

	/* pass errors through main loop, don't exit*/
	GrSetErrorHandler(NULL);

	/* init window mgr, background color*/
	wm_init();

	/* main loop*/
	while(1) { 
		GrGetNextEvent(&event);
		wm_handle_event(&event);
	}

	GrClose();
}
