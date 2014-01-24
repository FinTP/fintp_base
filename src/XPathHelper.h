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

#ifndef XPATHHELPER_H
#define XPATHHELPER_H

#include "DllMain.h"

#include <string>

#include <xalanc/Include/PlatformDefinitions.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>

#include <xalanc/XPath/XPathConstructionContextDefault.hpp>
#include <xalanc/XPath/XPathExecutionContextDefault.hpp>
#include <xalanc/XPath/XPathEvaluator.hpp>
#include <xalanc/DOMSupport/XalanDocumentPrefixResolver.hpp>

#include <xalanc/XPath/XPathFactoryDefault.hpp>
#include <xalanc/XPath/XObjectFactory.hpp>
#include <xalanc/XPath/XObject.hpp>

#include <xalanc/DOMSupport/XalanDocumentPrefixResolver.hpp>

#include <xalanc/PlatformSupport/XSLException.hpp>
#include <xalanc/XPath/NodeRefList.hpp>
#include <xalanc/XalanDOM/XalanDOMString.hpp>
#include <xalanc/XalanSourceTree/XalanSourceTreeDOMSupport.hpp>
#include <xalanc/XalanSourceTree/XalanSourceTreeInit.hpp>
#include <xalanc/XalanSourceTree/XalanSourceTreeParserLiaison.hpp>

using namespace std;

namespace FinTP
{
	class ExportedObject XPathHelper
	{
		private :
			
			//XALAN_CPP_NAMESPACE_QUALIFIER XPathEvaluator m_Evaluator;
			//XALAN_CPP_NAMESPACE_QUALIFIER XalanSourceTreeDOMSupport m_DOMSupport;
			//XALAN_CPP_NAMESPACE_QUALIFIER XalanSourceTreeParserLiaison m_Liaison;

			XPathHelper();

		public :

			~XPathHelper();		
		
			//evaluate function 
			static XALAN_CPP_NAMESPACE_QUALIFIER XalanNode* const Evaluate( const string& xPath, XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* doc, const string& defaultPrefix = "" );
			static XALAN_CPP_NAMESPACE_QUALIFIER NodeRefList const EvaluateNodes( const string& xPath, XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* doc, const string& defaultPrefix = "" );
			
			static string SerializeToString( XALAN_CPP_NAMESPACE_QUALIFIER XalanNode* refNode );
			//static string SerializeToString2( XALAN_CPP_NAMESPACE_QUALIFIER XalanNode* crtNode );
			static string SerializeToString( XALAN_CPP_NAMESPACE_QUALIFIER XalanDOMString refString );
	};
}

#endif // XPATHHELPER_H
