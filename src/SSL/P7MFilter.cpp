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

#include "P7MFilter.h"

#include "XmlUtil.h"
#include "../XPathHelper.h"
#include "Trace.h"
#include "AppExceptions.h"
#include "Base64.h"

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
//#include <strstream>
#include <cassert>

//#include <xercesc/framework/LocalFileInputSource.hpp>
//#include <xercesc/dom/DOMWriter.hpp>
//#include <xercesc/framework/MemBufFormatTarget.hpp>

//#include <xalanc/XPath/NodeRefList.hpp>
#include <xalanc/PlatformSupport/XalanOutputStreamPrintWriter.hpp>
#include <xalanc/PlatformSupport/XalanStdOutputStream.hpp>
#include <xalanc/XMLSupport/FormatterToXML.hpp>
#include <xalanc/XMLSupport/FormatterTreeWalker.hpp>
#include <xalanc/XercesParserLiaison/XercesDocumentWrapper.hpp>

#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#define BUF_MAX_SIZE ( 4*1024 )

using namespace std;
using namespace FinTP;

const string P7MFilter::P7MCERTFILENAME = "P7MCertFileName";
const string P7MFilter::P7MCERTPASSWD = "P7MCertPasswd";

P7MFilter::P7MFilter() : AbstractFilter( FilterType::P7M ), m_P7( NULL ), m_SI( ), m_Data( NULL ), m_P7bio( NULL ), 
	m_sk( ), m_PKey( ), m_X509( ), m_Pkcs12( NULL ), m_CertFileName( "" ), m_CertPasswd( "" )
{
	
}

P7MFilter::~P7MFilter()
{	
}

AbstractFilter::FilterResult P7MFilter::ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	DEBUG( "Method buffer to buffer ");
	ManagedBuffer* inputBuffer = inputData.get();
	
	int errCode;

	ValidateProperties();
	
	BIO *bp = BIO_new(BIO_s_mem());
	
	try
	{
		if( asClient )
		// Client - get data with in base64 format , verify using ssl, and extract original message
		{
			//verify
#ifndef	OPENSSL_NO_MD2
			EVP_add_digest(EVP_md2());
#endif
#ifndef	OPENSSL_NO_MD5
			EVP_add_digest(EVP_md5());
#endif
#ifndef	OPENSSL_NO_SHA1
			EVP_add_digest(EVP_sha1());
#endif
#ifndef	OPENSSL_NO_MDC2
			EVP_add_digest(EVP_mdc2());
#endif

			// clear error stack
			ERR_clear_error();

			string formatedData( (char* )inputBuffer->buffer(), 0, inputBuffer->size() );
			int formatedSize = formatedData.length();
			
			char* c_string;
			
			c_string = new char[ formatedSize + 1 ];
			
			formatedData.copy( c_string, formatedSize );
			c_string[ formatedSize ] = 0;
			
			DEBUG2( "Message before design is : [" << c_string << "]" );
			
			m_Data = BIO_new_mem_buf( c_string , -1);
			
			m_P7 = PKCS7_new();
			
			DEBUG( "Ready to start verify process " );
			m_P7 = P7MFilter::B64_read_PKCS7(m_Data);
			//m_P7 = PEM_read_bio_PKCS7( m_Data, NULL, NULL, NULL )
			if( m_P7 == NULL )
			{
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "Error	while reading pkcs7	object from	BIO format : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			} 
			
			delete[] c_string;

			// clear error stack
			ERR_clear_error();
			
			m_P7bio = NULL;
			
			if( PKCS7_is_detached( m_P7 ) ) 
			{
				throw runtime_error( "PKCS7 is detached and we don't suport this for the moment" );
			}
			else
			{
				m_P7bio=PKCS7_dataInit( m_P7, NULL );
			}
			
			//read original mesaje from pkcs7 object

			string outputBuffer = ReadDataFromBIO( m_P7bio );						
			DEBUG2("Current message : [" << outputBuffer << "]" );
			
			outputData.get()->copyFrom( outputBuffer );
			
			// We can now verify signatures	
			m_sk=PKCS7_get_signer_info( m_P7 );
			if ( m_sk == NULL )
			{
				throw runtime_error( "There are no signatures on this data " );
			}
	
			/* Ok, first we	need to, for each subject entry, see if	we can verify */
			for	(int i=0; i< sk_PKCS7_SIGNER_INFO_num( m_sk ); i++)
			{
				//ASN1_UTCTIME *tm;
				//char *str1,*str2;
				
				//get signature info
				m_SI = sk_PKCS7_SIGNER_INFO_value( m_sk, i );
				m_X509 = PKCS7_cert_from_signer_info( m_P7, m_SI );
				
				//do the verify 
				errCode = PKCS7_signatureVerify( m_P7bio, m_P7, m_SI, m_X509 );
				
				//rc=PKCS7_dataVerify( m_CertStore, &m_CertCtx, m_P7bio, m_P7, m_SI );

				if( errCode <= 0 )
				{
					SSL_load_error_strings();
					ERR_load_crypto_strings();

					stringstream errorMessage;
					errorMessage << "Error	while verify data with code : [" 
						<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
					TRACE( errorMessage.str() );
					throw runtime_error( errorMessage.str() );
				}

				//signer info
		/*		if ((tm=get_signed_time(si)) !=	NULL)
				{
					BIO_printf(bio_out,"Signed time:");
					ASN1_UTCTIME_print(bio_out,tm);
					ASN1_UTCTIME_free(tm);
					BIO_printf(bio_out,"\n");
				}
				if (get_signed_seq2string(si,&str1,&str2))
				{
					BIO_printf(bio_out,"String 1 is	%s\n",str1);
					BIO_printf(bio_out,"String 2 is	%s\n",str2);
				}
*/
			}
			
			BIO_free_all( bp );
			BIO_free_all( m_Data );
			BIO_free_all( m_P7bio );
			
			PKCS7_free( m_P7 );
			//X509_STORE_free( m_CertStore );
			
		}
		else
		// Server - 
		{
#ifndef OPENSSL_NO_MD2
			EVP_add_digest(EVP_md2());
#endif
#ifndef OPENSSL_NO_MD5
			EVP_add_digest(EVP_md5());
#endif
#ifndef OPENSSL_NO_SHA1
			EVP_add_digest(EVP_sha1());
#endif
#ifndef OPENSSL_NO_MDC2
			EVP_add_digest(EVP_mdc2());
#endif
			m_Data = BIO_new_mem_buf( ( char* )inputBuffer->buffer(), inputBuffer->size() );
			
			BIO* inputBioFile;
			DEBUG( "Ready to start sign process " );
			if( ( inputBioFile = BIO_new_file( m_CertFileName.c_str() ,"r") ) == NULL ) 
			{	
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "Error while open file for read public key : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}

			m_Pkcs12 = PKCS12_init( NID_pkcs7_data );
			
			d2i_PKCS12_bio( inputBioFile, &m_Pkcs12 );
			
			BIO_free( inputBioFile );

			SSLeay_add_all_algorithms();
			
			m_X509 = X509_new();
			m_PKey = EVP_PKEY_new();
			
			errCode = PKCS12_parse( m_Pkcs12, m_CertPasswd.c_str(), &m_PKey, &m_X509, NULL );
			if( errCode <= 0 )
			{
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "Error	while parse pkcs12 object  : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
						
			m_P7 = PKCS7_new();
			
			errCode = PKCS7_set_type( m_P7, NID_pkcs7_signed );
			if( !errCode )
			{
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "Error	while setting type  : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
			
			m_SI = PKCS7_add_signature( m_P7, m_X509, m_PKey, EVP_sha1() );
			
			if( m_SI == NULL )
			{	
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "Error while adding signature to pkcs7 object : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
			
			// If you do this then you get signing time automatically added
			PKCS7_add_signed_attribute( m_SI, NID_pkcs9_contentType, V_ASN1_OBJECT, OBJ_nid2obj(NID_pkcs7_data));
			
			// we may want to add more 
			PKCS7_add_certificate( m_P7, m_X509 );
			
			// Set the content of the signed to 'data' 
			PKCS7_content_new( m_P7, NID_pkcs7_data);

			/*if (!nodetach)
				PKCS7_set_detached(p7,1);
			*/
	
			if( ( m_P7bio = PKCS7_dataInit( m_P7, NULL ) ) == NULL ) 
			{	
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "Error while data initialization to pkcs7 object : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
			
			char buffer[ BUF_MAX_SIZE ];
			
			for (;;)
			{
				int i = BIO_read( m_Data, buffer, sizeof( buffer ));
				if (i <= 0) break;
				BIO_write( m_P7bio, buffer, i );
			}
			
			errCode = PKCS7_dataFinal( m_P7, m_P7bio );
			if( !errCode ) 
			{	
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "Error while data terminating process : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
			
			errCode = B64_write_PKCS7( bp, m_P7 );

			if( errCode <= 0 )
			{
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "Error while writing data to bio buffer : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}

			string outputBuffer = ReadDataFromBIO( bp );
			
			BIO_free_all( bp );
			BIO_free_all( m_Data );
			BIO_free_all( m_P7bio );
			
			PKCS7_free( m_P7 );
			
			DEBUG("Current message : [" << outputBuffer << "]" );
			
			outputBuffer.erase( 0, 22 );
			outputBuffer.erase( outputBuffer.end() - 21 , outputBuffer.end() );
			
			//TODO delete header and footer
			outputData.get()->copyFrom( outputBuffer );
				
		}
		//=================================================
	}
	catch( const std::exception& e )
	{
			BIO_free_all( bp );
			BIO_free_all( m_Data );
			BIO_free_all( m_P7bio );
			
			PKCS7_free( m_P7 );
			
		stringstream messageBuffer;
		messageBuffer << typeid( e ).name() << " exception [" << e.what() << "]";

		TRACE( messageBuffer.str() );	
		throw;
	}
	catch( ... )
	{  	
			BIO_free_all( bp );
			BIO_free_all( m_Data );
			BIO_free_all( m_P7bio );
			
			PKCS7_free( m_P7 );
			
		TRACE( "Unknown exception while processing message in P7MFilter filter" );
		throw;
	}

	return AbstractFilter::Completed;	

}	

AbstractFilter::FilterResult P7MFilter::ProcessMessage(AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	if( asClient )
	{
		DEBUG( "Method BufferToXml ");
		ManagedBuffer* outputBuffer = new ManagedBuffer();
		AbstractFilter::buffer_type outputDataBuffer( outputBuffer );

		AbstractFilter::FilterResult result = ProcessMessage( inputData, outputDataBuffer, transportHeaders, asClient );
		
		// get dom implementation
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* tempDoc = NULL;
		try		
		{
			DEBUG( "Deserializing document" );
			if ( outputBuffer == NULL )
				throw runtime_error( "Empty document" );

			/*DEBUG2("Message to deserialize : [" << outputBuffer->str() <<"]");
			
			tempDoc = XmlUtil::DeserializeFromString( outputBuffer->buffer(), outputBuffer->size() );
			DEBUG( "Deserialization OK" );*/

			if ( outputData == NULL )
				throw logic_error( "You must supply a valid DOMDocument to ProcessMessage" );
				
			//create document
			DOMElement* element = outputData->createElement( unicodeForm( "TIFF" ) );
			DEBUG( "TIFF elem. created." );

			string encodedProdData = Base64::encode( outputBuffer->buffer(), outputBuffer->size() );
			DEBUG( "TIFF in Base64 format : [" << encodedProdData << "]" );

			DOMText* prodDataVal = outputData->createTextNode( unicodeForm( encodedProdData ) );
			DEBUG( "TextNode with encoded value created" );

			element->appendChild( prodDataVal );
			DEBUG( "TextNode appended" );

			outputData->getDocumentElement()->appendChild( element );
			DEBUG( "Original node appended" );
		}
		catch( const XMLException& e )
		{
			stringstream errorMessage;
			
			errorMessage << "XmlException during extracting tiff from p7m. Type : " << ( int )e.getErrorType() << 
				" Code : [" << e.getCode() << "] Message = " << localForm( e.getMessage() ) <<
				" Line : " << e.getSrcLine();

			throw runtime_error( errorMessage.str() );
		}
		catch( ... )
		{
			
			//release document
			if( tempDoc != NULL )
				tempDoc->release();
			
			throw;
		}
		
		//release document
		if( tempDoc != NULL )
			tempDoc->release();
									
		return result;
		
	}
	else
	{
		throw FilterInvalidMethod( AbstractFilter::BufferToXml );
	}
}

bool P7MFilter::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient )
{
	if( asClient )
		switch( method )
		{
		case AbstractFilter::BufferToXml :
			return true;
		case AbstractFilter::BufferToBuffer :
			return true;
		default:
			return false;
		}
	else 
		switch( method )
		{
			case AbstractFilter::XmlToBuffer :
				return true;
			case AbstractFilter::BufferToBuffer :
				return true;
			default:
				return false;
		}
}

void P7MFilter::ValidateProperties()
{
	if ( m_Properties.ContainsKey( P7MFilter::P7MCERTFILENAME ) )
	{
		m_CertFileName = m_Properties[ P7MFilter::P7MCERTFILENAME ];
		if ( m_Properties.ContainsKey( P7MFilter::P7MCERTPASSWD ) )
			m_CertPasswd = m_Properties[ P7MFilter::P7MCERTPASSWD ];
		else
			throw invalid_argument( "Required parameter missing : P7MCERTPASSWD" ); 
	}
	else  m_CertFileName ="";
}

string P7MFilter::ReadDataFromBIO( BIO* bp )
{
	char c_buf[ BUF_MAX_SIZE ];
	string buffer = "";
	bool moreToRead = true;
	int i=0;
	while( moreToRead )
	{
		i=BIO_read( bp, c_buf, sizeof( c_buf ) );
	
		if ( i>0 )
		{
			if( i< BUF_MAX_SIZE )
			{
				buffer.append( string( c_buf, i ) );
				moreToRead = false;
			}
			else
			{
				buffer.append( string( c_buf,i ) );
			    moreToRead = true;
			}		    
		}
		else
		{
			moreToRead = false;
		}
	}
	return buffer;
}

bool P7MFilter::IsSigned( const string& signedString )
{
	bool returnValue = false;
	char* c_string = NULL;
	PKCS7* p7 = NULL;
	BIO* data = NULL;
	try
	{
#ifndef	OPENSSL_NO_MD2
			EVP_add_digest(EVP_md2());
#endif
#ifndef	OPENSSL_NO_MD5
			EVP_add_digest(EVP_md5());
#endif
#ifndef	OPENSSL_NO_SHA1
			EVP_add_digest(EVP_sha1());
#endif
#ifndef	OPENSSL_NO_MDC2
			EVP_add_digest(EVP_mdc2());
#endif

			// clear error stack
			ERR_clear_error();

			string formatedData = signedString;
			int formatedSize = formatedData.length();
			
			c_string = new char[ formatedSize + 1 ];
			
			formatedData.copy( c_string, formatedSize );
			c_string[ formatedSize ] = '\0';
			
			DEBUG( "Message before design is : [" << c_string << "]" );
			
			
			data = BIO_new_mem_buf( c_string , -1);
			
			p7 = PKCS7_new();
			
			DEBUG( "Ready to start verify process " );
			p7 = B64_read_PKCS7( data );
			if( p7 == NULL)
			{
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "Error	while reading pkcs7	object from	BIO format : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			} 
			if( PKCS7_type_is_signed( p7 ) )
				returnValue = true;
			else returnValue = false;
			
			if( c_string != NULL )
			{
				delete[] c_string;
				c_string = NULL;
			}
			BIO_free_all( data );
				
			PKCS7_free( p7 );
	}
	catch( ... )
	{
		if( c_string != NULL )
		{
			delete[] c_string;
			c_string = NULL;
		}
		BIO_free_all( data );
			
		PKCS7_free( p7 );

		TRACE( "Unknown exception while reading message from pkcs7 format, may not be signed message" );
		throw;
	}

	return returnValue;
}

int P7MFilter::Verify_Callback(int	ok,	X509_STORE_CTX *ctx )
{
	char buf[256];
	X509 *err_cert;
	int	err,depth;

	err_cert=X509_STORE_CTX_get_current_cert(ctx);
	err=	X509_STORE_CTX_get_error(ctx);
	depth=	X509_STORE_CTX_get_error_depth(ctx);

	X509_NAME_oneline( X509_get_subject_name( err_cert), buf, 256);

	TRACE( "depth= [" << depth << "] and buffer= [" << buf << "]" );

	if (!ok)
		TRACE( "verify error:num= [" << err << "] and error string= [" << X509_verify_cert_error_string(err) << "]" );

	switch (ctx->error)
	{
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
			X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert),buf,256);
			TRACE( "issuer=	" << buf ) ;
			break;
		case X509_V_ERR_CERT_NOT_YET_VALID:
		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
			TRACE( "notBefore=" );
			//ASN1_UTCTIME_print(bio_err,X509_get_notBefore(ctx->current_cert));
			break;
		case X509_V_ERR_CERT_HAS_EXPIRED:
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
			TRACE( "notAfter=" );
			//ASN1_UTCTIME_print(bio_err,X509_get_notAfter(ctx->current_cert));
			break;

		default:
			break;
	}

	return(ok);
}

PKCS7* P7MFilter::B64_read_PKCS7( BIO* bio )
{
	BIO *b64;
	PKCS7 *p7;
	if(!(b64 = BIO_new(BIO_f_base64()))) 
	{
		PKCS7err(PKCS7_F_B64_READ_PKCS7,ERR_R_MALLOC_FAILURE);
		return 0;
	}
	bio = BIO_push(b64, bio);
	if(!(p7 = d2i_PKCS7_bio(bio, NULL))) 
		PKCS7err(PKCS7_F_B64_READ_PKCS7,PKCS7_R_DECODE_ERROR);
	BIO_flush(bio);
	bio = BIO_pop(bio);
	BIO_free(b64);
	return p7; 
}

int P7MFilter::B64_write_PKCS7(BIO *bio, PKCS7 *p7)
{
	BIO *b64;
	if(!(b64 = BIO_new(BIO_f_base64()))) {
		PKCS7err(PKCS7_F_B64_WRITE_PKCS7,ERR_R_MALLOC_FAILURE);
		return 0;
	}
	bio = BIO_push(b64, bio);
	i2d_PKCS7_bio(bio, p7);
	BIO_flush(bio);
	bio = BIO_pop(bio);
	BIO_free(b64);
	return 1;
} 
