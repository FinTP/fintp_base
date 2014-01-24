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
\file /src/DllMain.h
\brief Defines options for compile as a library and contains VersionInfo class
*/

#pragma once

#ifdef WIN32
	#ifdef _BASEDLL
		#define ExportedObject __declspec( dllexport )
		#ifdef QPAYEXT
			#define ExportedObjectExt __declspec( dllexport )
		#else
			#define ExportedObjectExt __declspec( dllimport )
		#endif
		//#define EXPIMP_TEMPLATE
	#else
		#define ExportedObject __declspec( dllimport )
		#define ExportedObjectExt __declspec( dllimport )
		//#define EXPIMP_TEMPLATE extern
	#endif
#else
	#define ExportedObject 
	#define ExportedObjectExt
	//#define EXPIMP_TEMPLATE
#endif

#ifndef DLLMAIN_FINTP_LIB
#define DLLMAIN_FINTP_LIB

#ifdef WIN32
	#pragma warning( disable : 4251 )
	//to disable specific warning for std classes
	//warning C4251: : class 'std::vector<_Ty>' needs to have dll-interface to be used by clients of class <class>
#endif

//#define USE_FinTP using namespace FinTP;

#include <string>

using namespace std;

namespace FinTP
{
	/**
	\class VersionInfo
	\brief Contains information about library version
	\author Horia Beschea
	*/
	class ExportedObject VersionInfo
	{
		private : 
			/// \brief Constructor
			VersionInfo(){};
			
		public :
			/// \brief \return Name of the library
			static std::string Name();
			/// \brief \return Id of the library
			static std::string Id();
			///\return Repository url 
			static std::string Url();
			/// \brief \return BuildDate of the library
			static std::string BuildDate();
	};
}

#endif // DLLMAIN_FINTP_LIB
