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

#ifndef BATCHITEMMT950EVAL_H
#define BATCHITEMMT950EVAL_H

#include "../BatchItemEval.h"

namespace FinTP
{
	class ExportedObject BatchItemMT950Eval : public BatchItemEval
	{
		public:
		
			BatchItemMT950Eval();
			~BatchItemMT950Eval();
			
		protected :
		
			// batchitemeval overrides
			int getSequence() { return m_Sequence; }
			string getBatchId() { return m_Statement; }
			bool isLast() { return m_LastMessage; }

			void setDocument( XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* doc );

		private :
		
			long m_Sequence;
			bool m_LastMessage;
			string m_Statement;
	};
}

#endif // BATCHITEMMT950EVAL_H
