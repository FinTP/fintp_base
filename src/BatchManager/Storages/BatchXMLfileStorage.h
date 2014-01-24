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

#ifndef BATCHXMLFILESTORAGE_H
#define BATCHXMLFILESTORAGE_H

#include "../BatchStorageBase.h"
#include "XmlUtil.h"
#include "../../XSLT/XSLTFilter.h"


namespace FinTP
{
	class ExportedObject BatchXMLfileStorage : public BatchStorageBase
	{
		public:
		
			BatchXMLfileStorage();
			~BatchXMLfileStorage();
			
			//at first enqueue create DOMDocument from the item 
			// ex. ACH XML - first element will be <CoreBulk...> <GrpHdr> ...</GrpHdr> <CdTrf> ... first message ... </CdtTf> </Core Bulk...>
			//next enqueues will add the children as items
			void enqueue( BatchResolution& resolution );
			
			BatchItem dequeue();
			
			// open/close storage
			
			// if openmode = inputMode    => TODO: decompose the DOM from the BatchXML
			// if openmode = outputMode   => TODO: build the DOM for the BatchXML
			void open( const string& storageId, ios_base::openmode openMode );	
			
			//free the storage: delete the whole document 
			void close( const string& storageId );
			
			void commit() {}
			void rollback() {}

			//nothing special TO DO
			long size() const { return 0; }
				
			//nothing special TO DO
			void setBufferSize( const unsigned long buffersize ){}
			
			//serialize XMLBatch document ( for test purpose )
			string getSerializedXml() const;
			
			void setSerializedXml( const string& document );		
			void setSerializedXml( const unsigned char * document, const unsigned long size );

			// for dequeue 
			// Xslt will be applied on m_CrtStorage , parameter passed will be item position 
			// will return the m_CrtStorage item with position specified
			
			// using Xslt will offer the possibility to do a specialized selection
			// for example => return item in position specified
			//			   => header + item in posistion specified
			string getXslt() const { return m_XsltFileName; }		
			void setXslt( const string& xsltFile ) { m_XsltFileName = xsltFile; }

			// for enqueue 
			// XPath will be applied on m_CrtStorage and will return the parent node
			// for subsequent batch items
			string getXPath() const { return m_XPath; }		
			void setXPath( const string& xpath ) { m_XPath = xpath; }
			
			long getCrtSequence(){ return m_CrtSequence; }

			void setXPathCallback( string ( *xPathCallback )( const string& itemNamespace ) )
			{
 				 m_XPathCallback = xPathCallback;
			}

			string getInternalBatchID() const { return m_BatchId; }
			
		private :

			//m_CrtStorage is the DOM under construction 
			//the XML batch file under construction or under parsed
			XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* m_CrtStorage;
			XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMWrapperParsedSource* m_CrtStorageWrapper;

			XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* m_CrtStorageRoot, *m_CrtInsertNode;
			string m_BatchId;
			int m_NumberOfItems;
			
			// current sequence in the current Storage
			long m_CrtSequence;
			
			XSLTFilter m_XsltFilter;
			string m_XsltFileName;
			string m_XPath;

			string ( *m_XPathCallback )( const string& itemNamespace );
			map< string, vector< string > > m_XPathTokens;
			vector< string > getXPath( const string& itemNamespace );
	};
}

#endif // BATCHXMLFILESTORAGE_H
