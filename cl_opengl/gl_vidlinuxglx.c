/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#ifdef OS_LINUX
#include <sys/vt.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>

#include <GL/glx.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>

#ifdef USE_DGA
#include <X11/extensions/xf86dga.h>
#endif

#ifdef USE_VMODE
#include <X11/extensions/xf86vmode.h>
#endif

#include "quakedef.h"

#define WARP_WIDTH		320
#define WARP_HEIGHT		200

int mouse_buttons=3; // dummy for screen.c

// BorisU -->
#ifdef USE_VMODE
static qboolean vidmode_ext = false;
static XF86VidModeModeInfo **vidmodes;
static int num_vidmodes;
static qboolean vidmode_active = false;
//static XF86VidModeGamma oldgamma;
#endif
static int scrnum;

void InitHWGamma (void);
void RestoreHWGamma (void);

qboolean V_OnHWgammaChange (cvar_t *var, const char *value);
cvar_t	vid_hwgammacontrol = {"vid_hwgammacontrol","1", 0, V_OnHWgammaChange}; 

qboolean	vid_gammaworks = false;
qboolean	vid_hwgamma_enabled = false;
qboolean	vid_hwgammaproblems = false;
unsigned short (*currentgammaramp) [3][256] = NULL;

// <-- BorisU

static Display *dpy = NULL;
static Window win;
static GLXContext ctx = NULL;

static float old_windowed_mouse = 0;

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
			PointerMotionMask)

#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask)

// compatibility with old Quake -- setting to 0 disables KP_* codes
cvar_t	cl_keypad = {"cl_keypad","0"}; // BorisU

qboolean OnChange_windowed_mouse(cvar_t *, const char *);
cvar_t	_windowed_mouse = {"_windowed_mouse", "1", CVAR_ARCHIVE, OnChange_windowed_mouse};

cvar_t	vid_mode = {"vid_mode","0"};
 
static float	mouse_x, mouse_y;
static float	old_mouse_x, old_mouse_y;

cvar_t	m_filter = {"m_filter", "0"};

static int scr_width, scr_height;

/*-----------------------------------------------------------------------*/

float	gldepthmin, gldepthmax;

cvar_t	gl_ztrick = {"gl_ztrick","1"};

/*-----------------------------------------------------------------------*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}

static int XLateKey(XKeyEvent *ev)
{
	int key;
	char buf[64];
	KeySym keysym;
	int kp; // BorisU

	key = 0;
	kp = (int)cl_keypad.value;

	XLookupString(ev, buf, sizeof buf, &keysym, 0);


	switch(keysym) {
		case XK_Print:			key = K_PRINTSCR; break;

		case XK_Scroll_Lock:	key = K_SCRLCK; break;

		case XK_Caps_Lock:		key = K_CAPSLOCK; break;

		case XK_Num_Lock:		key = kp ? KP_NUMLOCK : K_PAUSE; break;

		case XK_KP_Page_Up:		key = kp ? KP_PGUP : K_PGUP; break;
		case XK_Page_Up:		key = K_PGUP; break;

		case XK_KP_Page_Down:	key = kp ? KP_PGDN : K_PGDN; break;
		case XK_Page_Down:		key = K_PGDN; break;

		case XK_KP_Home:		key = kp ? KP_HOME : K_HOME; break;
		case XK_Home:			key = K_HOME; break;

		case XK_KP_End:			key = kp ? KP_END : K_END; break;
		case XK_End:			key = K_END; break;

		case XK_KP_Left:		key = kp ? KP_LEFTARROW : K_LEFTARROW; break;
		case XK_Left:			key = K_LEFTARROW; break;

		case XK_KP_Right:		key = kp ? KP_RIGHTARROW : K_RIGHTARROW; break;
		case XK_Right:			key = K_RIGHTARROW; break;

		case XK_KP_Down:		key = kp ? KP_DOWNARROW : K_DOWNARROW; break;

		case XK_Down:			key = K_DOWNARROW; break;

		case XK_KP_Up:			key = kp ? KP_UPARROW : K_UPARROW; break;

		case XK_Up:				key = K_UPARROW; break;

		case XK_Escape:			key = K_ESCAPE; break;

		case XK_KP_Enter:		key = kp ? KP_ENTER : K_ENTER; break;

		case XK_Return:			key = K_ENTER; break;

		case XK_Tab:			key = K_TAB; break;

		case XK_F1:				key = K_F1; break;

		case XK_F2:				key = K_F2; break;

		case XK_F3:				key = K_F3; break;

		case XK_F4:				key = K_F4; break;

		case XK_F5:				key = K_F5; break;

		case XK_F6:				key = K_F6; break;

		case XK_F7:				key = K_F7; break;

		case XK_F8:				key = K_F8; break;

		case XK_F9:				key = K_F9; break;

		case XK_F10:			key = K_F10; break;

		case XK_F11:			key = K_F11; break;

		case XK_F12:			key = K_F12; break;

		case XK_BackSpace: key = K_BACKSPACE; break;

		case XK_KP_Delete:		key = kp ? KP_DEL : K_DEL; break;
		case XK_Delete:			key = K_DEL; break;

		case XK_Pause:			key = K_PAUSE; break;

		case XK_Shift_L:		key = K_LSHIFT; break;								
		case XK_Shift_R:		key = K_RSHIFT; break;

		case XK_Execute: 
		case XK_Control_L:		key = K_LCTRL; break;
		case XK_Control_R:		key = K_RCTRL; break;

		case XK_Alt_L:	
		case XK_Meta_L:			key = K_LALT; break;								
		case XK_Alt_R:	
		case XK_Meta_R:			key = K_RALT; break;

		case XK_Super_L:		key = K_LWIN; break;
		case XK_Super_R:		key = K_RWIN; break;
		case XK_Menu:			key = K_MENU; break;

		case XK_KP_Begin:		key = kp ? KP_5 : '5'; break;

		case XK_KP_Insert:		key = kp ? KP_INS : K_INS; break;
		case XK_Insert:			key = K_INS; break;

		case XK_KP_Multiply:	key = '*'; break;

		case XK_KP_Add:			key = kp ? KP_PLUS : '+'; break;

		case XK_KP_Subtract:	key = kp ? KP_MINUS : '-'; break;

		case XK_KP_Divide:		key = kp ? KP_SLASH : '/'; break;


		default:
			key = *(unsigned char*)buf;
			if (key >= 'A' && key <= 'Z')
				key = key - 'A' + 'a';
//			fprintf(stdout, "case 0x0%x: key = ___;break;/* [%c] */\n", keysym);
			break;
	} 

	return key;
}

static void install_grabs(void)
{
	XGrabPointer(dpy, win,
				 True,
				 0,
				 GrabModeAsync, GrabModeAsync,
				 win,
				 None,
				 CurrentTime);

#ifdef USE_DGA
	XF86DGADirectVideo(dpy, DefaultScreen(dpy), XF86DGADirectMouse);
	dgamouse = 1;
#else
	XWarpPointer(dpy, None, win,
				 0, 0, 0, 0,
				 vid.width / 2, vid.height / 2);
#endif

	XGrabKeyboard(dpy, win,
				  False,
				  GrabModeAsync, GrabModeAsync,
				  CurrentTime);

//	XSync(dpy, True);
}

static void uninstall_grabs(void)
{
#ifdef USE_DGA
	XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
	dgamouse = 0;
#endif

	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);

//	XSync(dpy, True);
}

qboolean OnChange_windowed_mouse(cvar_t *var, const char *value)
{
	if (vidmode_active && !Q_atof(value)) {
		Con_Printf("Cannot turn %s off when using -fullscreen mode\n", var->name);
		return true;
	}
	return false;
}

static void GetEvent(void)
{
	XEvent event;
	int b;

	if (!dpy)
		return;

	XNextEvent(dpy, &event);

	switch (event.type) {
	case KeyPress:
	case KeyRelease:
		Key_Event(XLateKey(&event.xkey), event.type == KeyPress);
		break;

	case MotionNotify:
#ifdef USE_DGA
		if (dgamouse && _windowed_mouse.value) {
			mouse_x += event.xmotion.x_root;
			mouse_y += event.xmotion.y_root;
		} else
#endif
		{
			if (_windowed_mouse.value) {
				mouse_x = (float) ((int)event.xmotion.x - (int)(vid.width/2));
				mouse_y = (float) ((int)event.xmotion.y - (int)(vid.height/2));

				/* move the mouse to the window center again */
				XSelectInput(dpy, win, X_MASK & ~PointerMotionMask);
				XWarpPointer(dpy, None, win, 0, 0, 0, 0, 
					(vid.width/2), (vid.height/2));
				XSelectInput(dpy, win, X_MASK);
			}
		}
		break;

	case ButtonPress:
		b=-1;
		if (event.xbutton.button == 1)
			b = 0;
		else if (event.xbutton.button == 2)
			b = 2;
		else if (event.xbutton.button == 3)
			b = 1;
		if (b>=0)
			Key_Event(K_MOUSE1 + b, true);
		break;

	case ButtonRelease:
		b=-1;
		if (event.xbutton.button == 1)
			b = 0;
		else if (event.xbutton.button == 2)
			b = 2;
		else if (event.xbutton.button == 3)
			b = 1;
		if (b>=0)
			Key_Event(K_MOUSE1 + b, false);
		break;
	}

	if (old_windowed_mouse != _windowed_mouse.value) {
		old_windowed_mouse = _windowed_mouse.value;

		if (!_windowed_mouse.value) {
			/* ungrab the pointer */
			uninstall_grabs();
		} else {
			/* grab the pointer */
			install_grabs();
		}
	}
}


void VID_Shutdown(void)
{
	uninstall_grabs();

	if (!ctx)
		return;

#ifdef USE_VMODE
	if (dpy) {
		RestoreHWGamma();
		glXDestroyContext(dpy, ctx);
		if (win)
			XDestroyWindow(dpy, win);
		if (vidmode_active)
			XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[0]);
		XCloseDisplay(dpy);
		vidmode_active = false;
	}

#else
	glXDestroyContext(dpy, ctx);
#endif
}

void signal_handler(int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	Sys_Quit();
	exit(0);
}

void InitSig(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}

void VID_ShiftPalette(unsigned char *p)
{
//	VID_SetPalette(p);
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = *y = 0;
	*width = scr_width;
	*height = scr_height;
}


void GL_EndRendering (void)
{
	glFlush();
	glXSwapBuffers(dpy, win);
}

static Cursor CreateNullCursor(Display *display, Window root)
{
	Pixmap cursormask; 
	XGCValues xgc;
	GC gc;
	XColor dummycolour;
	Cursor cursor;

	cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
	xgc.function = GXclear;
	gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
	XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
	dummycolour.pixel = 0;
	dummycolour.red = 0;
	dummycolour.flags = 04;
	cursor = XCreatePixmapCursor(display, cursormask, cursormask,
		&dummycolour,&dummycolour, 0,0);
	XFreePixmap(display,cursormask);
	XFreeGC(display,gc);
	return cursor;
}

void VID_Init(unsigned char *palette)
{
	int i;
	int attrib[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 1,
		None
	};
	int width = 640, height = 480;
	XSetWindowAttributes attr;
	unsigned long mask;
	Window root;
	XVisualInfo *visinfo;
	qboolean fullscreen = false;
	int MajorVersion, MinorVersion;
	int actualWidth, actualHeight;

	S_Init();

	Cvar_RegisterVariable (&vid_mode);
	Cvar_RegisterVariable (&gl_ztrick);
	Cvar_RegisterVariable (&_windowed_mouse);
	Cvar_RegisterVariable (&vid_hwgammacontrol);
	
	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

// interpret command-line params

// set vid parameters
// fullscreen
	if (COM_CheckParm("-window"))
		fullscreen = false;
	if (COM_CheckParm("-fullscreen"))
		fullscreen = true;

	if ((i = COM_CheckParm("-width")) != 0)
		width = atoi(com_argv[i+1]);
	if ((i = COM_CheckParm("-height")) != 0)
		height = atoi(com_argv[i+1]);

	if ((i = COM_CheckParm("-conwidth")) != 0)
		vid.conwidth = Q_atoi(com_argv[i+1]);
	else
		vid.conwidth = 640;

	vid.conwidth &= 0xfff8; // make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth*3 / 4;

	if ((i = COM_CheckParm("-conheight")) != 0)
		vid.conheight = Q_atoi(com_argv[i+1]);
	if (vid.conheight < 200)
		vid.conheight = 200;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Error couldn't open the X display\n");
		exit(1);
	}

	scrnum = DefaultScreen(dpy);
	root = RootWindow(dpy, scrnum);

#ifdef USE_VMODE
	MajorVersion = MinorVersion = 0;
	if (!XF86VidModeQueryVersion(dpy, &MajorVersion, &MinorVersion)) {
		vidmode_ext = false;
	} else {
		Con_Printf("Using XF86-VidModeExtension Ver. %d.%d\n", MajorVersion, MinorVersion);
		vidmode_ext = true;
	}
#endif

	visinfo = glXChooseVisual(dpy, scrnum, attrib);
	if (!visinfo) {
		fprintf(stderr, "qkHack: Error couldn't get an RGB, Double-buffered, Depth visual\n");
		exit(1);
	}

#ifdef USE_VMODE
	if (vidmode_ext) {
		int best_fit, best_dist, dist, x, y;

		XF86VidModeGetAllModeLines(dpy, scrnum, &num_vidmodes, &vidmodes);
		// Are we going fullscreen?  If so, let's change video mode
		if (fullscreen) {
			best_dist = 9999999;
			best_fit = -1;

			for (i = 0; i < num_vidmodes; i++) {
				if (width > vidmodes[i]->hdisplay ||
					height > vidmodes[i]->vdisplay)
					continue;

				x = width - vidmodes[i]->hdisplay;
				y = height - vidmodes[i]->vdisplay;
				dist = (x * x) + (y * y);
				if (dist < best_dist)
				{
					best_dist = dist;
					best_fit = i;
				}
			}

			if (best_fit != -1) {
				actualWidth = vidmodes[best_fit]->hdisplay;
				actualHeight = vidmodes[best_fit]->vdisplay;

				// change to the mode
				XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[best_fit]);
				vidmode_active = true;

				// Move the viewport to top left
				XF86VidModeSetViewPort(dpy, scrnum, 0, 0);

			} else
				fullscreen = 0;
		}
	}
#endif

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = X_MASK;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

#ifdef USE_VMODE
	// fullscreen
	if (vidmode_active)
	{
		mask = CWBackPixel | CWColormap | CWSaveUnder | CWEventMask | CWBackingStore | CWOverrideRedirect;
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	}
#endif

	win = XCreateWindow(dpy, root, 0, 0, width, height,
						0, visinfo->depth, InputOutput,
						visinfo->visual, mask, &attr);
	// kazik -->
	// inviso cursor
	XDefineCursor(dpy, win, CreateNullCursor(dpy, win));
	// kazik <--

	XMapWindow(dpy, win);

	{
		int x, y;
		x = y = 0;
		if ((i = COM_CheckParm("-position")) != 0) {
			x = atoi(com_argv[i+1]);
			y = atoi(com_argv[i+2]);
		}
		XMoveWindow(dpy, win, x, y);
	}

#ifdef USE_VMODE
	if (vidmode_active) {
		XRaiseWindow(dpy, win);
		XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		XFlush(dpy);
		// Move the viewport to top left
		XF86VidModeSetViewPort(dpy, scrnum, 0, 0);
	}
#endif

	XFlush(dpy);

	ctx = glXCreateContext(dpy, visinfo, NULL, True);

	glXMakeCurrent(dpy, win, ctx);

	scr_width = width;
	scr_height = height;

	if (vid.conheight > height)
		vid.conheight = height;
	if (vid.conwidth > width)
		vid.conwidth = width;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
	vid.numpages = 2;

	InitSig(); // trap evil signals

	GL_Init();

	Check_Gamma(palette);

	VID_SetPalette(palette);

	// Check for 3DFX Extensions and initialize them.
	VID_Init8bitPalette();

	InitHWGamma(); // BorisU

	Con_SafePrintf ("Video mode %dx%d initialized\n", width, height);

	vid.recalc_refdef = 1;				// force a surface cache flush
}

void Sys_SendKeyEvents(void)
{
	if (dpy) {
		while (XPending(dpy)) 
			GetEvent();
	}
}

void Force_CenterView_f (void)
{
	if (concussioned) return;
	cl.viewangles[PITCH] = 0;
}

void IN_Init(void)
{
	Cvar_RegisterVariable (&cl_keypad); // BorisU
}

void IN_Shutdown(void)
{
}

/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
}

/*
===========
IN_Move
===========
*/
void IN_MouseMove (usercmd_t *cmd)
{
	if (m_filter.value)
	{
		mouse_x = (mouse_x + old_mouse_x) * 0.5;
		mouse_y = (mouse_y + old_mouse_y) * 0.5;
	}
	old_mouse_x = mouse_x;
	old_mouse_y = mouse_y;

	mouse_x *= sensitivity.value;
	mouse_y *= sensitivity.value;

// add mouse X/Y movement to cmd
	if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
		cmd->sidemove += m_side.value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;
	
	if (in_mlook.state & 1)
		V_StopPitchDrift ();
		
	if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;
		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	}
	else
	{
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * mouse_y;
		else
			cmd->forwardmove -= m_forward.value * mouse_y;
	}
	mouse_x = mouse_y = 0.0;
}

void IN_Move (usercmd_t *cmd)
{
	IN_MouseMove(cmd);
}


void VID_UnlockBuffer() {}
void VID_LockBuffer() {}


qboolean V_OnHWgammaChange (cvar_t *var, const char *value)
{
	float newhwgamma = Q_atof(value);
	
	if (newhwgamma == vid_hwgammacontrol.value) 
		return true;
	
	if (newhwgamma == 1 && vid_hwgammaproblems)
		return true; 

	if (newhwgamma != 1 && mtfl)
		return true; 

	return false;
}

static unsigned short systemgammaramp[3][256];
static qboolean customgamma = false;

/*
======================
VID_SetDeviceGammaRamp

Note: ramps must point to a static array
======================
*/

void VID_SetDeviceGammaRamp (unsigned short ramps[3][256])
{
#ifdef USE_HWGAMMA
	if (vid_gammaworks) {
		currentgammaramp = ramps;
		if (vid_hwgamma_enabled) {
			XF86VidModeSetGammaRamp(dpy,scrnum,256,ramps[0],ramps[1],ramps[2]);
			customgamma = true;
		}
	}
#endif
}

void InitHWGamma (void)
{
#ifdef USE_HWGAMMA
	int xf86vm_gammaramp_size;

	XF86VidModeGetGammaRampSize(dpy,scrnum,&xf86vm_gammaramp_size);
	
	vid_gammaworks = (xf86vm_gammaramp_size == 256);

	if (vid_gammaworks){
		Con_Printf("Hardware GammaRamp support found\n");
		XF86VidModeGetGammaRamp(dpy,scrnum,xf86vm_gammaramp_size,
			systemgammaramp[0],systemgammaramp[1],systemgammaramp[2]);
	} else {
		if (mtfl)
			Sys_Error("Can't start without hardware GammaRamp support");
		Con_Printf("Hardware GammaRamp support not found\n");
		Cvar_SetValue(&vid_hwgammacontrol,0);
		vid_hwgammacontrol.flags |= CVAR_ROM;
	}
	vid_hwgamma_enabled = vid_hwgammacontrol.value && vid_gammaworks; // && fullscreen?
#endif
}

void RestoreHWGamma (void)
{
#ifdef USE_HWGAMMA
//	XF86VidModeSetGamma(dpy, scrnum, &oldgamma);
	if (vid_gammaworks && customgamma) {
		customgamma = false;
		VID_SetDeviceGammaRamp (systemgammaramp);
	}
#endif
}
