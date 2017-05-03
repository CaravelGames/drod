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
 * Portions created by the Initial Developer are Copyright (C) 
 * 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//DbMessageText.h
//Declarations for CDbMessageText.
//Class for handling message texts set and retrieved from database.

//USAGE
//
//To provide access to any message text in the database, you can construct a new
//CDbMessageText with the message ID you want.  The text will load on the first
//time it is used, and remain in memory until the object is destroyed.  
//
//  CDbMessageText Text(MID_UnexpectedNeatherDeath);
//  ShowOkayMessage(Text); //Text not loaded until this line.
//
//A one-time construction should work fine too:
//
//  ShowOkayMessage(CDbMessageText(MID_UnexpectedNeatherDeath)); //Text loaded for the call then freed.
//
//Overwriting the value can be done with an assignment call:
//
//  {
//    CDbMessageText NameText(dwNameMID);
//    NameText = L"Charlie";
//    NameText.UpdateText();
//  }
//
//The actual database write will occur when the call to UpdateText() is made.
//
//If you don't have the message ID at time of construction, you might want to make a call
//to Bind() later.  Like so:
//
//  CDbMessageText NameText;
//  UINT dwNameMID = GetNameMID();
//  if (dwNameMID)
//  {
//    NameText.Bind(dwNameMID);
//    NameText = L"Charlie";
//    NameText.UpdateText();
//  }
//
//If you want to add a new message text, you can assign text to an unbound object and 
//commit it.  UpdateText() will return a new message ID, and the object will then be bound
//to that new message.  Like so:
//
//  CDbMessageText NameText;
//  NameText = L"Charlie";
//  UINT dwNameMID = NameText.UpdateText();
//
//OTHER COMMENTS
//
//Don't expect two or more CDbMessageText objects updating the same message at the
//same time to stay in sync.
//
//The currently selected language affects all operations.  Each message text is uniquely
//identified by a message ID and language code.

#ifndef DBMESSAGETEXT_H
#define DBMESSAGETEXT_H

#include "DbBase.h"
#include <BackEndLib/AttachableObject.h>
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/StretchyBuffer.h>
#include <BackEndLib/Wchar.h>

#include <string>
using std::string;

#define UNBOUND_MESSAGE  ((MESSAGE_ID)0)

class CDbMessageText : public CDbBase, public CAttachableObject
{
public:
	CDbMessageText(const MESSAGE_ID eSetMessageID = UNBOUND_MESSAGE);
	
	//Called implicitly by constructor or explicitly to bind the text to an existing
	//message in the database.
	void  Bind(const MESSAGE_ID eSetMessageID);

	//Release binding and set string to empty without making any writes.
	void  Clear();

	//Called explicitly to write any text changes to the database.
	//If you don't bind to a message ID, a new one will be created.  The 
	//bound message ID will be returned in either case.
	MESSAGE_ID UpdateText();

	//Deletes message text permanently from the DB.
	void  Delete();

	//Whether this text exists in the DB.
	virtual bool Exists(const UINT dwID) const;

	//Outputs language texts for this message ID.
	void  ExportText(CStretchyBuffer& buf, const char* pTagName);
	void  ImportText(const char** atts);

	//Get size of string.
	UINT GetSize() {Load(); return this->wstrText.size();}
	bool  IsEmpty() {Load(); return this->wstrText.size() == 0;}

	//Get message ID if it's really needed.
	UINT GetMessageID() const {return this->eMessageID;}

	void  Load();

	void SaveNewInstance() { this->eMessageID = 0; this->bIsDirty = true; }

	virtual bool Update() {UpdateText(); return true;}

	//Invoke this to always save the message object using the default language code.
	void UseDefaultLanguage(const bool bVal=true) {this->bUseDefaultLanguage = bVal;}

	//Assign a new string.
	const WCHAR * operator = (const WCHAR * wczSet);
	const WCHAR * operator = (const CDbMessageText &text);

	//Append a string.
	const WCHAR * operator += (const WCHAR * wczSet);

	//Compare two strings.
	bool operator == (CDbMessageText &text);
	bool operator == (const WSTRING &text);
	bool operator == (const WCHAR* text);
	bool operator != (const WCHAR* text);

	//Returns const string, loading from DB if needed.  Don't try to modify the 
	//returned string, because you will be circumventing the state change code.  
	//Use above string modifications operators or add new ones.
	operator const WCHAR *();
	operator const WCHAR *() const;  //no loading from DB

private:
	MESSAGE_ID     eMessageID;
	WSTRING         wstrText;
	bool           bIsLoaded;
	bool           bIsDirty;
	bool           bUseDefaultLanguage;
};

#endif //...#ifndef DBMESSAGETEXT_H
