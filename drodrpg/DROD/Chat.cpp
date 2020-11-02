// $Id: Chat.cpp 8502 2008-01-17 20:55:17Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2006
 * Caravel Software. All Rights Reserved.
 *
 * Contributors:
 *
 * ***** END LICENSE BLOCK ***** */

#include "Chat.h"
#include "DrodFontManager.h"
#include <FrontEndLib/EventHandlerWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include "../DRODLib/Db.h"
#include "../DRODLib/NetInterface.h"
#include <BackEndLib/Files.h>
#include <BackEndLib/Base64.h>
#include <BackEndLib/Ports.h>

#include <SDL.h>

namespace CNetChat {

const WCHAR ignoreShortStr[] = {We('i'),We(' '),We(0)};
const WCHAR replyStr[] = {We('r'),We('e'),We('p'),We('l'),We('y'),We(' '),We(0)};
const WCHAR replyShortStr[] = {We('r'),We(' '),We(0)};
const WCHAR unignoreShortStr[] = {We('u'),We(' '),We(0)};
const WCHAR whisperShortStr[] = {We('w'),We(' '),We(0)};
const WCHAR afkStr[] = {We('a'),We('f'),We('k'),We(' '),We(0)};
const WCHAR notAfkStr[] = {We('n'),We('a'),We('f'),We('k'),We(' '),We(0)};

#define ACTIVE_REFRESH_INTERVAL (10000) //10s
#define AFK_REFRESH_INTERVAL (60000)    //1m

//*****************************************************************************
inline BYTE getXdigit(const char c)
//Returns: hex char converted into byte value
{
	ASSERT(isxdigit(c));
	if (isdigit(c))
		return c - '0';
	return 10 + tolower(c) - 'a';
}

//*****************************************************************************
void stripCRs(WSTRING& line)
//Strips carriage returns from this string.
{
	//Strip CRs from text to display.
	for (UINT i=0; i<line.size(); ++i)
	{
		if (line[i] == '\r')
			line.erase(i--, 1);
	}
}

//*****************************************************************************
//
//Expat callback entry point (private)
//

void ChatStartElement_cb (
	void *pObject, // (in) Pointer to caller object
	const XML_Char *name, const XML_Char **atts)
{
	((Interface*)pObject)->StartElement(name, atts);
}

//*****************************************************************************
bool orderChats::operator() (const Data* p1, const Data* p2) const
//Compare the ordering of two chat messages.
{
	return p1->chatID < p2->chatID; //IDs are always in order of receival by server
}

//*****************************************************************************
Interface::Interface()
	: handle(0)
	, timeOfLastUpdate(0), lastIDreceived(0)
	, refreshInterval(ACTIVE_REFRESH_INTERVAL)
	, timeOfLastRefreshRequest(0)
	, lastTextReformat(Ref_None)
	, bIsUserAFK(false)
{}

//*****************************************************************************
Interface::~Interface()
{
	reset();
}

//*****************************************************************************
void Interface::deleteRequest(const CIDSet& /*ids*/)
//Request that server have chat messages with these IDs removed from the chat log.
{
	//!!implement
	//g_pTheNet->DeleteChats(ids);
}

//*****************************************************************************
void Interface::AddToHistory(Data* c)
//Adds a chat to the history.
{
	ASSERT(c);
	this->history.push_back(c);

	//Maintains the last user who whispered me.
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	const WSTRING playerCNetName = pPlayer ? (const WCHAR*)(pPlayer->CNetNameText) : wszEmpty;
	delete pPlayer;
	if (!c->whisperedUsers.empty() && c->sender.compare(playerCNetName))
		AddUserWhoWhisperedMe(c->sender);
}

//*****************************************************************************
WSTRING Interface::FormatMessageForDisplay(const Data *c) const
//Formats a chat data object for display to user.
{
	WSTRING wstr = c->sender;

	//Whispered list.
	if (!c->whisperedUsers.empty())
	{
		wstr += wszSpace;
		wstr += wszLeftBracket;
		wstr += g_pTheDB->GetMessageText(MID_WhisperedTo);
		wstr += wszSpace;
		wstr += c->whisperedUsers.front();
		for (vector<WSTRING>::const_iterator iter = c->whisperedUsers.begin()+1;
				iter != c->whisperedUsers.end(); ++iter)
		{
			wstr += wszComma;
			wstr += *iter;
		}
		wstr += wszRightBracket;
	}

	wstr += wszColon;
	wstr += wszSpace;
	wstr += c->text;

	return wstr;
}

//*****************************************************************************
void Interface::AddUserWhoWhisperedMe(const WSTRING& sender)
//If sender is in users who have whispered me, move to front of deque,
//otherwise simply add to front of deque.
//Post-condition: 'sender' is at front of deque.
{
	for (std::deque<WSTRING>::iterator iter=this->usersWhoWhisperedMe.begin();
			iter!=this->usersWhoWhisperedMe.end(); ++iter)
	{
		if (!sender.compare(*iter))
		{
			this->usersWhoWhisperedMe.erase(iter);
			break;
		}
	}

	//Push on front of deque.
	this->usersWhoWhisperedMe.push_front(sender);
}

//*****************************************************************************
WSTRING Interface::GetLastUserWhoWhisperedMe() const
//Returns: front entry in queue of last whisper senders
{
	if (this->usersWhoWhisperedMe.empty())
		return wszEmpty;
	return this->usersWhoWhisperedMe.front();
}

//*****************************************************************************
WSTRING Interface::GetNextUserWhoWhisperedMe()
//Returns: next entry in queue of last whisper senders.
//If there is no next user, return the first user in the queue.
//Post-cond: non-empty queue has front entry rotated to end
{
	if (this->usersWhoWhisperedMe.empty())
		return wszEmpty;

	const WSTRING prevWhisperer = this->usersWhoWhisperedMe.front();
	this->usersWhoWhisperedMe.pop_front();
	this->usersWhoWhisperedMe.push_back(prevWhisperer);

	for (std::deque<WSTRING>::const_iterator iter=this->usersWhoWhisperedMe.begin();
			iter!=this->usersWhoWhisperedMe.end(); ++iter)
	{
		if (!prevWhisperer.compare(*iter))
		{
			++iter;
			if (iter==this->usersWhoWhisperedMe.end())
				iter = this->usersWhoWhisperedMe.begin();
			return *iter;
			break;
		}
	}

	//No match -- return same last user.
	return prevWhisperer;
}

//*****************************************************************************
void Interface::IgnoreUser(const WSTRING& username, const bool bIgnore)
//If bIgnore is set, then add username to ignored list.  Otherwise remove it.
{
	for (vector<WSTRING>::iterator iter = this->ignoredUsers.begin();
			iter != this->ignoredUsers.end(); ++iter)
	{
		if (!iter->compare(username))
		{
			//Found this name in the ignored list.
			if (!bIgnore) //Remove it.
			{
				this->ignoredUsers.erase(iter);
				return;
			}
		}
	}

	//Username was not found.
	if (bIgnore) //Add it.
		this->ignoredUsers.push_back(username);
}

//*****************************************************************************
bool Interface::IsClientUserAFK() const
//Returns: whether anough time has passed since user input to consider
//the client user is AFK.
{
	static const UINT AFK_SECONDS = 600; //10m
	const UINT dwLastUserEvent = CEventHandlerWidget::GetTimeOfLastUserInput();
	const UINT dwSecondsSinceLastEvent = (SDL_GetTicks() - dwLastUserEvent) / 1000;
	return dwSecondsSinceLastEvent > AFK_SECONDS;
}

//*****************************************************************************
bool Interface::IsFriend(const WSTRING& username) const
//Returns: whether username is declared a friend
{
	for (vector<WSTRING>::const_iterator iter = this->friends.begin();
			iter != this->friends.end(); ++iter)
	{
		if (!iter->compare(username))
			return true;
	}
	return false;
}

//*****************************************************************************
bool Interface::IsIgnored(const WSTRING& username) const
//Returns: whether username is being ignored
{
	for (vector<WSTRING>::const_iterator iter = this->ignoredUsers.begin();
			iter != this->ignoredUsers.end(); ++iter)
	{
		if (!iter->compare(username))
			return true;
	}
	return false;
}

//*****************************************************************************
bool Interface::IsWhisperedTo(const Data& data, const WSTRING& username) const
//Returns: whether username is either one of those whispered to in this message data
//         or the message author
{
	//Message author.
	if (!data.sender.compare(username))
		return true;

	//One of the message recipients.
	for (vector<WSTRING>::const_iterator iter = data.whisperedUsers.begin();
			iter != data.whisperedUsers.end(); ++iter)
	{
		if (!iter->compare(username))
			return true;
	}
	return false;
}

//*****************************************************************************
void Interface::SetFriend(const WSTRING& username, const bool bBefriend)
//If bBefriend is set, then add username to friends list.  Otherwise remove it.
{
	for (vector<WSTRING>::iterator iter = this->friends.begin();
			iter != this->friends.end(); ++iter)
	{
		if (!iter->compare(username))
		{
			//Found this name in the friends list.
			if (!bBefriend) //Remove it.
			{
				this->friends.erase(iter);
				return;
			}
		}
	}

	//Username was not found.
	if (bBefriend) //Add it.
		this->friends.push_back(username);
}

//*****************************************************************************
void Interface::reset()
//Resets interface state.
{
	resolveOutstandingRequests();

	this->lastIDreceived = 0;
	this->timeOfLastUpdate = 0;
	this->chatIDsReceived.clear();

	ORDERED_CHATS::const_iterator recIter;
	for (recIter = this->received.begin(); recIter != this->received.end(); ++recIter)
		delete *recIter;
	this->received.clear();

	CHAT_VECTOR::const_iterator iter;
	for (iter = this->deleted.begin(); iter != this->deleted.end(); ++iter)
		delete *iter;
	this->deleted.clear();

	for (iter = this->users.begin(); iter != this->users.end(); ++iter)
		delete *iter;
	this->users.clear();

	for (iter = this->history.begin(); iter != this->history.end(); ++iter)
		delete *iter;
	this->history.clear();
}

//*****************************************************************************
void Interface::resolveOutstandingRequests(const UINT timeout) //ms
//Wait for any outstanding queries/sends to finish before exiting.
{
	if (this->handle)
	{
		Uint32 dwNow, dwStartWaiting = SDL_GetTicks();
		while (g_pTheNet->GetStatus(this->handle) < 0)
		{
			SDL_Delay(20);
			if ((dwNow = SDL_GetTicks()) > dwStartWaiting + timeout)
				break; //stop waiting
		}
		CStretchyBuffer* pBuffer = g_pTheNet->GetResults(this->handle);
		delete pBuffer;
		this->handle = 0;
	}
}

//*****************************************************************************
PollResponse Interface::PollCNet()
//Receives chat responses and sends chat messages.
//
//Returns: true if operations completed normally; false if server response error occurred
{
	bool bReceivedData = false;
	bool bBadDataReceived = false;

	//Ensure last request was completed before another upload is initiated.
	if (this->handle)
	{
		if (g_pTheNet->GetStatus(this->handle) >= 0)
		{
			//Received data.  Process.
			bReceivedData = true;
			CStretchyBuffer* pBuffer = g_pTheNet->GetResults(this->handle);
			this->handle = 0;
			if (pBuffer)
			{
				if (pBuffer->Size())
					bBadDataReceived = !Parse(*pBuffer);
				delete pBuffer;
			} //else server sent invalid response
		}
	}

	//Net interface is idle.  Handle uploads and downloads.
	if (!this->handle)
	{
		if (!this->textsToSend.empty())
		{
			//There are texts to send.

			//Since user is sending texts, they are obviously not AFK now.
			//Return refresh interval to normal.
			if (this->bIsUserAFK)
			{
				this->bIsUserAFK = false;
				this->refreshInterval = ACTIVE_REFRESH_INTERVAL;
				SendAFK(false);
			}

			this->handle = g_pTheNet->SendChatText(
					this->textsToSend, this->lastIDreceived, g_pTheNet->GLOBAL_CHATROOM);
			this->timeOfLastUpdate = SDL_GetTicks();
			this->textsToSend.clear();
		} else {
			//Request updated chat log while idle.
			Uint32 dwNow = SDL_GetTicks();
			if (dwNow > this->timeOfLastUpdate + this->refreshInterval)
			{
				//Send empty posting to refresh.

				//AFK check.
				const bool bIsUserNowAFK = IsClientUserAFK();
				if (bIsUserNowAFK && !this->bIsUserAFK)
				{
					//User has gone AFK.
					//Slow down text refresh.
					this->refreshInterval = AFK_REFRESH_INTERVAL;
					SendAFK();
				}
				else if (!bIsUserNowAFK && this->bIsUserAFK)
				{
					//User has returned.
					//Resume normal refresh interval.
					this->refreshInterval = ACTIVE_REFRESH_INTERVAL;
					SendAFK(false);
				}
				this->bIsUserAFK = bIsUserNowAFK;

				this->handle = g_pTheNet->SendChatText(
					this->textsToSend, this->lastIDreceived, g_pTheNet->GLOBAL_CHATROOM);
				this->timeOfLastUpdate = SDL_GetTicks();
			}
		}
	}

	if (bBadDataReceived)
		return Res_InvalidData;
	return bReceivedData ? Res_Received : Res_None;
}

//*****************************************************************************
void Interface::AddChatToListBox(CListBoxWidget* pListBox, const Data* c) const
//Adds a new chat text to the bottom of the items in the list box widget.
{
	ASSERT(pListBox);
	ASSERT(c);
	const WSTRING wstr = FormatMessageForDisplay(c);
	const UINT width = pListBox->GetW() - pListBox->GetVScrollBarWidth() - 5;
	UINT newChars = 0, index = 0;
	do {
		newChars = g_pTheFM->GetCharsThatFitWithin(FONTLIB::F_ListBoxItem,
				wstr.c_str() + index, width);
		if (!newChars)
			break; //some weird, unsupported text was encountered -- ignore it

		WSTRING line = wstr.c_str() + index;
		line.resize(newChars);
		index += newChars;
		stripCRs(line);
		const UINT index = pListBox->AddItem(c->chatID, line.c_str());
		pListBox->SetItemColorAtLine(index, c->color);
	} while (index < wstr.size());
}

//*****************************************************************************
void Interface::PopulateListBoxFromHistory(CListBoxWidget* pListBox) const
//Sets the items in the given list box widget to the chat history.
{
	pListBox->Clear();
	for (UINT i=0; i<this->history.size(); ++i)
		AddChatToListBox(pListBox, this->history[i]);
}

//*****************************************************************************
bool Interface::ReformatText(
//Call this to update a text on the fly as the user is entering it into a text box.
//
//Returns: whether text was reformatted
//
//Params:
	WSTRING &text,          //(in/out) text to reformat
	const bool bNextOption) //[default=false] whether to reformat text as last time, choosing next reformat parameters
{
//Predefined macros and console command tokens.
#define MACROMATCH(pwMacroStr) (!WCSncmp(pText+index, pwMacroStr, WCSlen(pwMacroStr)))
#define ADVANCE_TO_NEXT_TOKEN {while (!iswspace(pText[index])) ++index; \
		while (iswspace(pText[index])) ++index;}

	//No text to reformat.
	if (text.empty())
		return false;

	//Is this a macro text?
	const WCHAR *pText = text.c_str();
	if (pText[0] != W_t('/')) //reformat context-sensitive text macro
		return false;

	//Immediately after a successful text reformat,
	//adding a Space at the end of the text (i.e. due to hitting "Space" again)
	//will update text to show next available macro parameter options.
	if (bNextOption)
	{
		if (pText[text.length()-1] == W_t(' ')) //last char is a space
		{
			switch (this->lastTextReformat)
			{
				//Switch to whisper to a different user who whispered me.
				case Ref_Reply:
				{
					WSTRING nextWhisperer = GetNextUserWhoWhisperedMe();
					if (nextWhisperer.empty())
						break; //last whisperer list is now empty -- no text to use
					text = GetWhisperText(nextWhisperer, NULL); //reset text message
				}
				return true; //done

				//No reformat on record -- just process as normal below.
				default:
				case Ref_None: break;
			}
		}
	}

	this->lastTextReformat = Ref_None;
	UINT index=1; //skip slash

	//Reply in whisper to the last user who whispered me.
	if (MACROMATCH(replyStr) || MACROMATCH(replyShortStr))
	{
		const WSTRING lastWhisperer = GetLastUserWhoWhisperedMe();
		if (lastWhisperer.size()) //reply macro is meaningful
		{
			//Construct a whisper message back to the last whisperer.
			this->lastTextReformat = Ref_Reply;
			ADVANCE_TO_NEXT_TOKEN;
			text = GetWhisperText(lastWhisperer, pText + index);
			return true; //text was changed
		}
		return false;
	}

	return false; //no changes

#undef MACROMATCH
#undef ADVANCE_TO_NEXT_TOKEN
}

//*****************************************************************************
void Interface::SendAFK(const bool bAFK) //[default=true]
{
	WSTRING wstr = wszForwardSlash;
	wstr += bAFK ? afkStr : notAfkStr;
	this->textsToSend.push_back(SendData(wstr));
}

//*****************************************************************************
void Interface::SendIgnoreRequest(const WSTRING& username, const bool bIgnore)
//Request server mark me as ignoring/unignoring a user.
{
	WSTRING wstr = wszForwardSlash;
	wstr += bIgnore ? ignoreShortStr : unignoreShortStr;
	wstr += wszQuote; //in case username has spaces in it
	wstr += username;
	wstr += wszQuote;
	this->textsToSend.push_back(SendData(wstr));
}

//*****************************************************************************
void Interface::SendText(const WCHAR* pText, const CListBoxWidget *pListBox) //[default=NULL]
//Uploads inputted text.  If userIDs is not empty, then it indicates who message is whispered to.
{
	if (!pText)
		return;

	if (!WCSlen(pText))
	{
		//An empty line may be sent to explicitly request a chat refresh.
		//We'll throttle the maximum frequency of such requests.
		static const UINT MAXIMUM_REFRESH_REQUEST_RATE = 3000; //ms
		const Uint32 now = SDL_GetTicks(); 
		if (this->timeOfLastRefreshRequest + MAXIMUM_REFRESH_REQUEST_RATE > now)
			return; //too early to make another refresh request
		this->timeOfLastRefreshRequest = now;
	}

	//Check text for special command macros.
	if (ParseCommandMacro(pText, pListBox))
		return;

	//Queue text.  It gets uploaded when the net interface is ready.
	CIDSet userIDs;
	if (pListBox)
		userIDs = pListBox->GetSelectedItems();
	this->textsToSend.push_back(SendData(WSTRING(pText), userIDs));
}

//*****************************************************************************
void Interface::SetRefreshInterval(const UINT time)
{
	static const UINT MIN_REFRESH_INTERVAL = 1000; //ms

	this->refreshInterval = time < MIN_REFRESH_INTERVAL ? MIN_REFRESH_INTERVAL : time;
}

//*****************************************************************************
void Interface::parseUserlist(const WCHAR* pUserlist, vector<WSTRING>& out)
//Parse comma-separated list of user names
{
	out.clear();
	if (!pUserlist)
		return;

	WSTRING wstr = pUserlist;
	WCHAR *pStr = (WCHAR*)(wstr.c_str());
	WCHAR *pUsername = WCStok(pStr, wszComma);
	while (pUsername)
	{
		if (WCSlen(pUsername))
			out.push_back(WSTRING(pUsername));
		pUsername = WCStok(NULL, wszComma);
	}
}

//*****************************************************************************
void Interface::StartElement(
//Expat callback function: Process XML start tag, and attributes.
//Parses the information for one CaravelNet chat operation each iteration.
//
//Params:
	const XML_Char *name, const XML_Char **atts)
{
	if (!stricmp(name, "DROD")) return; //skip root header
	if (stricmp(name, "chat")) return; //only process chat tags

	Data *c = new Data;

	//Parse chat tag fields.
	int i;
	char *str;
	for (i = 0; atts[i]; i += 2) {
		const ChatTagType pType = ParseTagField(atts[i]);
		if (pType == Unknown_Chat_Tag)
		{
			//Ignore unknown tag fields.
			continue;
		}
		str = (char* const)atts[i + 1];
		switch (pType)
		{
			case ChatID:
				c->chatID = convertToUINT(str);
				break;
			case OpType:
				c->opType = convertToUINT(str);
				break;
			case Timestamp:
				c->timestamp = convertToUINT(str);
				break;
			case Sender:
				UTF8ToUnicode(str, c->sender);
				break;
			case Text:
				Base64::decode(str, c->text);
				break;
			case Receiver:
			{
				WSTRING username;

				//Parse comma-separated usernames.
				str = strtok(str,",");
				do {
					if (str)
					{
						UTF8ToUnicode(str, username);
						c->whisperedUsers.push_back(username);
					}
				}
				while ((str = strtok(NULL,",")));
			}
			break;
			case RefreshFreq:
				SetRefreshInterval(convertToUINT(str));
			break;
			case AdminText:
				Base64::decode(str, c->text);
				c->bAdminMessage = true;
			break;
			case Color:
			{
				ASSERT(strlen(str) == 6); //"rrggbb" format
				c->color.r = getXdigit(str[0]) * 16 + getXdigit(str[1]);
				c->color.g = getXdigit(str[2]) * 16 + getXdigit(str[3]);
				c->color.b = getXdigit(str[4]) * 16 + getXdigit(str[5]);
			}
			break;
			case UserIsAFK:
				c->bAFK = true;
			break;
			default:
				break;   //skip any unrecognized fields
		}
	}

	//Queue records for processing in the right order.
	switch (c->opType)
	{
		case Op_Received:
			//When there is lag between the client and server,
			//it may be that the client requests the same chat messages twice.
			//However, if we receive a chat message that has already been received,
			//then this duplicate send should be skipped by the client.
			if (this->chatIDsReceived.has(c->chatID))
			{
				delete c;
			} else {
				this->chatIDsReceived += c->chatID;
				this->received.insert(c);
				if (this->lastIDreceived < c->chatID)
					this->lastIDreceived = c->chatID;
			}
		break;
		case Op_Delete:
			this->deleted.push_back(c);
		break;
		case Op_UserOnline:
			this->users.push_back(c);
		break;
		case Op_IgnoreUser:
			parseUserlist(c->sender.c_str(), this->ignoredUsers);
		break;
		case Op_UnignoreUser:
			IgnoreUser(c->sender, false);
		break;
		case Op_Friend:
			parseUserlist(c->sender.c_str(), this->friends);
		break;
		case Op_Unfriend:
			SetFriend(c->sender.c_str(), false);
		break;
		default:
			delete c;
		break; //ignore unrecognized operations
	}
}

//
// Private methods.
//

//*****************************************************************************
WSTRING Interface::GetWhisperText(const WSTRING& whisperer, const WCHAR *pText) const
//Returns a macro text indicating to whisper to the indicated user.
{
	WSTRING wstr = wszForwardSlash;
	wstr += whisperShortStr;
	wstr += wszQuote; //in case user name has spaces in it
	wstr += whisperer;
	wstr += wszQuote;
	wstr += wszSpace;
	if (pText)
		wstr += pText;
	return wstr;
}

//*****************************************************************************
bool Interface::Parse(CStretchyBuffer &buffer)
//Parses CaravelNet chat updates.
//
//Returns: true if data received were valid, else false
{
	const UINT size = buffer.Size();
	if (size <= 10)	//quick check for invalid XML data
		return false;

	//Parser init.
	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, this);
	XML_SetElementHandler(parser, ChatStartElement_cb, NULL);

	char *buf = ((char*)(BYTE*)buffer);

	//Parse the XML.
	const bool bError = XML_Parse(parser, buf, size, true) == XML_STATUS_ERROR;
	if (bError)
	{
		//Some problem occurred.
		char errorStr[256];
		_snprintf(errorStr, 256,
				"Chat Parse Error: %s at line %u:%u" NEWLINE
				"BufferSize=%u" NEWLINE,
				XML_ErrorString(XML_GetErrorCode(parser)),
				(UINT)XML_GetCurrentLineNumber(parser),
				(UINT)XML_GetCurrentColumnNumber(parser),
				size);
		CFiles Files;
		Files.AppendErrorLog((char *)errorStr);
		Files.AppendErrorLog(buf);	//output the problem text
	}
	XML_ParserFree(parser);

	return !bError;
}

//*****************************************************************************
bool Interface::ParseCommandMacro(const WCHAR *pText, const CListBoxWidget* /*pListBox*/)
//Scan text for predefined operation macros.
//ListBox contains usernames + ID keys.
//
//Returns: whether text starts with a slash, indicating a chat macro
{
	ASSERT(pText);
	if (pText[0] != W_t('/')) //all macros begin with slash
		return false; //not a macro

	//Send macros as plaintext to the server.
	//It will handle friend/ignore/whisper requests, etc.
	this->textsToSend.push_back(SendData(WSTRING(pText)));
	return true;
}

//*****************************************************************************
ChatTagType Interface::ParseTagField(const char *str)
//Returns: enumeration corresponding to tag name
const
{
	static const char *tagStr[Chat_Tag_Count] = {
		"ID", "OpType", "Time", "Sender", "Text", "Receiver", "RefreshFreq",
		"Admin", "Color", "AFK"
	};

	for (int eTag=First_Chat_Tag; eTag<Chat_Tag_Count; ++eTag)
		if (!stricmp(str, tagStr[eTag])) //not case sensitive
			return ChatTagType(eTag);

	return Unknown_Chat_Tag;
}

}; //namespace CNetChat
