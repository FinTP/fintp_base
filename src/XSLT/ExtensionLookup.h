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

#ifndef EXTENSIONLOOKUP_H
#define EXTENSIONLOOKUP_H

#include <xalanc/Include/PlatformDefinitions.hpp>
#include <xalanc/XPath/Function.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xalanc/XalanTransformer/XalanTransformer.hpp>
#include <xalanc/XPath/XObjectFactory.hpp>

#include "DatabaseProvider.h"
#include "Database.h"

#include "../DllMain.h"

XALAN_USING_XALAN(Function)
XALAN_USING_XALAN(XPathExecutionContext)
XALAN_USING_XALAN(XalanDOMString)
XALAN_USING_XALAN(XalanNode)
//XALAN_USING_XALAN(StaticStringToDOMString)
XALAN_USING_XALAN(XObjectPtr)

#ifdef XALAN_1_9
XALAN_USING_XALAN(MemoryManagerType)
#endif

namespace FinTP
{
	class ExportedObject FunctionLookup : public Function
	{
		public:
			virtual XObjectPtr execute( XPathExecutionContext& executionContext, XalanNode* context, const XObjectArgVectorType& args,const LocatorType* locator ) const;

			static void setDatabaseCallback( Database* ( *callback )( void ) )
			{
				m_CallbackDatabase = callback;
			}

			static void setProviderCallback( DatabaseProviderFactory* ( *callback )( void ) )
			{
				m_CallbackProvider = callback;
			}

	#ifdef XALAN_1_9
			#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
				virtual Function* clone( MemoryManagerType& theManager ) const;
			#else
				virtual FunctionLookup* clone( MemoryManagerType& theManager ) const;
			#endif
		protected:
			const XalanDOMString& getError( XalanDOMString& theResult ) const;
	#else
			#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
				virtual Function* clone() const;
			#else
				virtual FunctionLookup* clone() const;
			#endif
		protected:
			const XalanDOMString getError() const;
	#endif
			
		private:
			// The assignment and equality operators are not implemented...
			FunctionLookup& operator=( const FunctionLookup& );
			bool operator==( const FunctionLookup& ) const;

			static Database* ( *m_CallbackDatabase )( void );
			static DatabaseProviderFactory* ( *m_CallbackProvider )( void );

			/*static DatabaseProviderFactory *m_DatabaseProvider;

			static pthread_once_t DatabaseKeysCreate;
			static pthread_key_t DatabaseKey;
		
			static void CreateKeys();
			static void DeleteDatabase( void* data );

			static Database* getDatabase();

			static void Initialize();
			static void Terminate();*/
	};
}

#endif // EXTENSIONLOOKUP_H
