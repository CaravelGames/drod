// $Id$


//*********************************************************************
//* C_Base64 - a simple base64 encoder and decoder.
//*
//*     Copyright (c) 1999, Bob Withers - bwit@pobox.com
//*
//* This code may be freely used for any purpose, either personal
//* or commercial, provided the authors copyright notice remains
//* intact.
//*********************************************************************

#ifndef Base64_H
#define Base64_H

#include "Types.h"
#include "Wchar.h"

using std::string;

namespace Base64
{
	string encode(const string &data);
	string encode(const WSTRING &data);
	string encode(const unsigned char* data, const unsigned long dwDataSize);
	
	void decode(const string &data, string &returnvalue);
	void decode(const string &data, WSTRING &returnvalue);
	string decode(const string &data);
	unsigned long decode(const string &data, unsigned char* &returnvalue);
};

#endif
