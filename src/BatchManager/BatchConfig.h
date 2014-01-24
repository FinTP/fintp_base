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

#ifndef BATCHCONFIG_H
#define BATCHCONFIG_H

//#include <xercesc/dom/DOM.hpp>
#include <vector>
#include <map>

//XERCES_CPP_NAMESPACE_USE;
using namespace std;

#include "DllMain.h"

namespace FinTP
{
	class ExportedObject BatchRequestTransform
	{
		public:

			enum TransformType
			{
				SingleTemplate,
				Batch
			};
			
			BatchRequestTransform() : m_Type( BatchRequestTransform::SingleTemplate ), m_File( "" ) {};
			BatchRequestTransform( const BatchRequestTransform::TransformType type, const string& file ) :
				m_Type( type ), m_File( file ) {};
		
			BatchRequestTransform( const BatchRequestTransform& source ) : 
				m_Type( source.getType() ), m_File( source.getFile() ) {};

			BatchRequestTransform& operator=( const BatchRequestTransform& source )
			{
				if ( this == &source )
					return *this;

				m_Type = source.getType();
				m_File = source.getFile();

				return *this;
			}
			
			string getFile() const { return m_File; }
			BatchRequestTransform::TransformType getType() const { return m_Type; }
			
			static BatchRequestTransform::TransformType Parse( const string& type );
			
		private :

			BatchRequestTransform::TransformType m_Type;
			string m_File;		
	};

	//EXPIMP_TEMPLATE template class ExportedObject std::vector< BatchRequestTransform >;

	class ExportedObject BatchRequest
	{
		public:

			enum OrderType
			{
				None,
				Sequence
			};
			
			BatchRequest( const string& messageType, const BatchRequest::OrderType order, const string& storage ) :
				m_MessageType( messageType ), m_Order( order ), m_Storage( storage ) {};

			string getMessageType() const { return m_MessageType; }
			string getStorage() const { return m_Storage; }
			BatchRequest::OrderType getOrder() const { return m_Order; }
			
			static BatchRequest::OrderType Parse( const string& order );
			
			void AddTransform( const BatchRequestTransform& transform );
			const BatchRequestTransform& GetTransform();
			
		private :
		
			vector< BatchRequestTransform > m_Transforms;
			string m_MessageType;
			BatchRequest::OrderType m_Order;
			string m_Storage;
	};

	//EXPIMP_TEMPLATE template class ExportedObject std::map< string, BatchRequest >;

	class ExportedObject BatchConfig
	{
		public:
		
			BatchConfig();
			~BatchConfig();
			
			const BatchRequest& GetRequest( const string& queue, const string& messageType = "*" );
			
			void setConfigFile( const string& configFilename );
			
		private :
		
			map< string, BatchRequest > m_Requests;
	};
}

#endif // BATCHCONFIG_H 
