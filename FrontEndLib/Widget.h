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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef WIDGET_H
#define WIDGET_H

#ifdef WIN32
#pragma warning(disable:4786)
#endif

//SUMMARY
//
//CWidget is an abstract base class from which all widgets derive.  A widget is
//a graphical area which paints by certain rules and can optionally handle events.
//Widgets may contain other widgets, and many operations take advantage of this
//hierarchical grouping.  For example, moving one widget will move all of its
//children at the same time.
//
//Several drawing utility functions are implemented in CWidget.  It is convenient
//to put them here because nearly all drawing in this app must be done in the
//context of a widget.
//
//USAGE
//
//You can derive new classes from CWidget.  Typically, you would want widgets to
//appear on a screen, and this can be accomplished by constructing a new
//CWidget-derived class and calling the AddWidget() method of the CScreen-derived
//class.  You can add a widget to any other widget, not just a CScreen-derived class.
//Painting, loading, unloading, moving, hiding, and disabling of a parent widget
//will affect all of its children.
//
//The sections that follow describe implementing a new CWidget-derived class.
//
//FOCUSABLE WIDGETS
//
//If you want your widget to be capable of receiving the focus, you should derive
//it from CFocusWidget instead of CWidget.  When a widget is derived from CFocusWidget,
//its IsFocusable() method will return true instead of false.
//
//After focusable widgets have been added to a parent, they may be explicitly removed
//and added from the focus list with calls to CEventHandlerWidget::AddFocusWidget()
//and RemoveFocusWidget().
//
//ANIMATED WIDGETS
//
//If you want your widget to be automatically animated between events, then you
//should override CWidget::IsAnimated() to return true and CWidget::HandleAnimate()
//to perform the animation.
//
//If you call AddWidget() to add a widget to a parent without an event-handler,
//the widget will not be automatically animated.  For example, adding a CScalerWidget
//to a CFrameWidget before the CFrameWidget is added to a CScreen, will result in the
//CScalerWidget not being animated.  However, if the CScalerWidget was added to the
//CFrameWidget AFTER CFrameWidget was added to the CScreen, the CScalerWidget
//would animate.
//
//After animated widgets have been added to a parent, they may be explicitly removed
//and added from the animation list with calls to CEventHandlerWidget::AddAnimatedWidget()
//and RemoveAnimatedWidget().
//
//PAINTING
//
//You must implement the pure Paint() method in your CWidget-derived class.  When
//you want to draw something, call GetDestSurface() for the surface.  It goes against
//the widget design to draw directly to the screen, which may or may not be the
//destination surface (See CWidget::SetDestSurface()).
//
//If you need to directly write to the pixel data, enclose your writes in a
//LockDestSurface()/UnlockDestSurface() pair.  Also, you must choose whether to make
//your routine safe for clipping or not.  Clipping is supported in CWidget methods,
//but in no derived classes, that write directly to the pixel data,
//except by accident (debug errors).  You can override CWidget::PaintClipped() and
//put an "ASSERT(!"Can't paint clipped.")" statement in it.
//This will designate your widget as unsafe for clipping and keep it out of trouble.
//If you are not directly writing to the pixel data, your widget is safe for clipping.
//
//NOTE: If a child widget is to be clipped to a specified parent region, then
//child widgets should never call, or have their Paint() method called directly.
//Instead, call RequestPaint().  This will ensure that clipping is performed.
//
//The last two lines of your CWidget::Paint() should probably be:
//
//  PaintChildren();
//  if (bUpdateRect) UpdateRect();
//
//The first line is not strictly necessary if your widget is not meant to contain other
//widgets, but adding the call adds just a tiny bit of overhead.  It makes your
//widget paint any contained widgets.
//
//The second is used by the painting machinery to handle all surface updates, and needs
//to be present.
//
//LOADING AND UNLOADING
//
//You only need to override Load() if your widget requires resource loading or has some
//failable initialization.  If you override Load(), follow this sequence:
//
//  Perform loading tasks for your widget.
//  If any failures occurred then return false.
//  "return LoadChildren();"
//
//If you override Load(), you should also override Unload() to provide a symmetrical
//unloading.  It should follow this sequence:
//
//  "UnloadChildren();"
//  Perform unloading tasks for your widget.
//
//Note that a widget may be constructed, and then loaded and unloaded MULTIPLE times
//before it is destructed.  This is under the control of the screen manager, which
//will leave some screens (and their children) in memory, and unload others to reduce
//memory consumption.  You want to retain state information after an Unload(), but not
//resources.  For example, CTextBoxWidget will release the bitmap is uses to draw
//itself in Unload(), but will not release its text data until the destructor is called.
//
//HANDLING EVENTS
//
//If your widget is a child of an event-handling widget (CScreen and CDialog-derived
//classes), it will receive events from an active event-handling widget.  The
//CWidget::Handle*() methods will be called with information about the event.  You
//override the Handle*() methods that your widget should respond to.  See descriptions
//of the handlers below.
//
//Handlers are responsible for repainting the widget when it changes.  If your
//handler is going to call an event notifier, such as OnSelectChange(), paint your
//widget first.  This generally makes your widget appear to respond more quickly
//to user interaction.
//
//ACCEPTING TEXT ENTRY
//
//If your widget is meant to record a text entry in response to keypresses, then
//you should override CWidget::AcceptsTextEntry() to return true.  If your widget
//merely responds to keypresses, but doesn't convert them to text directly, then
//you shouldn't override AcceptsTextEntry().

#include "Colors.h"
#include <BackEndLib/Types.h>
#include <BackEndLib/AttachableObject.h>
#include <BackEndLib/Wchar.h>

#include <list>
#include <string>
#include <vector>
using std::list;
using std::vector;

//Tag constants.
static const UINT TAG_UNSPECIFIED = 0L;
static const UINT TAG_QUIT = (UINT)-1L;
static const UINT TAG_ESCAPE = (UINT)-2L;
static const UINT TAG_SCREENSIZE = (UINT)-3L;

//OK button has same tag value everywhere.
static const UINT TAG_OK = 9000L;

static const UINT TAG_FIRST_RESERVED = (UINT)-3L;
#define IS_RESERVED_TAG(t) ((UINT)(t) >= TAG_FIRST_RESERVED)

//Rect-related macros.
#define COPY_RECT(sr,dr) memcpy(&(dr), &(sr), sizeof(SDL_Rect))
#define CLEAR_RECT(r) memset(&(r), 0, sizeof(SDL_Rect))
#define IS_IN_RECT(x_,y_,r) ((x_) >= (r).x && (x_) < (r).x + (r).w && \
		(y_) >= (r).y && (y_) < (r).y + (r).h)
#define SET_RECT(r, x_, y_, w_, h_) (r).x = (x_); (r).y = (y_); (r).w = (w_); (r).h = (h_)
#define ARE_RECTS_EQUAL(r1, r2) ( (r1).x == (r2).x && (r1).y == (r2).y && \
		(r1).w == (r2).w && (r1).h == (r2).h )

//UI colors.
#define RGB_FOCUS    138, 126, 103
#define RGB_PLACEHOLDER 163, 143, 137

//Widget types.
//Add new enums as new CWidget-derived classes are created.
enum WIDGETTYPE
{
	WT_Unspecified = -1,
	WT_Screen = 0,
	WT_Button,
	WT_Dialog,
	WT_Label,
	WT_Frame,
	WT_OptionButton,
	WT_Slider,
	WT_ListBox,
	WT_TextBox,
	WT_ScrollingText,
	WT_Scaler,
	WT_TabbedMenu,
	WT_ObjectMenu,
	WT_ProgressBar,
	WT_HTML,
	WT_Scrollable,
	WT_HyperLink,
	WT_Image,
	WT_Marquee,
	WT_Menu,
	WT_Room,
	WT_Tiles,
	WT_Stack,

	WT_Count
};

//Hotkey information.
struct HOTKEY {
	SDL_Keycode key;
	UINT tag;
};

//*******************************************************************************
class CEffectList;
class CEventHandlerWidget;
class CScalerWidget;
class CScreenManager;
class CWidget : public CAttachableObject
{
public:
	CWidget* GetParent() const { return this->pParent; }

protected:
	friend class CEffectList;
	friend class CEventHandlerWidget;
	friend class CScalerWidget;
	friend class CScreenManager;
	friend class CMarqueePart; //needs to be able to delete widgets

	CWidget(WIDGETTYPE eSetType, UINT dwSetTagNo, int nSetX, int nSetY, UINT wSetW, UINT wSetH);
	virtual ~CWidget();

	//Called whenever widget desires to resize itself.
	bool        CanResize() const;
	//Called whenever a child widget's Resize() method is called.
	virtual void   ChildResized() { }
	bool        bResizing;  //to avoid recursive calls to Resize()

	void        ClearChildren();
	void        EraseChildren(SDL_Surface* pBackground, SDL_Rect rect, const bool bUpdate=false);
	CEventHandlerWidget * GetEventHandlerWidget();
	CWidget *      GetFirstSibling();
	CWidget *      GetLastSibling();
	CWidget *      GetNextSibling();
	CWidget *      GetPrevSibling();
	virtual UINT GetHotkeyTag(const SDL_Keysym& keysym);
	UINT         GetHotkeyTagInSelf(const SDL_Keysym& keysym);
	UINT         GetHotkeyTagInChildren(const SDL_Keysym& keysym);
	void        GetScrollOffset(int &nOffsetX, int &nOffsetY) const;
	virtual bool   IsAnimated() const {return false;}
	virtual bool   IsDoubleClickable() const {return true;}
	Uint32         IsLocked() const;
	virtual bool   IsFocusable() const {return false;}
	virtual bool   IsHoverable() const {return false;}
	bool        IsSelectable(const bool bSelectIfParentIsHidden=false) const;
	bool        IsScrollOffset() const;
	virtual bool         Load();
	bool        LoadChildren();
	void        PaintChildren(bool bUpdateRects = false);
	void        RemoveAllHotkeys();
	void        RemoveHotkey(const UINT tag);
	void        SetParent(CWidget *const pSetParent);
	virtual void         Unload();
	void        UnloadChildren();

	//
	//Event handlers.
	//

	//Called between events for an animated widget (CWidget::IsAnimated() was overrode to
	//return true).
	virtual void   HandleAnimate() { }

	//Called when the widget previously received a mouse down within its area,
	//a mouse up has not yet occurred, and mouse motion is occurring.  The mouse pointer
	//may be inside or outside of the widget area.
	virtual void   HandleDrag(const SDL_MouseMotionEvent &/*Motion*/) { }

	//Called when the widget is receiving a mouse down within its area.
	virtual  void  HandleMouseDown(const SDL_MouseButtonEvent &/*Button*/) { }

	//Called when the widget previously received a mouse down within its area,
	//a mouse up has not yet occurred, the mouse pointer is inside the widget area,
	//and a certain interval has passed.
	virtual  void  HandleMouseDownRepeat(const SDL_MouseButtonEvent &/*Button*/) { }

	//Called when the widget previously received a mouse down within its area,
	//and a mouse up is occurring.  The mouse pointer may or may not be within
	//the widget area.
	virtual  void  HandleMouseUp(const SDL_MouseButtonEvent &/*Button*/) { }

	//Called when the widget is receiving a mouse wheel scroll.
	virtual  void  HandleMouseWheel(const SDL_MouseWheelEvent &/*Wheel*/) { }

	//Called when the mouse pointer is moving inside the widget area.
	virtual  void  HandleMouseMotion(const SDL_MouseMotionEvent &/*Motion*/) { }

	//Called right after the mouse pointer has left the widget area of a
	//hoverable widget.
	virtual  void  HandleMouseOut() { }

	//Called when the widget has the focus and a key is pressed.  Should return
	//true if this widget is generally affected by key down events, or false if
	//not.
	virtual void   HandleKeyDown(const SDL_KeyboardEvent &/*Key*/) { }

	//Called when the widget has the focus and a key is released.
	virtual void   HandleKeyUp(const SDL_KeyboardEvent &/*Key*/) { }

	//Called when the widget has focus and text has been input.
	virtual void   HandleTextInput(const SDL_TextInputEvent &/*text*/) { }

	bool        bIsLoaded;
	bool        bIsEnabled;
	int            x, y;
	UINT        w, h;
	int            nChildrenScrollOffsetX, nChildrenScrollOffsetY;
	CWidget *   pParent;

	bool           bIsVisible;
	list<CWidget *>      Children;

	//Data structures:
	vector<std::string> imageFilenames;
	vector<SDL_Surface*> images;

private:
	UINT       dwTagNo;
	WIDGETTYPE  eType;
	vector<HOTKEY> hotkeys;   //hotkey assignments
	SDL_Surface *  pSurface;

public:
	virtual bool   AcceptsTextEntry() {return false;}
	void        AddHotkey(const SDL_Keycode key, const UINT tag);
	virtual CWidget *    AddWidget(CWidget *pNewWidget, bool bLoad = false);
	virtual void   Center();
	virtual bool ContainsCoords(const int nX, const int nY) const;
	static bool ClipRectToRect(SDL_Rect& rect, const SDL_Rect& clipRect);
	void        ClipWHToDest();
	virtual void   Disable() {bIsEnabled = false;}
	void        DrawPlaceholder();
	void        Enable(const bool bVal=true) {this->bIsEnabled = bVal;}
	virtual void   GetChildClippingRect(SDL_Rect &ChildClipRect) const;
	CEventHandlerWidget * GetParentEventHandlerWidget() const;
	void        GetRect(SDL_Rect &rect) const
			{rect.x = this->x; rect.y = this->y; rect.w = this->w; rect.h = this->h;}
	void        GetRectContainingChildren(SDL_Rect &ChildContainerRect) const;
	UINT        GetTagNo() const {return this->dwTagNo;}
	WIDGETTYPE  GetType() const {return this->eType;}
	CWidget *   GetWidget(const UINT dwTagNo, const bool bFindVisibleOnly=false);
	virtual CWidget * GetWidgetContainingCoords(const int nX, const int nY,
			const WIDGETTYPE eWidgetType) const;
	int         GetX() const {return this->x;}
	int         GetY() const {return this->y;}
	UINT        GetW() const {return this->w;}
	UINT        GetH() const {return this->h;}

	virtual void   Hide(const bool bPaint = true);
	void        HideChildren();
	bool        IsActive() const {return IsEnabled() && IsVisible();}
	bool        IsEnabled() const {return bIsEnabled;}
	bool        IsInsideOfParent() const;
	bool        IsInsideOfParents() const;
	bool        IsInsideOfRect(const int nX, const int nY, const UINT wW,
			const UINT wH) const;
	bool        IsLoaded() const {return this->bIsLoaded;}
	bool        IsVisible(const bool bInParent = false) const;
	virtual void   Move(const int nSetX, const int nSetY);
	void        MoveChildren(const int dx, const int dy);
	bool        OverlapsRect(const int nX, const int nY, const UINT wW,
			const UINT wH) const;
	virtual void   Paint(bool bUpdateRect = true)=0;
	virtual void   PaintClipped(const int nX, const int nY, const UINT wW, const UINT wH,
			const bool bUpdateRect = true);
	void        PaintClippedInsideParent(bool bUpdateRect = true);
	virtual bool   ParentMustPaintChildren() const {return false;}
	virtual void   RemoveWidget(CWidget *pRemoveWidget);
	void        RequestPaint(const bool bUpdateRect = true);
	virtual void   Resize(const UINT wSetW, const UINT wSetH);
	void        Scroll(const int dx, const int dy);
	void        ScrollAbsolute(const int nScrollX, const int nScrollY);
	void        SetHeight(const UINT wHeight);
	void        SetWidth(const UINT wWidth);
	void        Show(const bool bFlag = true);
	void        ShowChildren();
	static WCHAR TranslateUnicodeKeysym(const SDL_Keysym& keysym, const bool bConsiderCaps=true);
	static void  TranslateUnicodeKeysym(WCHAR& wc, const SDL_Keycode sym, const bool bCaps);

	template<typename TWidget>
	TWidget *GetWidget(const UINT dwTagNo, const bool bFindVisibleOnly = false) {
		return dynamic_cast<TWidget *>(GetWidget(dwTagNo, bFindVisibleOnly));
		//return DYN_CAST(TWidget *, CWidget *, GetWidget(dwFindTagNo, bFindVisibleOnly));
	}

	//SDL helper functions that act on screen surface.
	void        DrawCol(int nX, int nY, UINT wH,
			const SURFACECOLOR &Color, SDL_Surface *pDestSurface = NULL);
	void        DrawFilledRect(SDL_Rect &rect, const SURFACECOLOR &Color,
			SDL_Surface *pDestSurface=NULL);
	void        DrawPixel(const int nX, const int nY,
			const SURFACECOLOR &Color, SDL_Surface *pDestSurface = NULL);
	void        DrawRect(const SDL_Rect &rect, const SURFACECOLOR &Color,
			SDL_Surface *pDestSurface=NULL);
	void        DrawRow(int nX, int nY, UINT wW,
			const SURFACECOLOR &Color, SDL_Surface *pDestSurface = NULL);
	static bool GetBlitRectFromClipRect(const int nPixelX, const int nPixelY,
			const SDL_Rect& ClipRect, SDL_Rect& BlitRect);
	void        GetDestSurfaceColor(Uint8 bytRed, Uint8 bytGreen,
			Uint8 bytBlue, SURFACECOLOR &Color) const;
	SDL_Surface *  GetDestSurface() const;
	SDL_Surface *  LockDestSurface(SDL_Surface *pDestSurface=NULL);
	void        SetChildrenDestSurface(SDL_Surface *pSurface);
	void        SetDestSurface(SDL_Surface *pSurface);
	void        UnlockDestSurface(SDL_Surface *pDestSurface=NULL);
	virtual void   UpdateRect() const;
	void  UpdateRect(SDL_Rect &rect) const;

	//Updates screen region so that any surface changes are displayed.
	inline void UpdateRect(int nX, int nY, UINT wW, UINT wH) const
		{ SDL_Rect rect = MAKE_SDL_RECT(nX, nY, wW, wH); UpdateRect(rect); }

private:
	void        DrawRectClipped(const SDL_Rect &rect, const SURFACECOLOR &Color,
			SDL_Surface *pDestSurface);
};

typedef std::list<CWidget *>::const_iterator WIDGET_ITERATOR;

void SetWidgetScreenSurface(SDL_Surface *pSetScreenSurface);
SDL_Surface * GetWidgetScreenSurface();

#endif //...#ifndef WIDGET_H
