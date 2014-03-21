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

#include "Base64.h"
#include "SSL/HMAC.h"
#include "StringUtil.h"
#include "Trace.h"

#include "SwiftFormatFilter.h"

namespace FinTP
{

const string SwiftFormatFilter::SERVICE_KEY = "SERVICE";
const string SwiftFormatFilter::LAU_KEY = "LAUKEY";

const string SwiftFormatFilter::SAA_FILEACT = "SAA_FILEACT";
const string SwiftFormatFilter::SAA_FIN = "SAA_FIN";
const string SwiftFormatFilter::SAE = "SAE";


bool SwiftFormatFilter::isMethodSupported( FilterMethod method, bool asClient )
{
	switch ( method )
	{
	case SwiftFormatFilter::BufferToBuffer :
		return true;
	default:
		return false;
	}
}

/*
The application and Alliance Lite2 exchange PDUs that are sequences of bytes with the following structure:
* Prefix (1 byte)
* Length (6 bytes)
* Signature (24 bytes)
* DataPDU: XML structure containing the information relevant for processing (message or report) encoded in UTF-8 format.DataPDU: XML structure containing the information relevant for processing (message or report) encoded in UTF-8 format.
*/

#define PREFIX_SIZE		1
#define LENGTH_SIZE		6
#define SIGNATURE_SIZE	24
#define HEADER_SIZE		31

bool SwiftFormatFilter::isStrictFormat( const ManagedBuffer& managedBuffer )
{
	unsigned char* rawBuffer = managedBuffer.buffer();
	size_t size = managedBuffer.size();
	return ( rawBuffer[0] == 0x1F && size > HEADER_SIZE && rawBuffer[HEADER_SIZE] == '<' && rawBuffer[size-1] == '>' );
}

AbstractFilter::FilterResult SwiftFormatFilter::ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	ManagedBuffer *inputBuffer = inputData.get(), *outputBuffer = outputData.get();
	string service = transportHeaders[SwiftFormatFilter::SERVICE_KEY];
	if ( inputBuffer && outputBuffer )
	{
		string key = transportHeaders[SwiftFormatFilter::LAU_KEY];

		DEBUG( "Applying SwiftFormatFilter filter with service: " << service );
		//TODO: Another design for this if loaded implementation
		if( service == SwiftFormatFilter::SAA_FIN )
		{
			bool authenticated = true;
			string errorMessage;
			string SIGN_SECTION_BEGIN = "{MDG:";
			string SIGN_SECTION_END = "}}";
			string MESSAGE_SECTION_END = "{S:";
			if ( asClient )
			{
				string inputString = inputBuffer->str();
				size_t signSectionPos = inputString.find( SIGN_SECTION_BEGIN );
				size_t messageSectionEndPos = inputString.find( MESSAGE_SECTION_END );
				if( signSectionPos != string::npos )//signed message
				{
					if ( key.empty() )
					{
						errorMessage = "Signed message processed, but no key provided in .config file";
						authenticated = false;
						TRACE( errorMessage );

					}
					inputString = inputString.substr( 0, messageSectionEndPos );
					if( HMAC::HMAC_Sha256Gen( inputString, key, HMAC::HEXSTRING_UPPERCASE ) != ( inputBuffer->str() ).substr( signSectionPos + 5, 64 ) )
					{
						errorMessage = "Message not authenticated";
						authenticated = false;
						TRACE( errorMessage );
					}
				}
				else
				{
					if( !key.empty() )
					{
						errorMessage = "Signed message expected (check MQSeriesToApp.LAUCertificateFile key in .config file ). Message not authenticated";
						authenticated = false;
						TRACE( errorMessage );
					}
				}

				if ( !authenticated )
				{
					errorMessage = "Message not authenticated";
					throw runtime_error( errorMessage );
				}
				outputBuffer->copyFrom( inputString );
			}
			else
			{
				const string& inputString = inputBuffer->str();
				stringstream signedOutput;
				signedOutput << inputString << MESSAGE_SECTION_END << SIGN_SECTION_BEGIN << HMAC::HMAC_Sha256Gen( inputString, key, HMAC::HEXSTRING_UPPERCASE ) << SIGN_SECTION_END;
				outputBuffer->copyFrom( signedOutput.str() );
			}
		}
		else if( service == SwiftFormatFilter::SAA_FILEACT )
		{
			if ( asClient ) //delete 31 byte prefix
			{
				unsigned char* rawBuffer = inputBuffer->buffer();
				size_t size = inputBuffer->size();

				if ( isStrictFormat( *inputBuffer ) )
				{
					bool authenticated = true;

					if ( key.empty() )
					{
						for ( size_t i = 0; i < SIGNATURE_SIZE; ++i )
							if ( rawBuffer[i + PREFIX_SIZE + LENGTH_SIZE ] != 0 )
							{
								authenticated = false;
								break;
							}
					}
					else
					{
						//Signature (24 bytes): HMAC SHA-256 digest truncated to 128 bits Base64 encoded
						vector<unsigned char> digest = HMAC::HMAC_Sha256Gen( rawBuffer + HEADER_SIZE, size - HEADER_SIZE, reinterpret_cast<const unsigned char*>( key.c_str() ), key.size() );
						string base64Signature = Base64::encode( &digest[0], 16 ); // 16 bytes = 128 bits if CHAR_BIT = 8

						if ( memcmp( base64Signature.c_str(), rawBuffer + PREFIX_SIZE + LENGTH_SIZE, SIGNATURE_SIZE ) != 0 )
							authenticated = false;
					}

					if ( !authenticated )
						throw runtime_error( "Message not authenticated" );

					string messageSizeString( reinterpret_cast<char*>( rawBuffer ), PREFIX_SIZE, LENGTH_SIZE );
					size_t messageSize = StringUtil::ParseInt( messageSizeString );
					if ( messageSize == size - PREFIX_SIZE - LENGTH_SIZE )
					{
						size -= HEADER_SIZE;
						outputBuffer->allocate( size );
						memcpy( outputBuffer->buffer(), &rawBuffer[HEADER_SIZE], size );
					}
					else
						throw runtime_error( "Incorrect message size specified in message" );
				}
				else
					outputBuffer->copyFrom( inputBuffer );
			}
			else //add 31 byte prefix
			{
				size_t putMessageSize = inputBuffer->size() + HEADER_SIZE;
				outputBuffer->allocate( putMessageSize );
				unsigned char* putBuffer = outputBuffer->buffer();

				//Prefix (1 byte): the character '0x1f'.
				putBuffer[0] = 0x1f;

				//Length (6 bytes): length (in bytes) of the Signature and DataPDU fields: this length is base-10
				//encoded as 6 ASCII characters, left padded with 0s if needed.
				string sizePrefix = StringUtil::ToString( inputBuffer->size() + SIGNATURE_SIZE );
				size_t sizePrefixLength = sizePrefix.length();
				string leftZeroes;
				for ( int i = 0; i < LENGTH_SIZE - sizePrefixLength; ++i )
					leftZeroes += "0";
				sizePrefix = leftZeroes + sizePrefix;
				memcpy( &putBuffer[1], sizePrefix.c_str(), LENGTH_SIZE );

				if ( key.empty() )
					//Signature (24 bytes): null bytes
					memset( &putBuffer[PREFIX_SIZE + LENGTH_SIZE], 0, SIGNATURE_SIZE );
				else
				{
					//Signature (24 bytes): HMAC SHA-256 digest truncated to 128 bits Base64 encoded
					vector<unsigned char> digest = HMAC::HMAC_Sha256Gen( inputBuffer->buffer(), inputBuffer->size(), reinterpret_cast<const unsigned char*>( key.c_str() ), key.size() );
					string base64Signature = Base64::encode( &digest[0], 16 ); // 16 bytes = 128 bits if CHAR_BIT = 8
					memcpy( &putBuffer[PREFIX_SIZE + LENGTH_SIZE], base64Signature.c_str(), SIGNATURE_SIZE );
				}

				//DataPDU:
				memcpy( &putBuffer[HEADER_SIZE], inputBuffer->buffer(), inputBuffer->size() );
			}
		}
		return SwiftFormatFilter::Completed;
	}
	TRACE( "SwiftFormatFilter not applied!. Service: " <<  service )
	return SwiftFormatFilter::Completed;
}

AbstractFilter::FilterResult SwiftFormatFilter::ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
}

AbstractFilter::FilterResult SwiftFormatFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::XmlToXml );
}

AbstractFilter::FilterResult SwiftFormatFilter::ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
}

AbstractFilter::FilterResult SwiftFormatFilter::ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
}

AbstractFilter::FilterResult SwiftFormatFilter::ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::BufferToXml );
}

AbstractFilter::FilterResult SwiftFormatFilter::ProcessMessage( unsigned char* inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::BufferToXml );
}

}
