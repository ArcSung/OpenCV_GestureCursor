#include "mouseCtrl.h"
#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

//When called , it simulates a click at the curent mouse cursor location
/*void mouseClick()
{
	int button=Button1;
	Display *display = XOpenDisplay(NULL);

	XEvent event;

	if(display == NULL)
	{
		printf("Error connecting to display");
		exit(EXIT_FAILURE);
	}
	memset(&event, 0x00, sizeof(event));

	event.type = ButtonPress;
	event.xbutton.button = button;
	event.xbutton.same_screen = True;
	XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
	event.xbutton.subwindow = event.xbutton.window;
	while(event.xbutton.subwindow)
	{
		event.xbutton.window = event.xbutton.subwindow;
		XQueryPointer(display, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
	}

	if(XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0) cout<<"ERROR SENDING CLICK"<<endl;
	
	XFlush(display);

	XCloseDisplay(display);
}*/

//When called, it simulates a mouse release event at the current cursor location
/*void mouseRelease()
{
	int button=Button1;
	Display *display = XOpenDisplay(NULL);

	XEvent event;

	if(display == NULL)
	{
		cout<<"Error connecting to display"<<endl;
		exit(EXIT_FAILURE);
	}
	memset(&event, 0x00, sizeof(event));

	event.xbutton.button = button;
	event.xbutton.same_screen = True;
	XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
	event.xbutton.subwindow = event.xbutton.window;
	while(event.xbutton.subwindow)
	{
		event.xbutton.window = event.xbutton.subwindow;
		XQueryPointer(display, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
	}


	event.type = ButtonRelease;
#include<iostream>
	event.xbutton.state = 0x100;

	if(XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0) cout<<"ERROR RELEASING"<<endl;

	XFlush(display);

	XCloseDisplay(display);
}*/


//This function moves the mouse cursor to (x,y)
void mouseTo(int x,int y)
{
	 Display *display = XOpenDisplay(0);
	  Window root = DefaultRootWindow(display);
	  XWarpPointer(display, None, root, 0, 0, 0, 0, x, y);
	  XFlush(display);
	  XCloseDisplay(display);
}
