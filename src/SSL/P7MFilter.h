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

/**
\file P7MFilter.h
\brief P7M filter functions
*/
#ifndef _P7MFILTER_H_
#define _P7MFILTER_H_

#include "../AbstractFilter.h"
#include "../AppSettings.h"
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>

#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/pkcs7.h>
#include <openssl/x509.h>
#include <openssl/pkcs12.h>

namespace FinTP
{
	/**
	\class SSLFilter
	\brief Implement AbstractFilter methods for SSL format
	\author Sebastian Gansca
	*/
	class ExportedObject P7MFilter : public AbstractFilter
	{
		private:
		
			/// \brief PKCS#7 data in PEM format 
			PKCS7* m_P7;
			/// \brief PKCS#7 signer information
			PKCS7_SIGNER_INFO *m_SI;
			/// \brief I/O abstraction
			BIO	*m_Data, *m_P7bio;
			/// \brief Stack of PKCS#7 signer information
			STACK_OF(PKCS7_SIGNER_INFO)	*m_sk;
			/// \brief Private key
			EVP_PKEY* m_PKey;
			/// \brief X.509 certificate handling
			X509* m_X509;
			/// \brief PKCS#12 data
			PKCS12* m_Pkcs12;		
			
		public:
		
			/// \brief Certificate file name
			static const string P7MCERTFILENAME;
			/// \brief Certificate password
			static const string P7MCERTPASSWD;
			/// \brief Constructor
			P7MFilter();
			/// \brief Destructor
			~P7MFilter();	
				
			/**
			\brief Return \a TRUE if the filter can execute the requested operation in client/server context
			\param[in] method Method to test if supported
			\param[in] asClient \a TRUE if run as client , \a FALSE if run as server
			\return \a TRUE if the filter can execute the requested operation in client/server context
			*/
			bool isMethodSupported( FilterMethod method, bool asClient );
			
			/**
			\brief Process message from Buffer to Buffer , de-sign if as client , sign if as server, using options from transportHeaders
			\param[in] inputData Data to be transform in a buffer
			\param[out] outputData Data transformed in a buffer
			\param[in] transportHeaders Collection of options and parameters for transform
			\param[in] asClient \a TRUE if run as client , \a FALSE if run as server
			\return AbstractFilter::Completed if successful
			*/
			AbstractFilter::FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );
			/**
			\brief Process message from Buffer to XML , de-sign if as client , sign if as server, using options from transportHeaders
			\param[in] inputData Data to be transform in a buffer
			\param[out] outputData Data transformed in Xerces DOMDocument format
			\param[in] transportHeaders Collection of options and parameters for transform
			\param[in] asClient \a TRUE if run as client , \a FALSE if run as server
			\return AbstractFilter::Completed if successful
			*/
			AbstractFilter::FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient );
			/**
			\brief Process message from Buffer to Buffer , de-sign if as client , sign if as server, using options from transportHeaders
			\param[in] inputData Data to be transform in Xerces DOMDocument format
			\param[out] outputData Data transformed in a buffer
			\param[in] transportHeaders Collection of options and parameters for transform
			\param[in] asClient \a TRUE if run as client , \a FALSE if run as server
			\return AbstractFilter::Completed if successful
			*/
			AbstractFilter::FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
			}

			/// \brief	\attention Method XML to XML not supported by this filter
			AbstractFilter::FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::XmlToXml );
			}
			
			/// \brief	\attention Method XML to Buffer in char * format not supported by this filter
			AbstractFilter::FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
			}

			/// \brief	\attention Method Buffer to Buffer in char * format not supported by this filter
			AbstractFilter::FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
			}

			/**
			\brief \return \a TRUE if input document is signed
			\param[in] signedString Input string to verify
			*/
			static bool IsSigned( const string& signedString );

		private :
		
			/// \brief Certificate file name
			string m_CertFileName;
			/// \brief Certificate password
			string m_CertPasswd;
			
			/// \brief Validates required properties
			void ValidateProperties();
			/**
			\brief \return Data in std::string format, converted from BIO format
			\param[in] bp Input data in BIO format
			*/
			static string ReadDataFromBIO( BIO* bp );
			//read pkcs7 structure from b64 format
			static PKCS7* B64_read_PKCS7( BIO* bio );
			//write pkcs7 structure to b64 format
			static int B64_write_PKCS7(BIO *bio, PKCS7 *p7);
			
			///\deprecated Not use
			///\todo see if necesary
			static int Verify_Callback(int	ok,	X509_STORE_CTX *ctx );
	};
}
#endif // _P7MFILTER_H_
