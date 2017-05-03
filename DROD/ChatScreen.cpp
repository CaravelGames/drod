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
 * Portions created by the Initial Developer are Copyright (C) 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributors:
 *
 * ***** END LICENSE BLOCK ***** */

#include "ChatScreen.h"
#include "BrowserScreen.h"
#include "DrodFontManager.h"
#include "GameScreen.h"
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/ImageWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/TextBox2DWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/DbXML.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Clipboard.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/Wchar.h>

#define CONNECTION_STATUS_KEY (UINT(0))

const UINT TAG_LIST_BOX = 1100;
const UINT TAG_USERS_BOX = 1101;
const UINT TAG_INPUT = 1102;
const UINT TAG_SEND = 1103;
const UINT TAG_DELETE = 1104;
const UINT TAG_IGNORE = 1105;
const UINT TAG_HELP = 1106;
const UINT TAG_CANCEL = 1107;

//
//Public methods.
//

//*****************************************************************************
CChatScreen::CChatScreen()
	: CDrodScreen(SCR_Chat)
	, pChatListBox(NULL), pUsersListBox(NULL)
	, pTextWidget(NULL)
	, pSendButton(NULL), pDeleteButton(NULL), pIgnoreButton(NULL)
//Constructor
{
	this->imageFilenames.push_back(string("Background"));

	static const UINT CY_SPACE = 15;
	static const UINT CX_SPACE = 15;

	static const UINT CX_BUTTON = 100;
	static const UINT CY_BUTTON = CY_STANDARD_BUTTON;
	static const UINT CY_BUTTON_SPACE = 10;

	static const UINT CY_TITLE_SPACE = 14;
	static const UINT CX_TITLE = 300;
	static const UINT CY_TITLE = 52;
	const int X_TITLE = (this->w - CX_TITLE) / 2;
	static const int Y_TITLE = CY_TITLE_SPACE;

	//Chat window.
	static const int X_CHATLIST = CX_SPACE;
	static const int Y_CHATLIST = Y_TITLE + CY_TITLE + CY_SPACE;
	const UINT CX_CHATLIST = (this->w *3/4) - X_CHATLIST;
	static const UINT CY_CHATLIST = 23*CY_LBOX_ITEM + 4;   //23 items

	//Users window.
	static const int Y_USERLISTHEADER = Y_CHATLIST;
	static const UINT CY_USERLISTHEADER = 27;
	static const int X_USERLIST = X_CHATLIST + CX_CHATLIST + CX_SPACE;
	static const int Y_USERLIST = Y_USERLISTHEADER + CY_USERLISTHEADER;
	const UINT CX_USERLIST = this->w - X_USERLIST - X_CHATLIST;
	static const UINT CY_USERLIST = CY_CHATLIST - CY_USERLISTHEADER;

	//Text input.
	static const int X_TEXT = X_CHATLIST;
	static const int Y_TEXT = Y_CHATLIST + CY_CHATLIST + CY_SPACE;
	const UINT CX_TEXT = CX_CHATLIST;
	static const UINT CY_TEXT = 3*31 + 4;   //3 lines

	//Buttons.
	static const int X_SEND_BUTTON = X_TEXT + CX_TEXT + CX_SPACE;
	static const int Y_SEND_BUTTON = Y_TEXT;

	static const int X_DELETE_BUTTON = X_SEND_BUTTON + CX_BUTTON + CX_SPACE;
	static const int Y_DELETE_BUTTON = Y_SEND_BUTTON;
	static const int X_IGNORE_BUTTON = X_DELETE_BUTTON;
	static const int Y_IGNORE_BUTTON = Y_DELETE_BUTTON + CY_BUTTON + CY_BUTTON_SPACE;

	const int X_CANCEL_BUTTON = (this->w / 2) - CX_BUTTON;
	const int Y_CANCEL_BUTTON = this->h - CY_BUTTON - CY_SPACE;
	const int X_HELP_BUTTON = X_CANCEL_BUTTON + CX_BUTTON + CX_SPACE;
	const int Y_HELP_BUTTON = Y_CANCEL_BUTTON;

	//Title.
	AddWidget(new CLabelWidget(0L, X_TITLE, Y_TITLE,
			CX_TITLE, CY_TITLE, F_Title, g_pTheDB->GetMessageText(MID_ChatTitle)));

	//Chat window.
	this->pChatListBox = new CListBoxWidget(TAG_LIST_BOX,
			X_CHATLIST, Y_CHATLIST, CX_CHATLIST, CY_CHATLIST,
			false, false, true);
	AddWidget(this->pChatListBox);

	//Users list.
	AddWidget(new CLabelWidget(0, X_USERLIST, Y_USERLISTHEADER,
			CX_USERLIST, CY_USERLISTHEADER, F_FrameCaption, g_pTheDB->GetMessageText(MID_PlayersChatting)));
	this->pUsersListBox = new CListBoxWidget(TAG_USERS_BOX,
			X_USERLIST, Y_USERLIST, CX_USERLIST, CY_USERLIST, true, false, true);
	AddWidget(this->pUsersListBox);

	//Text input.
	this->pTextWidget = new CTextBox2DWidget(TAG_INPUT, X_TEXT, Y_TEXT, CX_TEXT, CY_TEXT, 1024, TAG_SEND);
	AddWidget(this->pTextWidget);

	//Buttons.
	this->pSendButton = new CButtonWidget(
			TAG_SEND, X_SEND_BUTTON, Y_SEND_BUTTON, CX_BUTTON, CY_BUTTON,
			g_pTheDB->GetMessageText(MID_ChatSend));
	AddWidget(this->pSendButton);

	this->pDeleteButton = new CButtonWidget(
			TAG_DELETE, X_DELETE_BUTTON, Y_DELETE_BUTTON, CX_BUTTON,
			CY_BUTTON, g_pTheDB->GetMessageText(MID_ChatDelete));
	this->pDeleteButton->Disable();
	AddWidget(this->pDeleteButton);
	this->pIgnoreButton = new CButtonWidget(
			TAG_IGNORE, X_IGNORE_BUTTON, Y_IGNORE_BUTTON, CX_BUTTON,
			CY_BUTTON, g_pTheDB->GetMessageText(MID_ChatIgnoreUser));
	this->pIgnoreButton->Disable();
	AddWidget(this->pIgnoreButton);

	//General buttons.
	CButtonWidget *pExitButton = new CButtonWidget(
			TAG_CANCEL, X_CANCEL_BUTTON, Y_CANCEL_BUTTON, CX_BUTTON, CY_BUTTON,
			g_pTheDB->GetMessageText(MID_ChatExit));
	AddWidget(pExitButton);

	CButtonWidget *pHelpButton = new CButtonWidget(TAG_HELP,
			X_HELP_BUTTON, Y_HELP_BUTTON, CX_BUTTON,
			CY_BUTTON, g_pTheDB->GetMessageText(MID_Help));
	AddWidget(pHelpButton);
	AddHotkey(SDLK_F1,TAG_HELP);
}

//*****************************************************************************
CChatScreen::~CChatScreen()
//Destructor.
{
}

//*****************************************************************************
void CChatScreen::Paint(
//Overridable method to paint the screen.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Blit the background graphic.
	SDL_BlitSurface(this->images[0], NULL, GetDestSurface(), NULL);

	PaintChildren();

	if (bUpdateRect) UpdateRect();
}

//*****************************************************************************
bool CChatScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	this->chat.SetRefreshInterval(10000); //10s automatic refresh

	this->chat.PopulateListBoxFromHistory(this->pChatListBox);
	this->pChatListBox->AddItem(CONNECTION_STATUS_KEY, g_pTheDB->GetMessageText(MID_ChatConnecting), true);
	this->pChatListBox->MoveViewToBottom();

	return true;
}

//
// Private methods
//

//*****************************************************************************
void CChatScreen::OnBetweenEvents()
{
	//Poll for server responses.
	switch (this->chat.PollCNet())
	{
		case CNetChat::Res_Received:
			ProcessReceivedData();
		break;
		case CNetChat::Res_InvalidData:
			this->pChatListBox->RemoveItem(CONNECTION_STATUS_KEY);
			this->pChatListBox->AddItem(CONNECTION_STATUS_KEY,
					g_pTheDB->GetMessageText(MID_ChatReceiveError), true);
			this->pChatListBox->RequestPaint();
		break;
		default:
		case CNetChat::Res_None: break;
	}
}

//*****************************************************************************
void CChatScreen::OnClick(
//Handles a button click.
//
//Params:
	const UINT dwTagNo)       //(in)   Widget that event applied to.
{
	switch (dwTagNo)
	{
		case TAG_HELP:
			CBrowserScreen::SetPageToLoad("chatscreen.html");
			GoToScreen(SCR_Browser);
		break;

		case TAG_DELETE:
			DeleteSelectedMessages();
		break;

		case TAG_SEND:
		{
			SendText(this->pTextWidget->GetText(), this->pUsersListBox);
			this->pTextWidget->SetText(wszEmpty);
			this->pTextWidget->RequestPaint();
		}
		break;

		case TAG_IGNORE:
			IgnoreSelectedUsers();
		break;

		case TAG_CANCEL:
		case TAG_ESCAPE:
			GoToScreen(SCR_Return);
		break;
		case TAG_QUIT:
			if (ShowYesNoMessage(MID_ReallyQuit) != TAG_NO)
				GoToScreen(SCR_None);
		break;
		default: break;
	}
}

//*****************************************************************************
void CChatScreen::OnDeactivate()
{
	//Notify server that user is leaving chat room.  Resolve outstanding requests.
	logoutFromChat();
}

//*****************************************************************************
void CChatScreen::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Key event.
{
	CDrodScreen::OnKeyDown(dwTagNo, Key);

	switch (Key.keysym.sym)
	{
		//Ctrl-c: copy text content of list to clipboard
		case SDLK_c:
			if (Key.keysym.mod & KMOD_CTRL)
			{
				WSTRING wstr;
				for (UINT line=0; line<this->pChatListBox->GetItemCount(); ++line)
				{
					wstr += this->pChatListBox->GetTextAtLine(line);
					wstr += wszCRLF;
				}
				CClipboard::SetString(wstr);
				g_pTheSound->PlaySoundEffect(SEID_MIMIC);
			}
		break;

		default: break;
	}
}

//*****************************************************************************
void CChatScreen::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo)       //(in) Widget that event applied to.
{
	//When a macro to reply to a whisper is received, update the text in the
	//chat box to this immediately.
	switch (dwTagNo)
	{
		case TAG_INPUT:
			ReformatChatText(TAG_INPUT, true);
		break;
		default: break;
	}
}

//*****************************************************************************
void CChatScreen::DeleteSelectedMessages()
//Send request to have selected chat messages deleted on the server-end.
{
	this->chat.deleteRequest(this->pChatListBox->GetSelectedItems());
}

//*****************************************************************************
void CChatScreen::IgnoreSelectedUsers()
//Toggle the ignore flag on selected users.
{
	const UINT userID = this->pUsersListBox->GetSelectedItem();
	if (userID == TAG_EMPTYCHATUSERLIST)
		return; //skip empty list or non-user list entries
	const bool bIgnore = this->pUsersListBox->IsItemEnabled(userID); //toggle
	const WSTRING username = this->pUsersListBox->GetTextForKey(userID);
	this->chat.SendIgnoreRequest(username, bIgnore);

	this->pUsersListBox->EnableItem(userID, !bIgnore);
	this->pUsersListBox->RequestPaint();
}

//*****************************************************************************
void CChatScreen::ProcessReceivedData()
{
	//Show connection has been established.
	this->pChatListBox->SetItemText(CONNECTION_STATUS_KEY, g_pTheDB->GetMessageText(MID_ChatConnected));

	//Add new chats to display in order they were received.
	if (!this->chat.received.empty())
	{
		for (CNetChat::ORDERED_CHATS::const_iterator iter = this->chat.received.begin();
				iter != this->chat.received.end(); ++iter)
		{
			CNetChat::Data *c = *iter;
			if (!this->chat.IsIgnored(c->sender) || c->bAdminMessage)
			{
				this->chat.AddToHistory(c);
				this->chat.AddChatToListBox(this->pChatListBox, c);
			} else {
				delete c;
			}
		}
		this->chat.received.clear();

		//Move list view to bottom of list.
		this->pChatListBox->MoveViewToBottom();
	}
	this->pChatListBox->RequestPaint();

	//Remove flagged previously received chat records from display.
	CNetChat::CHAT_VECTOR::const_iterator iter;
	if (!this->chat.deleted.empty())
	{
		for (iter = this->chat.deleted.begin(); iter != this->chat.deleted.end(); ++iter)
		{
			CNetChat::Data* c = *iter;
			this->pChatListBox->RemoveItem(c->chatID);
			delete c;
		}
		this->chat.deleted.clear();
		this->pChatListBox->RequestPaint();
	}

	//Users list is refreshed each update.
	PopulateChatUserList(TAG_USERS_BOX);

	this->pIgnoreButton->Enable(!this->pUsersListBox->IsEmpty());
	this->pIgnoreButton->RequestPaint();
	this->pUsersListBox->RequestPaint();
}

//*****************************************************************************
void CChatScreen::SendText(const WCHAR* pText, const CListBoxWidget *pListBox)
//Uploads text inputted by the user.
{
	SelectWidget(TAG_INPUT); //refocus the text box

	this->chat.SendText(pText, pListBox);
}
