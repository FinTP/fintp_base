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

#ifndef BATCHITEMEVAL_H
#define BATCHITEMEVAL_H

#include <map>
#include <vector>

//#include "../XmlUtil.h"
//#include "../XPathHelper.h"
#include <xalanc/XalanDOM/XalanDocument.hpp>

#include "BatchResolution.h"

namespace FinTP
{
	class ExportedObject BatchItemEval
	{
		public:
		
			BatchItemEval();
			virtual ~BatchItemEval();
			
			static BatchResolution Evaluate( XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* document, const string& xPath, const string& eyeCatcher );
			
			//static void setCallback( BatchItemEval* ( *callback )( string ) ) { m_Callback = callback; };
			static void setEvaluator( BatchItemEval* evaluator ) { m_Evaluator = evaluator; }
		
		protected :
		
			static BatchItemEval* getEvaluator( const string& messageType );
			
			virtual int getSequence() { return BatchItem::last().getSequence(); }
			virtual string getBatchId() { return BatchItem::last().getBatchId(); }
			virtual string getMessageId() { return BatchItem::last().getMessageId(); }
			virtual BatchResolution::BatchAction getAction() { return BatchResolution::Release; }
			virtual bool isLast() { return false; }
		
			XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* m_Document;
			virtual void setDocument( XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* doc ){ m_Document = doc; }
			
		private :
		
			//static BatchItemEval* ( *m_Callback )( string );
			static BatchItemEval* m_Evaluator;
	};
}
 
#endif // BATCHITEMEVAL_H
