#ifndef UI_H_
#define UI_H_

#include <X11/Xlib.h>

Display *dpy;
Atom xa_wm_proto, xa_wm_delete;

Window create_mainwin(void);
void redraw_mainwin(void);
int handle_mainwin_event(XEvent *ev);

#endif	/* UI_H_ */
