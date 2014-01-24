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

#include <cstdio>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "HMAC.h"
#include "Trace.h"
#include "Base64.h"

using namespace FinTP;

string HMAC::encode( const vector< unsigned char>& digest, digest_encoding encoding )
{
	if ( encoding == HEXSTRING )
	{
		stringstream hexString;
		for( unsigned int i = 0; i < digest.size(); ++i )
			hexString << hex << setfill('0') << setw(2) << static_cast<unsigned int>(digest[i]);
		return hexString.str();
	}
	else if ( encoding == HEXSTRING_UPPERCASE )
	{
		stringstream hexString;
		for( unsigned int i = 0; i < digest.size(); ++i )
			hexString << hex << uppercase << setfill('0') << setw(2) << static_cast<unsigned int>(digest[i]);
		return hexString.str();
	}
	else 
		if ( encoding == BASE64 )
			return Base64::encode( &digest[0], digest.size() );

	throw runtime_error( "Invalid encoding" );
}

vector<unsigned char> HMAC::Sha256( const unsigned char* inData, size_t dataSize )
{
	vector< unsigned char> out;
	out.resize( SHA256_DIGEST_LENGTH );
	unsigned int hashLength;
	EVP_MD_CTX ctx;
	const EVP_MD *hashFunc = EVP_sha256();

	EVP_MD_CTX_init( &ctx );
	EVP_DigestInit_ex( &ctx, hashFunc, NULL );
	EVP_DigestUpdate( &ctx, inData, dataSize );
	EVP_DigestFinal_ex( &ctx, &out[0], &hashLength );
	EVP_MD_CTX_cleanup( &ctx );

	return out;
}

string HMAC::Sha256( const unsigned char* inData, size_t dataSize, digest_encoding encoding  )
{
	return encode( HMAC::Sha256( inData, dataSize ), encoding );
}

string HMAC::Sha256( const string& data, digest_encoding encoding )
{
	const unsigned char* unsignedInData = reinterpret_cast< const unsigned char* >( data.c_str() );

	return Sha256( unsignedInData, data.size(), encoding );
}

vector<unsigned char> HMAC::HMAC_Sha256Gen( const unsigned char* inData, size_t dataSize, const unsigned char* key, size_t keySize )
{
	unsigned int hashLength;
	unsigned int keyLen;
	vector< unsigned char> out;
	out.resize( SHA256_DIGEST_LENGTH );

	HMAC_CTX      ctx;

	HMAC_CTX_init( &ctx );
	HMAC_Init_ex( &ctx, key, keySize, EVP_sha256(  ), NULL );
	HMAC_Update( &ctx, inData, dataSize );
	HMAC_Final( &ctx, &out[0], &hashLength );

	HMAC_cleanup(&ctx);

	return out;
}

vector<unsigned char> HMAC::HMAC_Sha256Gen( const string& inData, const string& key )
{   
	const unsigned char* unsignedInData = reinterpret_cast< const unsigned char* >( inData.c_str() );
	const unsigned char* unsignedKey = reinterpret_cast< const unsigned char* >( key.c_str() );
   		
	return HMAC_Sha256Gen( unsignedInData, inData.size(), unsignedKey, key.size() );
}

string HMAC::HMAC_Sha256Gen( const unsigned char* inData, size_t dataSize, const unsigned char* key, size_t keySize, digest_encoding encoding )
{
	return encode( HMAC_Sha256Gen( inData, dataSize, key, keySize ), encoding );
}

string HMAC::HMAC_Sha256Gen( const string& data, const string& key, digest_encoding encoding )
{
	return encode( HMAC_Sha256Gen( data, key ), encoding );
}