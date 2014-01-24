#ifndef PLATFORM_DEPS_H
#define PLATFORM_DEPS_H

#if defined( __x86_64__ )
	#define _64_BIT
#endif
	
#include "DllMain.h"
#include <boost/cstdint.hpp>
#include <string>
using namespace std;

namespace FinTP
{
	/// Util class for using paths in a platform independent manner
	class ExportedObject Path
	{
		public :
		
			// Path separator 
			static const string PATH_SEPARATOR;		
			
			// Combines two paths( relative + relative or absolute + relative )
			// <returns>The combine path</returns>
			static string Combine( const string& path1, const string& path2 );
			
			// Finds the filename in the specified path
			//<returns>The full filename( filename + extension )</returns>
			static string GetFilename( const string& path );
	};

	class ExportedObject Convert
	{
		//private :
		//	Convert();

		public :
		
	#ifdef WIN32
			static unsigned long ChangeEndian( const unsigned long value );
			static long ChangeEndian( const long value );
	#else
			static unsigned long ChangeEndian( unsigned long value );
			static long ChangeEndian( long value );
	#endif

	#ifdef _64_BIT
			static unsigned int ChangeEndian( unsigned int value );
			static int ChangeEndian( int value );
	#endif
	};

	class ExportedObject Platform
	{
		private :
			Platform();
		
			static string m_MachineName;
			static string m_MachineUID;
			static boost::uint16_t m_MachineHash;
			// Path separator 
			static const string NEWLINE_SEPARATOR;		

		public :

			static string GetOSName();
			static string GetMachineName();
			static string GetIp( const string& name="" );
			static string GetName( const string& ip="" );
			static string GetUID();
			static boost::uint16_t GetUIDHash();
			static string getNewLineSeparator();
	};

	class ExportedObject Process
	{
		private :
			Process();
			
		public :
			static long GetPID();
	};
}

#endif //PLATFORM_DEPS_H
