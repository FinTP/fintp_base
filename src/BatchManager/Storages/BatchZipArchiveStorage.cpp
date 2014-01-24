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

#include "BatchZipArchiveStorage.h"

#include <sstream>
#include <errno.h>
#include <iostream>

#include "XmlUtil.h"
#include "Trace.h"

using namespace FinTP;

BatchZipArchiveStorage::BatchZipArchiveStorage() : BatchStorageBase(), m_CrtSequence( 0 ), m_NumberOfItems( 0 ), m_getFirstIndex( -1 )
{
}

BatchZipArchiveStorage::~BatchZipArchiveStorage()
{
	try
	{
		m_CrtZipStorage.Close();
	}
	catch( ... ){}

	try
	{
		m_CrtZipMemFile.Close();
	}
	catch( ... ){}
}

void BatchZipArchiveStorage::enqueue( BatchResolution& resolution )
{
	DEBUG( "Enqueue" );

	stringstream errorMessage;
	BatchItem item = resolution.getItem();
	BatchItem::BATCHITEM_TYPE payloadType = item.getPayloadType();
	CZipMemFile itemMemFile;
	if ( ( payloadType == item.BATCHITEM_TXT ) || ( payloadType == item.BATCHITEM_XML ) )
		itemMemFile.Write( item.getPayload().c_str(), item.getPayload().size() );
	else
		itemMemFile.Write( item.getBinPayload()->buffer(), item.getBinPayload()->size() );

	DEBUG( "Current item sequence is " << item.getSequence() );
	
	try
	{
		bool result = m_CrtZipStorage.AddNewFile( itemMemFile, item.getEyecatcher().c_str() );
	
		if ( !result )
		{
			errorMessage << "An error occured while adding items to ZIP handle [" << item.getEyecatcher().c_str() << "]";
			throw runtime_error( errorMessage.str() );
		}
		
		itemMemFile.Close();
	}
	catch( CZipException& ex )
	{
		errorMessage << "An error occured while adding items to ZIP [" << ex.GetErrorDescription() << "]";
	
		m_CrtZipStorage.CloseNewFile( true );
		if ( !itemMemFile.IsClosed() )
			itemMemFile.Close();
		throw runtime_error( errorMessage.str() );
	}
	catch( ... )
	{
		m_CrtZipStorage.CloseNewFile( true );
		if ( !itemMemFile.IsClosed() )
			itemMemFile.Close();
		throw;
	}
	m_CrtSequence++;
	DEBUG( "Next sequence is " << m_CrtSequence );
}

BatchItem BatchZipArchiveStorage::dequeue()
{
	DEBUG( "Dequeue" );

	stringstream errorMessage;
	CZipMemFile ze;
	BatchItem item;
	BYTE* b = NULL;
	int zeLen = 0;
	bool result = false;
	try
	{
		if ( m_getFirstIndex > -1 )
		{
			result = m_CrtZipStorage.ExtractFile( m_getFirstIndex, ze );
			m_getFirstIndex = -1;
		}
		else
		{
			result = m_CrtZipStorage.ExtractFile( m_CrtSequence - 1, ze );
		}

		if( !result )
		{
			errorMessage << "An error occured while unzipping entry #[" << m_CrtSequence - 1 << "]";
			throw runtime_error( errorMessage.str() );
		}

		zeLen = ze.GetLength();
		b = new BYTE[ zeLen ];
		ze.Seek( 0, CZipAbstractFile::begin );
		int len = ze.Read( b, zeLen );

		WorkItem< ManagedBuffer > entryItemBuffer (new ManagedBuffer( b, ManagedBuffer::Adopt, zeLen ));
		ManagedBuffer* entryBuffer = entryItemBuffer.get();

		CZipFileHeader* fileHeader = m_CrtZipStorage.GetFileInfo( m_CrtSequence - 1 );
		CZipString fileName = fileHeader->GetFileName();

		// create item 
		// item.setBatchId( m_CrtStorageId );
		item.setSequence( m_CrtSequence );
		item.setBinPayload( entryBuffer );
		item.setEyecatcher( fileName );
		if ( m_CrtSequence >= m_NumberOfItems )
			item.setLast( true );
	}
	catch( CZipException& ex )
	{
		if ( !ze.IsClosed() )
			ze.Close();
		if( b != NULL )
			delete[] b;
		errorMessage << "An error occured while reading zip entries [" << ex.GetErrorDescription() << "]";
		throw runtime_error( errorMessage.str() );
	}
	catch( ... )
	{
		if( b != NULL )
			delete[] b;
		if ( !ze.IsClosed() )
			ze.Close();
		throw;
	}
	
	m_CrtSequence++;

	return item;
}

void BatchZipArchiveStorage::close( const string& storageId )
{
	try
	{
		if ( !m_CrtZipStorage.IsClosed() )
		{
			m_CrtZipStorage.Close();
		}
		m_CrtZipMemFile.Close();
	}
	catch( CZipException& ex )
	{
		m_CrtZipStorage.Close( CZipArchive::afAfterException );
		m_CrtZipMemFile.Close();
		stringstream errorMessage;
		errorMessage << "An error occured while closing zip handle [" << ex.GetErrorDescription() << "]";
		throw runtime_error( errorMessage.str() );
	}
	catch( ... )
	{
		m_CrtZipStorage.Close( CZipArchive::afAfterException );
		m_CrtZipMemFile.Close();
		throw;
	}
}

// open storage
void BatchZipArchiveStorage::open( const string& storageId, ios_base::openmode openMode )
{	
	stringstream errorMessage;
	m_CrtSequence = BatchItem::FIRST_IN_SEQUENCE;
	m_CrtStorageId = storageId;
	// if openmode = inputMode    => unzip
	// if openmode = outputMode   => zip
	try
	{
		if ( ( openMode & ios_base::out ) == ios_base::out )
		{
			m_CrtZipMemFile.ReInit();
			bool result = m_CrtZipStorage.Open( m_CrtZipMemFile, CZipArchive::zipCreate );
			if( !result )
			{
				errorMessage << "An error occured while creating zip handle";
				throw runtime_error( errorMessage.str() );
			}
		}
		else if ( ( openMode & ios_base::in ) == ios_base::in )
		{	
			if ( m_CrtZipStorage.IsClosed() )
			{
				bool result = m_CrtZipStorage.Open( m_CrtZipMemFile, CZipArchive::zipOpen );
				if( !result )
				{
					errorMessage << "An error occured while opening zip handle";
					throw runtime_error( errorMessage.str() );
				}
			}
		}
		else
		{
			return;
		}
		bool setCompatibility = m_CrtZipStorage.SetSystemCompatibility( ZipCompatibility::zcNtfs );
	}
	catch( CZipException& ex )
	{
		m_CrtZipStorage.Close( CZipArchive::afAfterException );
		errorMessage << "An error occured while opening zip handle [" << ex.GetErrorDescription() << "]";
		throw runtime_error( errorMessage.str() );
	}
	catch( ... )
	{
		m_CrtZipStorage.Close( CZipArchive::afAfterException );
		throw;
	}	
}

ManagedBuffer* BatchZipArchiveStorage::getBuffer()
{
	//bool result = m_CrtZipStorage.Open( m_CrtZipMemFile, CZipArchive::zipOpen );
	BYTE* b = NULL;
	int mfLen = 0;
	
	try
	{
		if ( !m_CrtZipStorage.IsClosed() )
			m_CrtZipStorage.Close();
		mfLen = m_CrtZipMemFile.GetLength();
		b = new BYTE[ mfLen ];
		
		m_CrtZipMemFile.Seek( 0, CZipAbstractFile::begin );
		int len = m_CrtZipMemFile.Read( b, mfLen );
	}
	catch( CZipException& ex )
	{
		if( b != NULL )
			delete[] b;

		m_CrtZipStorage.Close( CZipArchive::afAfterException );
		stringstream errorMessage ;
		errorMessage << "An error occured while reading zip  [" << ex.GetErrorDescription() << "]";
		throw runtime_error( errorMessage.str() );
	}
	catch( ... )
	{
		if( b != NULL )
			delete[] b;

		m_CrtZipStorage.Close( CZipArchive::afAfterException );	
		throw;
	}

	ManagedBuffer* newBuffer = NULL;
	try
	{
		/*
		if( mfLen > MAX_MESSAGE_LEN )
			newBuffer = new ManagedBuffer( b, ManagedBuffer::Adopt, mfLen, mfLen+1 );
		else
		*/
			newBuffer = new ManagedBuffer( b, ManagedBuffer::Adopt, mfLen );
	}
	catch( ... )
	{
		if ( newBuffer != NULL )
			delete newBuffer;
		throw;
	}
	return newBuffer;
}

void BatchZipArchiveStorage::setBuffer( const unsigned char *inputBuffer, const unsigned int isize ) 
{
	stringstream errorMessage;
	bool result = false;
	CZipMemFile mf;
	try
	{
		if ( !m_CrtZipStorage.IsClosed() )
		{
			m_CrtZipStorage.Close();
		}
		/*  reinit to clean m_CrtZipMemFile*/
		m_CrtZipMemFile.ReInit();
		// typedef unsigned char BYTE
		m_CrtZipMemFile.Write( inputBuffer, isize );

		result = m_CrtZipStorage.Open( m_CrtZipMemFile, CZipArchive::zipOpen );
		if( !result)
		{
			errorMessage << "An error occured while reading zip entries ["  "]";
			throw runtime_error( errorMessage.str() );
		}
		m_NumberOfItems = m_CrtZipStorage.GetCount();
		m_CrtZipStorage.Close();
	}
	catch( CZipException& ex )
	{
		m_CrtZipStorage.Close( CZipArchive::afAfterException );
		m_CrtZipMemFile.Close();
		errorMessage << "An error occured while reading zip entries [" << ex.GetErrorDescription() << "]";
		throw runtime_error( errorMessage.str() );
	}
	catch( ... )
	{
		m_CrtZipStorage.Close( CZipArchive::afAfterException );
		m_CrtZipMemFile.Close();
		throw;
	}
}
void BatchZipArchiveStorage::setDequeFirst( const string& findSubstr ) 
{
	stringstream errorMessage;
	bool result = false;
	try
	{
		if ( !m_CrtZipStorage.IsClosed() )
		{
			m_CrtZipStorage.Close();
		}
		/*  close then close free with m_Autodelete = true */

		result = m_CrtZipStorage.Open( m_CrtZipMemFile, CZipArchive::zipOpen );
		if( !result)
		{
			errorMessage << "An error occured while reading zip entries ["  "]";
			throw runtime_error( errorMessage.str() );
		}
		CZipFileHeader info ;
		CZipString fileName = "";
		for (int i = 0; i < m_NumberOfItems; i++)
		{
			m_CrtZipStorage.GetFileInfo(info, i);
			fileName = info.GetFileName();
			if ( fileName.Find( findSubstr.c_str() ) )
			{
				m_getFirstIndex = i;
				break;
			}
		}
		m_CrtZipStorage.Close();

	}
	catch( CZipException& ex )
	{
		m_CrtZipStorage.Close( CZipArchive::afAfterException );
		m_CrtZipMemFile.Close();
		errorMessage << "An error occured while reading zip entries [" << ex.GetErrorDescription() << "]";
		throw runtime_error( errorMessage.str() );
	}
	catch( ... )
	{
		m_CrtZipStorage.Close( CZipArchive::afAfterException );
		m_CrtZipMemFile.Close();
		throw;
	}
}
bool BatchZipArchiveStorage::isDequeFirst()
{
	bool returnValue = false;
	if ( m_getFirstIndex > -1 )
		returnValue = true;

	return returnValue;
}

