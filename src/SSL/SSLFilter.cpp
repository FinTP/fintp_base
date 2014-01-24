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

#include "SSLFilter.h"

#include "XmlUtil.h"
#include "../XPathHelper.h"
#include "Trace.h"
#include "AppExceptions.h"
#include "Base64.h"
#include "PlatformDeps.h"

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

const string SSLFilter::SSLCERTFILENAME = "SSLCertFileName";
const string SSLFilter::SSLCERTPASSWD = "SSLCertPasswd";
pthread_once_t SSLFilter::OnceInitControl = PTHREAD_ONCE_INIT;
pthread_mutex_t* SSLFilter::SSLSyncMutex = NULL;

SSLFilter::SSLFilter() : AbstractFilter( FilterType::SSL ), m_CertFileName( "" ), m_CertPasswd( "" )
{	
	int onceResult = pthread_once( &SSLFilter::OnceInitControl, &SSLFilter::Initialize );
	if ( 0 != onceResult )
	{
		TRACE( "Thread once initialization for SSL filter failed [" << onceResult << "]" );
	}
}

SSLFilter::~SSLFilter()
{	
}

AbstractFilter::FilterResult SSLFilter::ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	DEBUG( "Method buffer to buffer ");
	ManagedBuffer* inputBuffer = inputData.get();
		
	/// \brief PKCS#7 data in PEM format 
	PKCS7* pkcs7Data = NULL; 
	/// \brief PKCS#7 signer information
	PKCS7_SIGNER_INFO *signerInfo = NULL;
	/// \brief I/O abstraction
	BIO	*bioData = NULL, *bioDataP7 = NULL;
	/// \brief Stack of PKCS#7 signer information
	STACK_OF(PKCS7_SIGNER_INFO) *signerInfoStack = NULL;
	/// \brief Private key
	EVP_PKEY* privateKey = NULL;
	/// \brief X.509 certificate handling
	X509* x509Certificate = NULL;
	/// \brief PKCS#12 data
	PKCS12* pkcs12Data = NULL;
	
	int errCode;

	ValidateProperties();
	
	BIO *bp = BIO_new( BIO_s_mem() );
	
	try
	{
		if ( asClient )
		// Client - get data with in base64 format , verify using ssl, and extract original message
		{
			//verify
			// clear error stack
			ERR_clear_error();

			string tmpData( ( char* )inputBuffer->buffer(), 0, inputBuffer->size() );
			string formatedData = "-----BEGIN PKCS7-----\n" + tmpData;
			if( formatedData[ formatedData.length() - 1 ] == '\n' ) 
				formatedData += "-----END PKCS7-----";
			else
				formatedData += "\n-----END PKCS7-----";
			
			int formatedSize = formatedData.length();
			
			char* c_string = new char[ formatedSize + 1 ];
			
			formatedData.copy( c_string, formatedSize );
			c_string[ formatedSize ] = 0;
			
			if ( formatedSize > 100 )
			{
				string partialBuffer = string( c_string, 100 );
				DEBUG( "Message before unsign is : ( first 100 bytes ) [" << partialBuffer << "]" );
			}
			else
			{
				DEBUG( "Message before unsign is : [" << c_string << "]" );	
			}
					
			bioData = BIO_new_mem_buf( c_string , -1);
			
			//pkcs7Data = PKCS7_new();
			
			DEBUG( "Ready to start verify process " );
			if( ( pkcs7Data = PEM_read_bio_PKCS7( bioData, NULL, NULL, NULL ) ) == NULL)
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
			
			bioDataP7 = NULL;
			
			if( PKCS7_is_detached( pkcs7Data ) ) 
			{
				throw runtime_error( "PKCS7 is detached and we don't suport this for the moment" );
			}
			else
			{
				bioDataP7=PKCS7_dataInit( pkcs7Data, NULL );
			}
			
			//read original mesaje from pkcs7 object

			string outputBuffer = ReadDataFromBIO( bioDataP7 );						
			DEBUG2( "Current message : [" << outputBuffer << "]" );
			
			outputData.get()->copyFrom( outputBuffer );
			
			// We can now verify signatures	
			signerInfoStack = PKCS7_get_signer_info( pkcs7Data );
			if ( signerInfoStack == NULL )
			{
				throw runtime_error( "There are no signatures on this data " );
			}
	
			/* Ok, first we	need to, for each subject entry, see if	we can verify */
			for	( int i = 0; i < sk_PKCS7_SIGNER_INFO_num( signerInfoStack ); i++ )
			{
				//ASN1_UTCTIME *tm;
				//char *str1,*str2;
				
				//get signature info
				signerInfo = sk_PKCS7_SIGNER_INFO_value( signerInfoStack, i );
				x509Certificate = PKCS7_cert_from_signer_info( pkcs7Data, signerInfo );
				
				//do the verify
				errCode = PKCS7_signatureVerify( bioDataP7, pkcs7Data, signerInfo, x509Certificate );
				//rc=PKCS7_dataVerify( m_CertStore, &m_CertCtx, bioDataP7, pkcs7Data, signerInfo );

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
			BIO_free_all( bioData );
			BIO_free_all( bioDataP7 );
			
			PKCS7_free( pkcs7Data );
			//X509_STORE_free( m_CertStore );
			
		}
		else
		// Server - 
		{
			bioData = BIO_new_mem_buf( ( char* )inputBuffer->buffer(), inputBuffer->size() );
		
			BIO* inputBioFile = NULL;
			try
			{
				DEBUG( "Ready to start sign process " );
				if( ( inputBioFile = BIO_new_file( m_CertFileName.c_str(), "r" ) ) == NULL ) 
				{	
					SSL_load_error_strings();
					ERR_load_crypto_strings();

					stringstream errorMessage;
					errorMessage << "An error occured while opening the certificate file [" << m_CertFileName << 
						"] for read : [" << ERR_error_string( ERR_get_error(), NULL ) << "]" ;
			
					TRACE( errorMessage.str() );
				
					throw runtime_error( errorMessage.str() );
				}

				pkcs12Data = PKCS12_init( NID_pkcs7_data );
				d2i_PKCS12_bio( inputBioFile, &pkcs12Data );

				BIO_free( inputBioFile );
			}
			catch( ... )
			{
				if ( inputBioFile != NULL )
					BIO_free( inputBioFile );
				throw;
			}			

			SSLeay_add_all_algorithms();
			
			//x509Certificate = X509_new();
			//privateKey = EVP_PKEY_new();
	
			// parse PKCS#12 structure, the private key will be written to privateKey, the corresponding certificate to x509Certificate

			errCode = PKCS12_parse( pkcs12Data, m_CertPasswd.c_str(), &privateKey, &x509Certificate, NULL );
			if( errCode <= 0 )
			{
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "An error occured while parsing pkcs12 object : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]. Check the password used for certificate." ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
	

//in x
			pkcs7Data = PKCS7_new();
			
			errCode = PKCS7_set_type( pkcs7Data, NID_pkcs7_signed );
			if( !errCode )
			{
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "An error occured while setting type : [" << ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
			
			signerInfo = PKCS7_add_signature( pkcs7Data, x509Certificate, privateKey, EVP_sha1() );			
			if( signerInfo == NULL )
			{	
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "An error occured while adding signature to pkcs7 object : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
			
			// If you do this then you get signing time automatically added
			PKCS7_add_signed_attribute( signerInfo, NID_pkcs9_contentType, V_ASN1_OBJECT, OBJ_nid2obj( NID_pkcs7_data ) );
			
			// we may want to add more 
			PKCS7_add_certificate( pkcs7Data, x509Certificate );
			
			// Set the content of the signed to 'data' 
			PKCS7_content_new( pkcs7Data, NID_pkcs7_data );

			//if (!nodetach)
			//	PKCS7_set_detached(p7,1);
			
	
			if( ( bioDataP7 = PKCS7_dataInit( pkcs7Data, NULL ) ) == NULL ) 
			{	
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "An error occured while initializing pkcs7 data process : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
	
			
			char buffer[ BUF_MAX_SIZE ];
			
			for (;;)
			{
				int i = BIO_read( bioData, buffer, sizeof( buffer ) );
				if (i <= 0)
					break;
				BIO_write( bioDataP7, buffer, i );
			}
		
	// sf x
			
			errCode = PKCS7_dataFinal( pkcs7Data, bioDataP7 );
			if( !errCode ) 
			{	
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "An error occured while terminating pkcs7 data process : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
		
			errCode = PEM_write_bio_PKCS7( bp, pkcs7Data );
			if( errCode <= 0 )
			{
				SSL_load_error_strings();
				ERR_load_crypto_strings();

				stringstream errorMessage;
				errorMessage << "An error occured while writing data to bio buffer : [" 
					<< ERR_error_string( ERR_get_error(), NULL ) << "]" ;
		
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}

	string outputBuffer = ReadDataFromBIO( bp );

			//string outputBuffer = "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp";
			
			BIO_free_all( bp );
			BIO_free_all( bioData );
			BIO_free_all( bioDataP7 );
			
			PKCS7_free( pkcs7Data );
			EVP_PKEY_free( privateKey );
			X509_free( x509Certificate );		
			PKCS12_free( pkcs12Data );

			DEBUG2( "Current message : [" << outputBuffer << "]" );
			
			outputBuffer.erase( 0, 22 );
			outputBuffer.erase( outputBuffer.end() - 21 , outputBuffer.end() );
			
			//TODO delete header and footer
			outputData.get()->copyFrom( outputBuffer );
		}//Server end 
		//=================================================
	}
	catch( const std::exception& e )
	{
		BIO_free_all( bp );
		BIO_free_all( bioData );
		BIO_free_all( bioDataP7 );
		
		PKCS12_free( pkcs12Data );
		PKCS7_free( pkcs7Data );
		EVP_PKEY_free( privateKey );
		X509_free( x509Certificate );
		stringstream messageBuffer;
		messageBuffer << typeid( e ).name() << " exception [" << e.what() << "]";

		TRACE( messageBuffer.str() );	
		throw;
	}
	catch( ... )
	{
		BIO_free_all( bp );
		BIO_free_all( bioData );
		BIO_free_all( bioDataP7 );
		
		PKCS12_free( pkcs12Data );
		PKCS7_free( pkcs7Data );
		EVP_PKEY_free( privateKey );
		X509_free( x509Certificate );
		TRACE( "Unknown exception while processing message in SSLFilter filter" );
		throw;
	}

	return AbstractFilter::Completed;
}	

AbstractFilter::FilterResult SSLFilter::ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
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

			DEBUG2("Message to deserialize : [" << outputBuffer->str() <<"]");
			
			tempDoc = XmlUtil::DeserializeFromString( outputBuffer->buffer(), outputBuffer->size() );
			DEBUG( "Deserialization OK" );

			// replace document element
			if ( outputData == NULL )
				throw logic_error( "You must supply a valid DOMDocument to ProcessMessage" );
				
			DOMNode* docElement = outputData->getDocumentElement();
			if( docElement != NULL )
			{
				DEBUG( "Removing current document element" );
				outputData->removeChild( docElement );
				docElement->release();
			}
				
			DEBUG( "Append new root node" );
			outputData->appendChild( outputData->importNode( tempDoc->getDocumentElement(), true ) );
			DEBUG( "Object replaced" );
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

AbstractFilter::FilterResult SSLFilter::ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	if( asClient )
	{
		throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
	}
	else 
	{
		DEBUG( "Method XmlToBuffer");
		string serializedDOM = XmlUtil::SerializeToString( inputData );

		ManagedBuffer* inputBuffer = new ManagedBuffer();
		inputBuffer->copyFrom( serializedDOM );

		return ProcessMessage( AbstractFilter::buffer_type( inputBuffer ), outputData, transportHeaders, asClient );
	}
}

bool SSLFilter::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient )
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

void SSLFilter::ValidateProperties()
{
	if ( m_Properties.ContainsKey( SSLFilter::SSLCERTFILENAME ) )
	{
		m_CertFileName = m_Properties[ SSLFilter::SSLCERTFILENAME ];
		if ( m_Properties.ContainsKey( SSLFilter::SSLCERTPASSWD ) )
			m_CertPasswd = m_Properties[ SSLFilter::SSLCERTPASSWD ];
		else
			throw invalid_argument( "Required parameter missing : SSLCERTPASSWD" ); 
	}
	else  m_CertFileName ="";
}

string SSLFilter::ReadDataFromBIO( BIO* bp )
{
	char c_buf[ BUF_MAX_SIZE ];
	string buffer = "";
	bool moreToRead = true;
	int i=0;
	while( moreToRead )
	{
		i = BIO_read( bp, c_buf, sizeof( c_buf ) );	
		if ( i > 0 )
		{
			if( i < BUF_MAX_SIZE )
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

bool SSLFilter::IsSigned( const string& signedString )
{
	bool returnValue = false;
	char* c_string = NULL;
	PKCS7* p7 = NULL;
	BIO* data = NULL;
	try
	{
			// clear error stack
			ERR_clear_error();

			string formatedData = "-----BEGIN PKCS7-----\n" + signedString;
			if( formatedData[ formatedData.length() -1 ] == '\n' ) 
				formatedData += "-----END PKCS7-----";
			else
				formatedData += "\n-----END PKCS7-----";
			
			int formatedSize = formatedData.length();
			
			c_string = new char[ formatedSize + 1 ];
			
			formatedData.copy( c_string, formatedSize );
			c_string[ formatedSize ] = 0;
			
			if ( formatedSize > 100 )
			{
				string partialBuffer = string( c_string, 100 );
				DEBUG( "Message before unsign is : ( first 100 bytes ) [" << partialBuffer << "]" );
			}
			else
			{
				DEBUG( "Message before unsign is : [" << c_string << "]" );	
			}
			
			data = BIO_new_mem_buf( c_string, -1 );
		
			//p7 = PKCS7_new();
			
			DEBUG( "Ready to start verify process " );
			p7 = PEM_read_bio_PKCS7( data, NULL, NULL, NULL ) ;
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

void SSLFilter::Hack64bLines( string& unhackString )
{
	try
	{
		string::iterator bufferSeeker = unhackString.begin() + 64;
		if( ( *bufferSeeker != '\r' ) && ( *bufferSeeker != '\n' ) )
		{
			for( int insidx = 64; insidx < unhackString.length(); insidx += 64 )
			{
				unhackString.insert( insidx, Platform::getNewLineSeparator() );
				insidx += Platform::getNewLineSeparator().length();
			}
		}
	}
	catch( ... )
	{
		TRACE( "Unknown exception while splitting lines to 64 bytes" );
		throw;
	}
}

int SSLFilter::Verify_Callback( int ok, X509_STORE_CTX *ctx )
{
	char buf[ 256 ];
	X509 *err_cert;
	int	err,depth;

	err_cert = X509_STORE_CTX_get_current_cert( ctx );
	err = X509_STORE_CTX_get_error( ctx );
	depth = X509_STORE_CTX_get_error_depth( ctx );

	X509_NAME_oneline( X509_get_subject_name( err_cert), buf, 256 );

	TRACE( "depth= [" << depth << "] and buffer= [" << buf << "]" );

	if ( !ok )
		TRACE( "verify error:num= [" << err << "] and error string= [" << X509_verify_cert_error_string( err ) << "]" );

	switch ( ctx->error )
	{
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
			X509_NAME_oneline( X509_get_issuer_name( ctx->current_cert ), buf, 256 );
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

	return( ok );
}
void SSLFilter::SSLSetLock_Callback( int mode, int n, const char *file, int line )
{
    	if ( mode & CRYPTO_LOCK )
    	{
    		pthread_mutex_lock( &( SSLFilter::SSLSyncMutex[n] ) );
    	} 
    	else 
    	{
    		pthread_mutex_unlock( &( SSLFilter::SSLSyncMutex[n] ) );
    	}
}
unsigned long SSLFilter::SSLSetId_Callback()
{
	unsigned long ret;

	ret = ( unsigned long )pthread_self();
	
	return( ret );
}
void SSLFilter::Initialize()
{
	
	//mutexes an callbacks once initializations
	int i;

	SSLFilter::SSLSyncMutex = ( pthread_mutex_t* )OPENSSL_malloc( CRYPTO_num_locks() * sizeof( pthread_mutex_t ) );
	for ( i = 0; i < CRYPTO_num_locks(); i++ )
	{
		pthread_mutex_init( &( SSLFilter::SSLSyncMutex[i] ),NULL );
	}

	CRYPTO_set_id_callback( ( unsigned long (*)() )SSLFilter::SSLSetId_Callback );
	CRYPTO_set_locking_callback( ( void (*)( int,int, const char *,int ) )SSLFilter::SSLSetLock_Callback );
	
	//SSL digest table once initialisation 
#ifndef	OPENSSL_NO_MD2
	EVP_add_digest( EVP_md2() );
#endif
#ifndef	OPENSSL_NO_MD5
	EVP_add_digest( EVP_md5() );
#endif
#ifndef	OPENSSL_NO_SHA1
	EVP_add_digest( EVP_sha1() );
#endif
#ifndef	OPENSSL_NO_MDC2
	EVP_add_digest( EVP_mdc2() );
#endif
	DEBUG ( "SSLFilter initialisation performed" );
}

void SSLFilter::Terminate()
{
	//cleans up SSL initialisations
	int i;

	CRYPTO_set_locking_callback( NULL );

	if ( SSLFilter::SSLSyncMutex != NULL )
	{
		for( i = 0; i < CRYPTO_num_locks(); i++ )
		{
			pthread_mutex_destroy( &( SSLFilter::SSLSyncMutex[i] ) );
		}
		OPENSSL_free( SSLFilter::SSLSyncMutex );
	}
	EVP_cleanup();
	DEBUG( "SSL cleanup performed" );	
}
