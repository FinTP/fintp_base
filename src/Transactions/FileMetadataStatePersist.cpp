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

#include "FileMetadataStatePersist.h"

using namespace FinTP;

FileMetadataStatePersist::FileMetadataStatePersist() : AbstractStatePersistence()
{
}

FileMetadataStatePersist::~FileMetadataStatePersist()
{
}

void FileMetadataStatePersist::internalSet( const string& storageKey, DictionaryEntry storageData )
{
}

DictionaryEntry FileMetadataStatePersist::internalGet( const string& storageKey, const string& key )
{
	return DictionaryEntry( key, "" );
/*	fstream myMetafile( metaName.data() );
	if ( myMetafile.is_open() )
	{
		char metabuffer[ 255 ];
		while( !myMetafile.eof() )
		{
			myMetafile.getline( metabuffer, 255 );
			currentAttempt++;
		}
		currentAttempt --;
		myMetafile.close();
	}
	
			ofstream myNewMetafile( metaName.data(), ios::app );
		myNewMetafile.seekp( 0, ios::end );
		myNewMetafile << "Attempt #" << currentAttempt;
		if ( !renameFailed )
			myNewMetafile << endl;
		else
			myNewMetafile << " rename from [" << m_CurrentFilename << "] to [" << destName << "] failed : " << sys_errlist[ renameError ] << endl;
		myNewMetafile.close();
	
		if ( renameFailed )
			return;
	*/
}
		
void FileMetadataStatePersist::internalInitStorage( const string& key )
{
}

void FileMetadataStatePersist::internalReleaseStorage( const string& key )
{
}
