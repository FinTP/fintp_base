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

#ifndef SAAFILTER_H
#define SAAFILTER_H

#include "AbstractFilter.h"
#include "BatchManager/BatchManager.h"
#include "BatchManager/Storages/BatchMQStorage.h"
#include <boost/shared_ptr.hpp>
namespace FinTP
{
	class ExportedObject SAAFilter : public AbstractFilter
	{
		public:

			static const string SAAReplyQueue;
			static const string MESSAGE_DATE;

			SAAFilter( boost::shared_ptr< BatchManagerBase > batchManager ): AbstractFilter( FilterType::SWIFTFORMAT )
			{
				m_BatchStorage = batchManager;
			}
			~SAAFilter() {}

			bool canLogPayload() { return false; }

			bool isMethodSupported( FilterMethod method, bool asClient );

			FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient );

			FilterResult ProcessMessage( unsigned char* inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient );

			FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );

			FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient );

			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient );

			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );

			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient );
			void Rollback();
			void Commit();

		private:

			boost::shared_ptr<BatchManagerBase> m_BatchStorage;

			void ValidateProperties( NameValueCollection& transportHeaders );
	};
}

#endif //SAAFILTER_H
