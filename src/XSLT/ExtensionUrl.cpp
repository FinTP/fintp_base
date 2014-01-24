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

// Base header file.  Must be first.
#include <xalanc/Include/PlatformDefinitions.hpp>

#include <xercesc/util/PlatformUtils.hpp>
#include <xalanc/XalanTransformer/XalanTransformer.hpp>
#include <xalanc/XPath/XObjectFactory.hpp>
#include "ExtensionUrl.h"

#include <string>
#include <sstream>
#include "XmlUtil.h"
#include "Base64.h"
//#include "../Log/Trace.h"

using namespace std;
using namespace FinTP;
XALAN_CPP_NAMESPACE_USE

//
//  libcurl variables for error strings and returned data

char FunctionUrl::m_ErrorBuffer[ CURL_ERROR_SIZE ];
unsigned char** FunctionUrl::m_Buffer = NULL;
long FunctionUrl::m_BufferSize = 0;

//
//  libcurl variables for connection using ssl
string FunctionUrl::SSLCertificateFileName = "";
string FunctionUrl::SSLCertificatePasswd = "";

//
//  libcurl write callback function
//
int FunctionUrl::writer( char *data, size_t size, size_t nmemb, unsigned char** writerDataDummy )
{
	size_t index = 0;

	if ( m_Buffer == NULL )
		return 0;
		
	if ( *m_Buffer == NULL )
		*m_Buffer = new unsigned char[ size * nmemb ];
	else
	{
		unsigned char* newBuffer = new unsigned char[ m_BufferSize + size * nmemb ];
		memcpy( newBuffer, *m_Buffer, m_BufferSize );
		delete[] *m_Buffer;
		*m_Buffer = newBuffer;
		index = m_BufferSize;
	}

	memcpy( ( *m_Buffer + index ), data, size * nmemb );
	m_BufferSize += size * nmemb;
	
	return size * nmemb;
}

//
//  libcurl connection initialization
//
void FunctionUrl::initCurl( CURL *&conn, const string& url ) const
{
	CURLcode code;
	conn = curl_easy_init();

	if ( conn == NULL )
	{
		throw new runtime_error( "Failed to create CURL connection" );
	}

	code = curl_easy_setopt( conn, CURLOPT_ERRORBUFFER, m_ErrorBuffer );
	if ( code != CURLE_OK )
	{
		stringstream errorMessage;
		errorMessage << "Failed to set error buffer [" << code << "]";
		
		throw new runtime_error( errorMessage.str() );
	}

	code = curl_easy_setopt( conn, CURLOPT_URL, url.c_str() );
	if ( code != CURLE_OK )
	{
		stringstream errorMessage;
		errorMessage << "Failed to set URL code [" << code << "] error [" << m_ErrorBuffer << "]" ;
		
		throw new runtime_error( errorMessage.str() );
	}

	if ( SSLCertificatePasswd.length() > 0 )
	{
		
		code = curl_easy_setopt( conn, CURLOPT_KEYPASSWD, SSLCertificatePasswd.c_str() );
		if ( code != CURLE_OK )
		{
			stringstream errorMessage;
			errorMessage << "Failed to set the key password code [" << code << "] error [" << m_ErrorBuffer << "]" ;
		
			throw new runtime_error( errorMessage.str() );
		}
	}

	if ( SSLCertificateFileName.length() > 0 )
	{
		code = curl_easy_setopt( conn, CURLOPT_SSLCERT, SSLCertificateFileName.c_str() );
		if ( code != CURLE_OK )
		{
			stringstream errorMessage;
			errorMessage << "Failed to set the certificate name code [" << code << "] error [" << m_ErrorBuffer << "]" ;
		
			throw new runtime_error( errorMessage.str() );
		}
	
		code = curl_easy_setopt( conn, CURLOPT_SSL_VERIFYHOST, 2 );
		if ( code != CURLE_OK )
		{
			stringstream errorMessage;
			errorMessage << "Failed to set client verification code [" << code << "] error [" << m_ErrorBuffer << "]" ;
			
			throw new runtime_error( errorMessage.str() );
		}
	}
	else
	{
		code = curl_easy_setopt( conn, CURLOPT_SSL_VERIFYHOST, 0 );
		if ( code != CURLE_OK )
		{
			stringstream errorMessage;
			errorMessage << "Failed to stop to verify the host code [" << code << "] error [" << m_ErrorBuffer << "]" ;
			
			throw new runtime_error( errorMessage.str() );
		}
	}

	code = curl_easy_setopt( conn, CURLOPT_SSL_VERIFYPEER, 0 ); 
	if ( code != CURLE_OK )
	{
		stringstream errorMessage;
		errorMessage << "Failed to stop verifying the authenticity of the peer's certificate code [" << code << "] error [" << m_ErrorBuffer << "]" ;
	
		throw new runtime_error( errorMessage.str() );
	}

	code = curl_easy_setopt( conn, CURLOPT_FOLLOWLOCATION, 1L );
	if ( code != CURLE_OK )
	{
		stringstream errorMessage;
		errorMessage << "Failed to set redirect option [" << code << "] error [" << m_ErrorBuffer << "]" ;
		
		throw new runtime_error( errorMessage.str() );
	}

	code = curl_easy_setopt( conn, CURLOPT_WRITEFUNCTION, FunctionUrl::writer );
	if ( code != CURLE_OK )
	{
		stringstream errorMessage;
		errorMessage << "Failed to set writer [" << code << "] error [" << m_ErrorBuffer << "]" ;
		
		throw new runtime_error( errorMessage.str() );
	}

	m_Buffer = new unsigned char*;
	*m_Buffer = NULL;
	m_BufferSize = 0;

	code = curl_easy_setopt( conn, CURLOPT_WRITEDATA, NULL );
	if ( code != CURLE_OK )
	{
		stringstream errorMessage;
		errorMessage << "Failed to set write data [" << code << "] error [" << m_ErrorBuffer << "]" ;

		throw new runtime_error( errorMessage.str() );
	}
}

/**
* Execute an XPath function object.  The function must return a valid
* XObject.
*
* @param executionContext executing context
* @param context          current context node
* @param opPos            current op position
* @param args             vector of pointers to XObject arguments
* @return                 pointer to the result XObject
*/
XObjectPtr FunctionUrl::execute( XPathExecutionContext& executionContext, XalanNode* context, const XObjectArgVectorType& args, const LocatorType* locator ) const
{
	if ( args.size() != 1 )
	{
		stringstream errorMessage;
		errorMessage << "The Url() function takes one argument! [ url ], but received " << args.size();
#if (_XALAN_VERSION >= 11100)
		executionContext.problem( XPathExecutionContext::eXPath, XPathExecutionContext::eError, XalanDOMString( errorMessage.str().c_str() ), locator, context); 
#else
		executionContext.error( XalanDOMString( errorMessage.str().c_str() ), context );
#endif
	}

	string url = localForm( ( const XMLCh* )( args[ 0 ]->str().data() ) );

	CURL *conn = NULL;
	CURLcode code;

	m_BufferSize = 0;
	m_Buffer = NULL;

	string result = "";

	curl_global_init( CURL_GLOBAL_DEFAULT );

	// Initialize CURL connection
	try
	{
		initCurl( conn, url );
		
		// Retrieve content for the URL
		code = curl_easy_perform( conn );
		if ( code != CURLE_OK )
		{
			stringstream errorMessage;
			errorMessage << "Failed to get [" << url << "] code [" << code << "] error [" << m_ErrorBuffer << "]";
			//TRACE( errorMessage.str() );
			throw new runtime_error( errorMessage.str() );
		}

		curl_easy_cleanup( conn );

		if ( m_Buffer != NULL ) 
		{
			result = Base64::encode( *m_Buffer, m_BufferSize );

			if ( *m_Buffer != NULL )
				delete[] *m_Buffer;

			delete m_Buffer;
			m_Buffer = NULL;

			m_BufferSize = 0;
		}
	}
	catch( ... )
	{
		if ( conn != NULL )
			curl_easy_cleanup( conn );

		if ( m_Buffer != NULL ) 
		{
			if ( *m_Buffer != NULL )
				delete[] *m_Buffer;

			delete m_Buffer;
			m_Buffer = NULL;

			m_BufferSize = 0;
		}

		throw;
	}

	return executionContext.getXObjectFactory().createString( unicodeForm( result ) );
}

/**
* Implement clone() so Xalan can copy the function into
* its own function table.
*
* @return pointer to the new object
*/
// For compilers that do not support covariant return types,
// clone() must be declared to return the base type.
#ifdef XALAN_1_9
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionUrl*
#endif
FunctionUrl::clone( MemoryManagerType& theManager ) const
{
    return XalanCopyConstruct(theManager, *this);
}

const XalanDOMString& FunctionUrl::getError( XalanDOMString& theResult ) const
{
	theResult.assign( "The url function accepts only 1 argument [ url ]" );
	return theResult;
}
#else
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionUrl*
#endif
FunctionUrl::clone() const
{
	return new FunctionUrl( *this );
}

const XalanDOMString FunctionUrl::getError() const
{
	return StaticStringToDOMString( XALAN_STATIC_UCODE_STRING( "The url function accepts only 1 argument [ url ]" ) );
}
#endif
