// $Id: DbMessageText.cpp 8102 2007-08-15 14:55:40Z trick $

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

#include "DbMessageText.h"

//
//Public methods.
//

//*****************************************************************************
CDbMessageText::CDbMessageText(
//Constructor.
//
//Params:
	const MESSAGE_ID eSetMessageID) //(in) MessageID that text will be bound
											 //       to.  UNBOUND_MESSAGE (0) is default.
	: CAttachableObject()
	, eMessageID(UNBOUND_MESSAGE)
	, wstrText(wszEmpty)
	, bIsLoaded(false), bIsDirty(false)
	, bUseDefaultLanguage(false)
{
	if (eSetMessageID)
		Bind(eSetMessageID);
}

//*****************************************************************************
void CDbMessageText::Bind(
//Bind text to this message ID.  Subsequent database loads and updates will
//affect a record in MessageTexts determined by this value.
//
//Params:
	const MESSAGE_ID eSetMessageID)  //(in)   MessageID to bind to.
{
	ASSERT(this->eMessageID == UNBOUND_MESSAGE);
	ASSERT(eSetMessageID != UNBOUND_MESSAGE);

	this->eMessageID = eSetMessageID;
	this->bIsLoaded = false;
}

//*****************************************************************************
void CDbMessageText::Clear()
//Release binding and set string to empty without making any writes.
{
	this->bIsDirty = this->bIsLoaded = false;
	this->eMessageID = UNBOUND_MESSAGE;
	this->wstrText.resize(0);
}

//*****************************************************************************
MESSAGE_ID CDbMessageText::UpdateText()
//Commits changes to the database.  If text was not bound, it will become bound
//to new message ID.
{
	ASSERT(CDbBase::IsOpen());
	if (this->bIsDirty)
	{
		const Language::LANGUAGE curLanguage = Language::GetLanguage();
		if (this->bUseDefaultLanguage)
			Language::SetLanguage(Language::English);
		if (this->eMessageID != UNBOUND_MESSAGE)
			this->eMessageID = ChangeMessageText(this->eMessageID, 
					(this->wstrText.size()) ? this->wstrText.c_str() : wszEmpty);
		else
			this->eMessageID = AddMessageText(
					(this->wstrText.size()) ? this->wstrText.c_str() : wszEmpty);
		Language::SetLanguage(curLanguage);
		this->bIsDirty = false;
	}
	return this->eMessageID;
}

//*****************************************************************************
void CDbMessageText::Delete()
//Deletes message text permanently from the DB.
{
	if (eMessageID != UNBOUND_MESSAGE)
		DeleteMessage(this->eMessageID);
}

//*******************************************************************************
bool CDbMessageText::Exists(const UINT dwID) const
{
	c4_View view;
	return LookupRowByPrimaryKey(dwID, V_MessageTexts, view) != ROW_NO_MATCH;
}

//*****************************************************************************
void CDbMessageText::ExportText(CStretchyBuffer& buf, const char* pTagName)
//Outputs message (language) texts for this message ID as XML tags.
{
	ASSERT(pTagName);

	const Language::LANGUAGE eLanguage = Language::GetLanguage();
	CCoordSet ids(GetMessageTextIDs(this->eMessageID));
	bool bActiveLanguageFound = false;
	for (CCoordSet::const_iterator id = ids.begin(); id != ids.end(); ++id)
	{
		//Append XML fields for each message text for this messageID.
		const UINT wLanguageIndex = id->wY;
		if (!wLanguageIndex || wLanguageIndex >= Language::LanguageCount)
			continue; //not a valid language ID -- skip it

		buf += "<";
		buf += pTagName;

		if (eLanguage == wLanguageIndex)
			bActiveLanguageFound = true;
		buf += " lang='";
		buf += Language::GetCode(wLanguageIndex);

		BYTE* pbOutStr = NULL;
		c4_Bytes MessageTextBytes = p_MessageText(GetRowRef(V_MessageTexts, id->wX));
		const WCHAR *pText = GetMessageText(MessageTextBytes);
		const UINT wSize = to_utf8(pText, pbOutStr);
		buf += "' text=\"";
		if (wSize)
		{
			//Replace characters that are illegal in XML strings with entities.
			string outstr;
			for (UINT wIndex = 0; wIndex < wSize; ++wIndex)
				switch (pbOutStr[wIndex])
				{
					case 10:  outstr += "&#10;"; break;
					case 13:  outstr += "&#13;"; break;
					case '"': outstr += "&quot;"; break;
					case '<': outstr += "&lt;"; break;
					case '>': outstr += "&gt;"; break;
					case '&': outstr += "&amp;"; break;
					case '\'': //allow apostrophes in text
					default: outstr += pbOutStr[wIndex]; break;
				}
			buf.Append((const BYTE*)outstr.c_str(), outstr.size());
		}
		delete[] pbOutStr;

		buf += "\"/>" NEWLINE;
	}

	//Add an empty language tag for the active language when texts for other
	//languages, but not this one, are found.
	if (!bActiveLanguageFound && !ids.empty())
	{
		buf += "<";
		buf += pTagName;
		buf += " lang='";
		buf += Language::GetCode(eLanguage);
		buf += "' text=\"\"/>" NEWLINE;
	}
}

//*****************************************************************************
void CDbMessageText::ImportText(const char** atts)
//Add a new message text for this language if one does not exist.
{
	//There must be exactly two attribute-value pairs, 'lang' and 'text'.
	if (!atts[0] || !atts[1] || !atts[2] || !atts[3] || atts[4]) return;
	if (strcmp("lang", atts[0]) != 0) return;
	if (strcmp("text", atts[2]) != 0) return;
	if (strlen(atts[3]) == 0) return; //empty text -- do nothing

	//Look whether this message exists in the specified language.
	const Language::LANGUAGE oldLanguage = Language::GetLanguage(), eLanguage = Language::Get(atts[1]);
	Language::SetLanguage(eLanguage);
	const UINT dwFoundRowI = FindMessageText(this->eMessageID, false);
	if (dwFoundRowI != ROW_NO_MATCH)
	{
		//message text already exists -- don't overwrite
		Language::SetLanguage(oldLanguage);
		return;
	}

	ASSERT(!this->bIsDirty); //don't lose any updates to a text in memory

	//Add new language version of this message.
	UTF8ToUnicode(atts[3], strlen(atts[3]), this->wstrText);
	AddMessageText(this->eMessageID, this->wstrText.c_str());
	this->bIsLoaded = true;

	Language::SetLanguage(oldLanguage);
}

//*****************************************************************************
void CDbMessageText::Load()
//Loads text string from the DB, if needed.
{
	if (!this->bIsLoaded && this->eMessageID != UNBOUND_MESSAGE)
	{
		const Language::LANGUAGE curLanguage = Language::GetLanguage();
		if (this->bUseDefaultLanguage)
			Language::SetLanguage(Language::English);
		const WCHAR *pwczText = GetMessageText(this->eMessageID);
		Language::SetLanguage(curLanguage);

		this->wstrText = (pwczText != NULL) ? pwczText : wszEmpty;
		ASSERT(pwczText); //Probably bad binding if fires.
		this->bIsLoaded = true;
	}
}

//*****************************************************************************
const WCHAR * CDbMessageText::operator = (
//Assign a new string.
//
//Params:
	const WCHAR * wczSet) //(in)  New string.
//
//Returns:
//Const pointer to string.
{
	this->wstrText = (wczSet ? wczSet : wszEmpty);
	this->bIsDirty = true;
	this->bIsLoaded = true; //In other words, if we weren't loaded before, it
											//doesn't matter now, because new value will overwrite.

	return (this->wstrText.size()) ? this->wstrText.c_str() : wszEmpty;
}

//*****************************************************************************
const WCHAR * CDbMessageText::operator = (
//Assign a new string.
//
//Params:
	const CDbMessageText &text) //(in)  Make copy of this text.
{
	WCHAR const* pwczText = NULL;
	if (text.bIsLoaded)
	{
		pwczText = (const WCHAR*)text;
		this->bIsDirty = true;
	} else if (text.eMessageID != UNBOUND_MESSAGE) {
		const Language::LANGUAGE curLanguage = Language::GetLanguage();
		if (this->bUseDefaultLanguage)
			Language::SetLanguage(Language::English);
		pwczText = GetMessageText(text.eMessageID);
		Language::SetLanguage(curLanguage);

		//If this text is already bound to the same ID as 'text' has,
		//then retrieve the text but don't set the dirty bit on this object.
		//Otherwise, we'd be resaving an unchanged text to the DB on UpdateText.
		this->bIsDirty = text.eMessageID != this->eMessageID;
	}

	this->wstrText = pwczText ? pwczText : wszEmpty;
	this->bIsLoaded = true; //In other words, if we weren't loaded before, it
	                        //doesn't matter now, because new value will overwrite.
	return this->wstrText.size() ? this->wstrText.c_str() : wszEmpty;
}

//*****************************************************************************
const WCHAR * CDbMessageText::operator += (
//Append a string to end.
//
//Params:
	const WCHAR * wczSet) //(in)  String to append to current.
//
//Returns:
//Const pointer to string.
{
	Load();

	this->wstrText += wczSet;
	this->bIsDirty = true;

	return (this->wstrText.size()) ? this->wstrText.c_str() : wszEmpty;
}

//*****************************************************************************
bool CDbMessageText::operator == (
//Compare two strings.
//
//Params:
	CDbMessageText &text)   //(in)
//NOTE: This method cannot be const!
{
	return (WCScmp((const WCHAR *)*this, (const WCHAR *)text) == 0);
}

//*****************************************************************************
bool CDbMessageText::operator == (const WSTRING &text)
//Compare two strings.
//
//NOTE: This method cannot be const!
{
	return (WCScmp((const WCHAR *)*this, text.c_str()) == 0);
}

//*****************************************************************************
bool CDbMessageText::operator == (const WCHAR* text)
//Compare two strings.
//
//NOTE: This method cannot be const!
{
	return (WCScmp((const WCHAR *)*this, text) == 0);
}


//*****************************************************************************
bool CDbMessageText::operator != (const WCHAR* text)
//Compare two strings.
//
//NOTE: This method cannot be const!
{
	return (WCScmp((const WCHAR *)*this, text) != 0);
}

//*****************************************************************************
CDbMessageText::operator const WCHAR *()
//Returns const string, loading from database if needed.
{
	Load();
	return (this->wstrText.size()) ? this->wstrText.c_str() : wszEmpty;
}

//*****************************************************************************
CDbMessageText::operator const WCHAR *() const
//Returns const string.  No loading from DB.
{
	ASSERT(this->bIsLoaded);
	return (this->wstrText.size()) ? this->wstrText.c_str() : wszEmpty;
}

