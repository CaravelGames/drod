// $Id$

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Deadly Rooms of Death.
 *
 * The Initial Developer of the Original Code is
 * Caravel Software.
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996,
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Gerry Jo Jellestad (trick)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef DYN_H

//***************************************************************************
//This file generates the Dyn class in 3 recursive passes. Several additional
//passes are done when included from Dyn.cpp, to generate the code as well.
//
//Adding new functions for dynamic loading is very easy. Simply do this:
//
// * Go to the function list below (find *** FUNCTION LIST ***)
//
// * Add a new BEGINGROUP and ENDGROUP block. BEGINGROUP has 2+ arguments:
//      1st: Unique group identifier (<id>)
//      2nd: First library to try loading
//  (3rd..): Other libraries to try loading if first fails (optional)
//   Note: No trailing semicolon!
//
// * Add DYNFN macros within the group block. DYNFN has 4 arguments:
//      1st: Return type of the function
//      2nd: Name of the function (as defined in the external library)
//      3rd: Arguments of the function (within parenthesis)
//      4th: Function stub, in case it couldn't be loaded from ext.lib
//           Put the stub within curly braces, and protect any commas
//   Note: No trailing semicolon!
//
// * Make sure Dyn::LoadAll is called for all systems that use this, OR
//   call Dyn::Load<id> for each library you want to load.
//
// * Now you can call stuff with Dyn::function().  If something is
//   unavailable the stub function will be called in stead.
//
// * Call Dyn::UnloadAll or Dyn::Unload<id> when you're done.
//
//***************************************************************************

#ifndef DYN_H_PASS_1
#define DYN_H_PASS_1

//Start pass 1: Typedefs
#if defined __linux__ || defined __FreeBSD__
#	ifndef DONT_USE_GTK
#		define USE_GTK
#	endif
#	include <X11/Xlib.h>
#	if defined(USE_GTK)
//#		include <gtk/gtk.h>
#		include <cstdarg>
		typedef char gchar;
		typedef int gboolean;
		typedef unsigned long gulong;
		typedef void* gpointer;
		typedef struct _GObject GObject;
		typedef struct _GClosure GClosure;
		typedef union _GdkEvent GdkEvent;
		typedef struct _GtkWidget GtkWidget;
		typedef struct _GtkWindow GtkWindow;
		typedef struct _GtkDialog GtkDialog;
		enum GtkDialogFlags {
			GTK_DIALOG_MODAL = 1, GTK_DIALOG_DESTROY_WITH_PARENT = 2,
			GTK_DIALOG_NO_SEPARATOR = 4
		};
		enum GtkMessageType {
			GTK_MESSAGE_INFO, GTK_MESSAGE_WARNING, GTK_MESSAGE_QUESTION,
			GTK_MESSAGE_ERROR, GTK_MESSAGE_OTHER
		};
		enum GtkButtonsType {
			GTK_BUTTONS_NONE, GTK_BUTTONS_OK, GTK_BUTTONS_CLOSE,
			GTK_BUTTONS_CANCEL, GTK_BUTTONS_YES_NO, GTK_BUTTONS_OK_CANCEL
		};
		enum GConnectFlags {
			G_CONNECT_AFTER = 1, G_CONNECT_SWAPPED = 2
		};
		typedef void (*GCallback) (void);
		typedef void (*GClosureNotify) (gpointer, GClosure*);
#       define G_CALLBACK(x) ((GCallback)(x))
#		define DYN_G_OBJECT(x) ((GObject*)x)
#		define DYN_GTK_WIDGET(x) ((GtkWidget*)x)
#		define DYN_GTK_WINDOW(x) ((GtkWindow*)x)
#		define DYN_GTK_DIALOG(x) ((GtkDialog*)x)
#	endif
#endif
#include <stdio.h>
#define BEGINGROUP(x,...)
#define ENDGROUP
#define DYNFN(re,fn,arg,stub) typedef re (*T_##fn) arg;
#elif !defined DYN_H_PASS_2
#define DYN_H_PASS_2

//Start pass 2: Stub prototypes
#define BEGINGROUP(x,...)
#define ENDGROUP
#define DYNFN(re,fn,arg,stub) static re fn arg;
#elif !defined DYN_H_PASS_3
#define DYN_H_PASS_3

//Start pass 3: Function pointers
#define BEGINGROUP(x,...) \
	protected: static void *handle##x; public: \
	static bool Load##x(); \
	static void Unload##x();
#define ENDGROUP
#define DYNFN(re,fn,arg,stub) static T_##fn fn;
#elif !defined DYN_H_PASS_A
#define DYN_H_PASS_A

//Start pass A: Init function pointers with stubs
#if defined __linux__ || defined __FreeBSD__
#	include <dlfcn.h>
#	define LOADSO(x)    dlopen(x, RTLD_LAZY)
#	define LOADSYM(h,x) dlsym(h, x)
#	define UNLOADSO(h)  dlclose(h)
#else
#	include "SDL_loadso.h"
#	define LOADSO(x)    SDL_LoadObject(x)
#	define LOADSYM(h,x) SDL_LoadFunction(h, x)
#	define UNLOADSO(h)  SDL_UnloadObject(h)
#endif

#define BEGINGROUP(x,...) void* Dyn::handle##x = NULL; static const char *_libs_##x[] = { __VA_ARGS__, 0 };
#define ENDGROUP
#define DYNFN(re,fn,arg,stub) T_##fn Dyn::fn = &Dyn::Stub::fn;
#elif !defined DYN_H_PASS_B
#define DYN_H_PASS_B

//Start pass B: Generate load functions
#define BEGINGROUP(x,...) \
	bool Dyn::Load##x () { \
		if (!Dyn::handle##x) { void *handle = Dyn::handle##x = _loadSo(_libs_##x); \
		if (!handle) { Dyn::NoLib(#x); return false; }
#define ENDGROUP } return true; }
#define DYNFN(re,fn,arg,stub) { T_##fn tmp = (T_##fn)LOADSYM(handle, #fn); \
		if (tmp) Dyn::fn = tmp; else { Dyn::NoSym(#fn); Dyn::fn = &Dyn::Stub::fn; } }
#elif !defined DYN_H_PASS_C
#define DYN_H_PASS_C

//Start pass C: Generate unload functions
#define BEGINGROUP(x,...) \
	void Dyn::Unload##x () { \
		if (Dyn::handle##x) { \
			void *handle = Dyn::handle##x; \
			Dyn::handle##x = NULL;
#define ENDGROUP UNLOADSO(handle); } }
#define DYNFN(re,fn,arg,stub) Dyn::fn = &Dyn::Stub::fn;
#elif !defined DYN_H_PASS_D
#define DYN_H_PASS_D

//Start pass D: Generate LoadAll content
#define BEGINGROUP(x,...) st = Dyn::Load##x() && st;
#define ENDGROUP
#define DYNFN(re,fn,arg,stub)
#elif !defined DYN_H_PASS_E
#define DYN_H_PASS_E

//Start pass E: Generate UnloadAll content
#define BEGINGROUP(x,...) Dyn::Unload##x();
#define ENDGROUP
#define DYNFN(re,fn,arg,stub)
#elif !defined DYN_H_PASS_F
#define DYN_H_PASS_F

//Start pass F: Generate stubs
#define BEGINGROUP(x,...)
#define ENDGROUP
#define DYNFN(re,fn,arg,stub) re Dyn::Stub::fn arg stub
#endif

//***************************************************************************
//*** FUNCTION LIST ***

#if defined __linux__ || defined __FreeBSD__
//--- Xlib functions ---
BEGINGROUP(X11, "libX11.so.6")

DYNFN(Display*, XOpenDisplay, (char*), { return NULL; })
DYNFN(int,      XCloseDisplay, (Display*), { return Success; })
DYNFN(int,      XRaiseWindow, (Display*, Window), { return BadWindow; })

DYNFN(int,    XChangeProperty,      (Display*, Window, Atom, Atom, int, int,
	unsigned char *, int),
	{ return BadImplementation; })
DYNFN(int,    XConvertSelection,    (Display*, Atom, Atom, Atom, Window, Time),
	{ return BadImplementation; })
DYNFN(int,    XFree,                (void*),
	{ return 0; })
DYNFN(Window, XGetSelectionOwner,   (Display*, Atom),
	{ return None; })

DYNFN(Status, XGetWindowAttributes, (Display*, Window, XWindowAttributes *xwa),
{
	*xwa = ((XWindowAttributes){ 0, 0, 0, 0, 0, 0, NULL, None,
		InputOutput, ForgetGravity, UnmapGravity, NotUseful, 0, 0,
		False, None, False, IsUnviewable, 0, 0, 0, False, NULL });
	return BadImplementation;
})

DYNFN(int,    XGetWindowProperty,   (Display*, Window, Atom, long,
	long, Bool, Atom, Atom *actual_type, int *actual_format,
	unsigned long *nitems, unsigned long *bytes_after,
	unsigned char **prop),
{
	*actual_type = None;
	*actual_format = 0;
	*bytes_after = *nitems = 0;
	*prop = NULL;
	return BadImplementation;
})

DYNFN(Atom,   XInternAtom,          (Display*, const char*, Bool),
	{ return None; })
DYNFN(int,    XMoveWindow,          (Display*, Window, int, int),
	{ return 0; })
DYNFN(Status, XSendEvent,           (Display*, Window, Bool, long, XEvent*),
	{ return 0; })
DYNFN(int,    XSetSelectionOwner,   (Display*, Atom, Window, Time),
	{ return BadImplementation; })
DYNFN(int,    XSync,                (Display*, Bool),
	{ return BadImplementation; })

ENDGROUP

#ifdef USE_GTK
//--- GTK functions ---
BEGINGROUP(GTK, "libgtk-x11-2.0.so.0", "libgtk-3.so.0")

DYNFN(gboolean, gtk_init_check, (int *argc, char ***argv), { return 0; })
DYNFN(void,     gtk_main, (void), {})
DYNFN(void,     gtk_main_quit, (void), {})

DYNFN(GtkWidget*, gtk_message_dialog_new, (GtkWindow *parent,
	GtkDialogFlags flags, GtkMessageType type, GtkButtonsType buttons,
	const gchar *fmt, ...),
{
	va_list a;
	fprintf(stderr, (type == GTK_MESSAGE_ERROR) ? "Error: " : "Warning: ");
	va_start(a, fmt);
	vfprintf(stderr, fmt, a);
	va_end(a);
	fprintf(stderr, "\n");
	return NULL;
})

DYNFN(void, gtk_window_set_title, (GtkWindow *window, const gchar *title), {})
DYNFN(void, gtk_widget_show, (GtkWidget *widget), {})
DYNFN(void, gtk_widget_destroy, (GtkWidget *widget), {})

DYNFN(gulong, g_signal_connect_data, (gpointer instance,
	const gchar *detailed_signal, GCallback c_handler, gpointer data,
	GClosureNotify destroy_data, GConnectFlags connect_flags),
	{ return 0; });

ENDGROUP
#endif //USE_GTK

#endif //Linux/FreeBSD

//***************************************************************************

#undef BEGINGROUP
#undef ENDGROUP
#undef DYNFN

//***************************************************************************
#ifndef DYN_H_PASS
#define DYN_H_PASS
//End pass 1: Typedefs
class Dyn
{
private:
	struct Stub {
#include "Dyn.h" //Pass 2: Stub prototypes
	};

	static void NoLib(const char *e) { fprintf(stderr, "Couldn't open library %s\n", e); }
	static void NoSym(const char *e) { fprintf(stderr, "Couldn't find symbol %s\n", e); }

public:
	static bool LoadAll();
	static void UnloadAll();
#include "Dyn.h" //Pass 3: Function pointers
};

#ifdef DYN_INCLUDED_FROM_DYN_CPP
static void * _loadSo (const char **libs);
#include "Dyn.h" //Pass A: Init function pointers
static void * _loadSo (const char **libs)
{
	for (; *libs; ++libs)
	{
		void *handle = LOADSO(*libs);
		if (handle)
			return handle;
	}
	return 0;
}
#include "Dyn.h" //Pass B: Generate load functions
#include "Dyn.h" //Pass C: Generate unload functions
bool Dyn::LoadAll () {
	bool st = true;
#include "Dyn.h" //Pass D: Generate LoadAll content
	return st;
}
void Dyn::UnloadAll () {
#include "Dyn.h" //Pass E: Generate UnloadAll content
}
#include "Dyn.h" //Pass F: Generate stubs
#endif

#define DYN_H
#endif

//***************************************************************************

#endif //DYN_H
//***************************************************************************
