/*
* FinTP - Financial Transactions Processing Application
* Copyright (C) 2013 Business Information Systems (Allevo) S.R.L.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>
* or contact Allevo at : 031281 Bucuresti, 23C Calea Vitan, Romania,
* phone +40212554577, office@allevo.ro <mailto:office@allevo.ro>, www.allevo.ro.
*/

#ifndef HMAC_H
#define HMAC_H

#include <string>
#include <vector>
#include "../DllMain.h"

namespace FinTP
{
	class ExportedObject HMAC
	{
	public:
		enum digest_encoding{
				BASE64,
				HEXSTRING,
				HEXSTRING_UPPERCASE
		 };

		 static string encode( const vector< unsigned char>& inData, digest_encoding encoding );

		 static string Sha256( const unsigned char* data, size_t dataSize, digest_encoding encoding );
		 static string Sha256( const string& data, digest_encoding encoding );
		 static vector<unsigned char> Sha256( const unsigned char* inData, size_t dataSize );

		 static vector<unsigned char> HMAC_Sha256Gen( const string& data, const string& key );
		 static vector<unsigned char> HMAC_Sha256Gen( const unsigned char* inData, size_t dataSize, const unsigned char* key, size_t keySize );
		 static string HMAC_Sha256Gen( const unsigned char* inData, size_t dataSize, const unsigned char* key, size_t keySize, digest_encoding encoding );
		 static string HMAC_Sha256Gen( const string& data, const string& key, digest_encoding encoding );		 
	};
}

#endif
