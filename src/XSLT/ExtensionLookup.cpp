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
#include <xalanc/XPath/XPathParserException.hpp>
#include <xalanc/XalanDOM/XalanDOMException.hpp>
#include <xalanc/PlatformSupport/XalanStdOutputStream.hpp>
#include <xalanc/PlatformSupport/XalanOutputStreamPrintWriter.hpp>
#include <xalanc/PlatformSupport/DOMStringPrintWriter.hpp>
#include <xalanc/PlatformSupport/XalanSimplePrefixResolver.hpp>
#include <xalanc/XMLSupport/FormatterToXML.hpp>
#include <xalanc/XMLSupport/FormatterTreeWalker.hpp>
#include "ExtensionLookup.h"

#include <string>
#include <sstream>
#include <iostream>
//#include <strstream>
#include "XmlUtil.h"
#include "StringUtil.h"

//#include <boost/crc.hpp>
//#include <boost/cstdint.hpp>
#include "../XPathHelper.h"
#include "Base64.h"


using namespace std;
using namespace FinTP;
XALAN_CPP_NAMESPACE_USE

Database* ( *FunctionLookup::m_CallbackDatabase )( void ) = NULL;
DatabaseProviderFactory* ( *FunctionLookup::m_CallbackProvider )( void ) = NULL;

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
XObjectPtr FunctionLookup::execute( XPathExecutionContext& executionContext, XalanNode* context, const XObjectArgVectorType& args, const LocatorType* locator ) const
{
	XALAN_USING_XALAN( XalanDOMString );

	if ( args.size() < 1 )
	{
		stringstream errorMessage;
		errorMessage << "The Lookup() function takes 1 or more arguments [ sp to call, params... ], but received " << args.size();
#if (_XALAN_VERSION >= 11100)
		executionContext.problem( XPathExecutionContext::eXPath, XPathExecutionContext::eError, XalanDOMString( errorMessage.str().c_str() ), locator, context); 
#else
		executionContext.error( XalanDOMString( errorMessage.str().c_str() ), context );
#endif
	}

	string dbStoredProcedure = localForm( ( const XMLCh* )( args[ 0 ]->str().data() ) );

	string resultValue = "Database connection callback not supplied";

	if ( ( m_CallbackDatabase != NULL ) && ( m_CallbackProvider != NULL ) )
	{
		DataSet* result = NULL;
		try
		{
			Database* currentDatabase = ( *m_CallbackDatabase )();
			DatabaseProviderFactory* currentProvider = ( *m_CallbackProvider )();

			if ( ( currentDatabase == NULL ) || ( currentProvider == NULL ) )
				resultValue = "Could not obtain database connection";
			else
			{
				ParametersVector myParams;
				for ( unsigned int i = 1; i<args.size(); i++ )
				{				
					string paramValue = localForm( ( const XMLCh* )( args[ i ]->str().data() ) );
					DataParameterBase *paramRef = currentProvider->createParameter( DataType::CHAR_TYPE );
					paramRef->setDimension( paramValue.length() );	  	 
					paramRef->setString( paramValue );
					myParams.push_back( paramRef );
				}

				result = currentDatabase->ExecuteQueryCached( DataCommand::SP, dbStoredProcedure, myParams );
				resultValue = StringUtil::Trim( result->getCellValue( 0, "RESULT" )->getString() );
			}
		}
		catch( ... )
		{
			if ( result != NULL )
			{
				delete result;
			}
			throw;
		}
	}

	return executionContext.getXObjectFactory().createString( unicodeForm( resultValue ) );
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
	FunctionLookup*
#endif
FunctionLookup::clone( MemoryManagerType& theManager ) const
{
    return XalanCopyConstruct(theManager, *this);
}

const XalanDOMString& FunctionLookup::getError( XalanDOMString& theResult ) const
{
	theResult.assign( "The lookup function accepts 1 or more arguments  [ sp to call, params... ]" );
	return theResult;
}
#else
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionLookup*
#endif
FunctionLookup::clone() const
{
	return new FunctionLookup( *this );
}

const XalanDOMString FunctionLookup::getError() const
{
	return StaticStringToDOMString( XALAN_STATIC_UCODE_STRING( "The lookup function accepts 1 or more arguments [ sp to call, params... ]" ) );
}
#endif

/*
void FunctionLookup::Initialize()
{
	int onceResult = pthread_once( &FunctionLookup::DatabaseKeysCreate, &FunctionLookup::CreateKeys );
	if ( 0 != onceResult )
	{
		TRACE( "One time key creation for lookup DbOps failed [" << onceResult << "]" );
	}
}

void FunctionLookup::CreateKeys()
{
	cout << "Thread [" << pthread_self() << "] creating lookup database key..." << endl;

	int keyCreateResult = pthread_key_create( &FunctionLookup::DatabaseKey, &FunctionLookup::DeleteDatabase );
	if ( 0 != keyCreateResult )
	{
		TRACE( "An error occured while creating lookup database key [" << keyCreateResult << "]" );
	}
}

void FunctionLookup::DeleteDatabase( void* data )
{
	pthread_t selfId = pthread_self();	
	//TRACE_NOLOG( "Deleting data connection for thread [" << selfId << "]" );
	Database* database = ( Database* )data;
	if ( database != NULL )
	{
		try
		{
			database->Disconnect();
		}
		catch( ... )
		{
			TRACE_NOLOG( "An error occured while disconnecting from the lookup database" );
		}
		delete database;
		database = NULL;
	}
	int setSpecificResult = pthread_setspecific( FunctionLookup::DataKey, NULL );
	if ( 0 != setSpecificResult )
	{
		TRACE_NOLOG( "Set thread specific DatabaseKey failed [" << setSpecificResult << "]" );
	}
}

void FunctionLookup::createProvider( const NameValueCollection& configSection )
{
	if ( configSection.ContainsKey( "provider" ) )
	{
		string provider = configSection[ "provider" ];

		// create a provider if necessary
		if ( m_DatabaseProvider == NULL )
		{
			// lock and attempt to create the provider
			int mutexLockResult = pthread_mutex_lock( &m_SyncRoot );
			if ( 0 != mutexLockResult )
			{
				stringstream errorMessage;
				errorMessage << "Unable to lock Sync mutex [" << mutexLockResult << "]";
				TRACE( errorMessage.str() );

				throw runtime_error( errorMessage.str() );
			}

			try
			{			
				// other thread may have already created the provider
				if ( m_DatabaseProvider == NULL )
					m_DatabaseProvider = DatabaseProvider::GetFactory( provider );
			}
			catch( ... )
			{
				TRACE( "Unable to create database provider [" << provider << "]" );

				int mutexUnlockResult = pthread_mutex_unlock( &m_SyncRoot );
				if ( 0 != mutexUnlockResult )
				{
					TRACE( "Unable to unlock Sync mutex [" << mutexUnlockResult << "]" );
				}
				throw;
			}

			int mutexUnlockResult = pthread_mutex_unlock( &m_SyncRoot );
			if ( 0 != mutexUnlockResult )
			{
				TRACE( "Unable to unlock Sync mutex [" << mutexUnlockResult << "]" );
			}
		}
	}
}*/
