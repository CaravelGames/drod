// $Id: Chat.h 8502 2008-01-17 20:55:17Z mrimer $

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

#ifndef CHAT_H
#define CHAT_H

#include "../DRODLib/NetInterface.h"

#include <BackEndLib/StretchyBuffer.h>
#include <BackEndLib/IDSet.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>

#include <expat.h>

#include <SDL.h>

#include <deque>
#include <set>
#include <vector>
using std::vector;

class CListBoxWidget;
namespace CNetChat
{
	//Enumeration of supported XML fields.
	enum ChatTagType {
		First_Chat_Tag=0,
		ChatID=First_Chat_Tag, //unique ID within this chat "room"
		OpType,			 //type of chat operation
		Timestamp,      //CaravelNet message received time
		Sender,         //sending player's CaravelNet name
		Text,           //chat text
		Receiver,       //usernames receiving a whispered message
		RefreshFreq,    //server-requested refresh interval
		AdminText,      //administrator announcement
		Color,
		UserIsAFK,      //indicates 'Sender' is AFK
		Chat_Tag_Count,
		Unknown_Chat_Tag
	};
	enum ChatOpType {     //server reports various contexts:
		Op_Received=0,     //chat received
		Op_Delete=1,       //a previous chat is to be deleted
		Op_UserOnline=2,   //notice that user is chatting
		Op_IgnoreUser=3,   //I am ignoring a user
		Op_UnignoreUser=4, //I am unignoring a user
		Op_Friend=5,       //user(s) marked as friends
		Op_Unfriend=6      //user is no longer marked as a friend
	};
	enum PollResponse {
		Res_None,       //nothing new was observed during poll
		Res_Received,   //received new data from CNet
		Res_InvalidData //received invalid data response from CNet (server error or bad user key)
	};
	enum TextReformat {
		Ref_None,  //no reformat occurred on last reformat check
		Ref_Reply  //reply text was reformatted
	};

	struct Data
	{
		Data() : chatID(0), opType(0), timestamp(0), bAdminMessage(false), bAFK(false)
		{
			color.r = color.g = color.b = 0;
		}

		UINT chatID;      //unique ID
		UINT opType;      //operation
		UINT timestamp;   //time received by CaravelNet
		WSTRING sender;   //message sender
		WSTRING text;     //message text
		vector<WSTRING> whisperedUsers;   //users to whom message was whispered
		bool bAdminMessage; //administrator message
		SDL_Color color;  //color of text
		bool bAFK;        //indicates user is AFK
	};

	struct orderChats {
		bool operator() (const Data* p1, const Data* p2) const;
	};
	typedef std::set<Data*, orderChats> ORDERED_CHATS;
	typedef std::vector<Data*> CHAT_VECTOR;

	//CaravelNet chat interface.
	class Interface
	{
	public:
		Interface();
		virtual ~Interface();

		void AddToHistory(Data* c);
		WSTRING FormatMessageForDisplay(const Data *c) const;

		void deleteRequest(const CIDSet& ids);

		void    AddUserWhoWhisperedMe(const WSTRING& sender);
		WSTRING GetLastUserWhoWhisperedMe() const;
		WSTRING GetNextUserWhoWhisperedMe();

		void IgnoreUser(const WSTRING& username, const bool bIgnore);
		bool IsBusy() const {return this->handle != 0;}
		bool IsClientUserAFK() const;
		bool IsFriend(const WSTRING& username) const;
		bool IsIgnored(const WSTRING& username) const;
		bool IsWhisperedTo(const Data& data, const WSTRING& username) const;
		void refreshNow() {this->timeOfLastUpdate = 0;}
		void reset();
		void resolveOutstandingRequests(const UINT timeout=5000); //ms

		PollResponse PollCNet();
		void AddChatToListBox(CListBoxWidget* pListBox, const Data* c) const;
		void PopulateListBoxFromHistory(CListBoxWidget* pListBox) const;
		bool ReformatText(WSTRING &text, const bool bNextOption=false);
		void SendAFK(const bool bAFK=true);
		void SendIgnoreRequest(const WSTRING& username, const bool bIgnore);
		void SendText(const WCHAR* pText, const CListBoxWidget *pListBox=NULL);
		void SetFriend(const WSTRING& username, const bool bBefriend);
		void parseUserlist(const WCHAR* pUserlist, vector<WSTRING>& out);
		void SetRefreshInterval(const UINT time);

		//XML parsing.
		void StartElement(const XML_Char *name, const XML_Char **atts);

		//Data received.
		ORDERED_CHATS         received;
		vector<Data*> deleted, users, history;
		vector<WSTRING> ignoredUsers, friends;
		std::deque<WSTRING> usersWhoWhisperedMe;

	private:
		UINT handle;
		UINT timeOfLastUpdate, lastIDreceived;
		CIDSet chatIDsReceived;
		UINT refreshInterval;
		vector<SendData> textsToSend;
		UINT timeOfLastRefreshRequest;
		TextReformat lastTextReformat;
		bool bIsUserAFK;

		WSTRING GetWhisperText(const WSTRING& whisperer, const WCHAR* pText) const;

		bool        Parse(CStretchyBuffer &buffer);
		bool        ParseCommandMacro(const WCHAR *pText, const CListBoxWidget *pListBox);
		ChatTagType ParseTagField(const char *str) const;
	};
};

#endif //...#ifndef CHAT_H
