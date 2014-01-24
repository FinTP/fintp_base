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

#include <sstream>

#include <errno.h>

#include "XmlUtil.h"
#include "Trace.h"
#include "BatchZipStorage.h"

using namespace FinTP;

#include <sstream>
#include <iostream>
#include <strstream>

BatchZipStorage::BatchZipStorage() : BatchStorageBase(), m_CrtSequence( 0 ), m_NumberOfItems( 0 ), m_CrtUnzipStorage( 0 ), m_CrtZipStorage( 0 )
{
}

BatchZipStorage::~BatchZipStorage()
{
	try
	{
		if ( m_CrtUnzipStorage != 0 )
		{
			ZRESULT result = CloseZip( m_CrtUnzipStorage );
			if( result != ZR_OK )
			{
				stringstream errorMessage;
				errorMessage << "An error occured while closing unzip handle [" << result << "]";
				TRACE( errorMessage.str() );
			}
			m_CrtUnzipStorage = 0;
		}
	}
	catch( ... ){}
	try
	{
		if ( m_CrtZipStorage != 0 )
		{
			ZRESULT result = CloseZip( m_CrtZipStorage );
			if( result != ZR_OK )
			{
				stringstream errorMessage;
				errorMessage << "An error occured while closing zip handle [" << result << "]";
				TRACE( errorMessage.str() );
			}
			m_CrtZipStorage = 0;
		}
	}
	catch( ... ){}
}

void BatchZipStorage::enqueue( BatchResolution& resolution )
{
	DEBUG( "Enqueue" );

	BatchItem item = resolution.getItem();

	DEBUG( "Current item sequence is " << item.getSequence() );
	ZRESULT result = ZipAdd( m_CrtZipStorage, item.getEyecatcher().c_str(), item.getBinPayload()->buffer(), item.getBinPayload()->size() );
	if( result != ZR_OK )
	{
		stringstream errorMessage;
		errorMessage << "An error occured while adding items to ZIP [" << result << "]";
		throw runtime_error( errorMessage.str() );
	}
	m_CrtSequence++;
	DEBUG( "Next sequence is " << m_CrtSequence );
}

BatchItem BatchZipStorage::dequeue()
{
	DEBUG( "Dequeue" );
	
	ZIPENTRY ze;
	ZRESULT result = GetZipItem( m_CrtUnzipStorage, m_CrtSequence - 1, &ze );
	if( result != ZR_OK )
	{
		stringstream errorMessage;
		errorMessage << "An error occured while reading zip entries [" << result << "]";
		throw runtime_error( errorMessage.str() );
	}
	
	ManagedBuffer* outputBuffer = new ManagedBuffer( NULL, ManagedBuffer::Copy, ze.unc_size );
	WorkItem< ManagedBuffer > managedOutputBuffer( outputBuffer );

	result = UnzipItem( m_CrtUnzipStorage, m_CrtSequence - 1, outputBuffer->buffer(), outputBuffer->size() );
	if( result != ZR_OK )
	{
		stringstream errorMessage;
		errorMessage << "An error occured while reading unzipping entries [" << result << "]";
		throw runtime_error( errorMessage.str() );
	}

	// create item 
	BatchItem item;
	item.setBatchId( m_CrtStorageId );
	item.setSequence( m_CrtSequence );
	item.setBinPayload( outputBuffer );
	item.setEyecatcher( ze.name );
	if ( m_CrtSequence >= m_NumberOfItems )
		item.setLast( true );

	m_CrtSequence++;

	return item;
}

void BatchZipStorage::close( const string& storageId )
{
	if ( m_CrtUnzipStorage != 0 )
	{
		ZRESULT result = CloseZip( m_CrtUnzipStorage );
		if( result != ZR_OK )
		{
			stringstream errorMessage;
			errorMessage << "An error occured while closing unzip handle [" << result << "]";
			TRACE( errorMessage.str() );
		}
		m_CrtUnzipStorage = 0;
	}
	if ( m_CrtZipStorage != 0 )
	{
		ZRESULT result = CloseZip( m_CrtZipStorage );
		if( result != ZR_OK )
		{
			stringstream errorMessage;
			errorMessage << "An error occured while closing zip handle [" << result << "]";
			TRACE( errorMessage.str() );
		}
		m_CrtZipStorage = 0;
	}
}

// open storage
void BatchZipStorage::open( const string& storageId, ios_base::openmode openMode )
{
	// if the crt. storage is open ...
	m_CrtSequence = BatchItem::FIRST_IN_SEQUENCE;
	m_CrtStorageId = storageId;

	// if openmode = inputMode    => unzip
	// if openmode = outputMode   => zip
	
	if ( ( openMode & ios_base::out ) == ios_base::out )
	{
		string batchTempFilename = "tempBatch" + m_CrtStorageId;
		m_CrtZipStorage = CreateZip( batchTempFilename.c_str(), NULL );
	}
}

ManagedBuffer* BatchZipStorage::getBuffer()
{
	string batchTempFilename = "tempBatch" + m_CrtStorageId;
	ifstream inf( batchTempFilename.c_str(), ios_base::in | ios_base::binary | ios::ate );
	long currentFileSize = inf.tellg();
	inf.seekg( 0, ios::beg );

	unsigned char* buffer = new unsigned char[ currentFileSize ];
	inf.read( ( char * )buffer, currentFileSize );

	ManagedBuffer* retBuffer = new ManagedBuffer( buffer, ManagedBuffer::Adopt, currentFileSize );

	return retBuffer;
}

void BatchZipStorage::setBuffer( const unsigned char *inputBuffer, const unsigned int isize ) 
{
	m_CrtUnzipStorage = OpenZip( ( void* )inputBuffer, isize, NULL );
	
	ZIPENTRY ze;
	ZRESULT result = GetZipItem( m_CrtUnzipStorage, -1, &ze );
	if( result != ZR_OK )
	{
		stringstream errorMessage;
		errorMessage << "An error occured while reading zip entries [" << result << "]";
		throw runtime_error( errorMessage.str() );
	}
	m_NumberOfItems = ze.index;
}
void BatchZipStorage::removeTemp()
{
	string batchTempFilename = "tempBatch" + m_CrtStorageId;
	
	if ( remove( batchTempFilename.c_str() ) != 0 )
	{
		int errCode = errno;
		stringstream errorMessage;
		
#ifdef CRT_SECURE
		char errBuffer2[ 95 ];
		strerror_s( errBuffer2, sizeof( errBuffer2 ), errCode );
		errorMessage << "Remove temp zipfile [" << batchTempFilename << "] failed with code : " << errCode << " " << errBuffer2;
#else
		errorMessage << "Remove temp zipfile [" << batchTempFilename << "] failed with code : " << errCode << " " << strerror( errCode );
#endif	

		TRACE( errorMessage.str() );
	}
}
