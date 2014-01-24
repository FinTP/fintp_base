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

#include <string>
#include <sstream>
#include <iomanip>

extern "C"
{
#include <openssl/evp.h>
}

using namespace std;

string md5( const string& input  )
{
	unsigned char out[16];
	unsigned int hashLength;
	EVP_MD_CTX ctx;

	EVP_MD_CTX_init( &ctx );
	EVP_DigestInit_ex( &ctx, EVP_md5(), NULL );
	EVP_DigestUpdate( &ctx, input.c_str(), input.size() );
	EVP_DigestFinal_ex( &ctx, out, &hashLength );
	EVP_MD_CTX_cleanup( &ctx );

	stringstream hexString;
	for( unsigned int i = 0; i < hashLength; ++i )
		hexString << hex << setfill('0') << setw(2) << (unsigned int)out[i];

	return hexString.str();
}
