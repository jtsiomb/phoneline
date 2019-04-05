#include <stdio.h>
#include "ui.h"

static int mainwin_width = 400, mainwin_height = 300;
static int mainwin_mapped;

static void set_window_title(Window win, const char *title);

Window create_mainwin(void)
{
	int scr;
	Window win, root;
	long evmask;
	XClassHint chint;

	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);

	if(!(win = XCreateSimpleWindow(dpy, root, 10, 10, mainwin_width, mainwin_height,
			0, BlackPixel(dpy, scr)))) {
		fprintf(stderr, "failed to create window\n");
		return 0;
	}
	evmask = KeyPressMask | StructureNotifyMask | ButtonPressMask;
	XSelectInput(dpy, win, evmask);

	set_window_title(win, "xphoneline");

	XMapWindow(dpy, win);
	XRaiseWindow(dpy, win);

	XSetWMProtocols(dpy, win, &xa_wm_delete, 1);

	chint.res_name = chint.res_class = "xphoneline";
	XSetClassHint(dpy, win, &chint);

	return win;
}

void redraw_mainwin(void)
{
}

int handle_mainwin_event(XEvent *ev)
{
	switch(ev->type) {
	case Expose:
		if(ev->xexpose.count == 0 && mainwin_mapped) {
			redraw_mainwin();
		}
		break;

	case MapNotify:
		mainwin_mapped = 1;
		break;

	case UnmapNotify:
		mainwin_mapped = 0;
		break;

	case ClientMessage:
		if(ev->xclient.message_type == xa_wm_prot &&
				ev->xclient.data.l[0] == xa_wm_del_win) {
			return -1;
		}
		break;
	}
}

static void set_window_title(Window win, const char *title)
{
	XTextProperty wm_name;
	XStringListToTextProperty((char**)&title, 1, &wm_name);
	XSetWMName(dpy, win, &wm_name);
	XSetWMIconName(dpy, win, &wm_name);
	XFree(wm_name.value);
}
