/////////////////////////////////////////////////////////////////////////////
// Copyright © by W. T. Block, all rights reserved
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RenameByDate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object
CWinApp theApp;

/////////////////////////////////////////////////////////////////////////////
// given an image pointer and an ASCII property ID, return the propery value
CString GetStringProperty( Gdiplus::Image* pImage, PROPID id )
{
	CString value;

	// get the size of the date property
	const UINT uiSize = pImage->GetPropertyItemSize( id );

	// if the property exists, it will have a non-zero size 
	if ( uiSize > 0 )
	{
		// using a smart pointer which will release itself
		// when it goes out of context
		unique_ptr<Gdiplus::PropertyItem> pItem =
			unique_ptr<Gdiplus::PropertyItem>
			(
				(PropertyItem*)malloc( uiSize )
			);

		// Get the property item.
		pImage->GetPropertyItem( id, uiSize, pItem.get() );

		// the property should be ASCII
		if ( pItem->type == PropertyTagTypeASCII )
		{
			value = (LPCSTR)pItem->value;
		}
	}

	return value;
} // GetStringProperty

/////////////////////////////////////////////////////////////////////////////
// rename the given file by the date time properties in CDate member class
// called m_Date.
CString RenameFile( LPCTSTR lpszPathName )
{
	CString value;
	const CString csFolder = GetFolder( lpszPathName );
	const CString csExtension = GetExtension( lpszPathName );
	const CString csDate = m_Date.Date;
	const CString csPath = csFolder + csDate + csExtension;
	try
	{
		CFile::Rename( lpszPathName, csPath );
		value = csPath;

	} catch (...)
	{
		value = _T( "" );
	}

	return value;
} // RenameFile

/////////////////////////////////////////////////////////////////////////////
// Save the data inside pImage to the given filename but relocated to the 
// sub-folder "Corrected"
bool Save( LPCTSTR lpszPathName, Gdiplus::Image* pImage )
{
	USES_CONVERSION;

	// test the new properties stored in the given image
	const CString csOriginal =
		GetStringProperty( pImage, PropertyTagExifDTOrig );
	const CString csDigitized =
		GetStringProperty( pImage, PropertyTagExifDTDigitized );

	// save and overwrite the selected image file with current page
	int iValue =
		EncoderValue::EncoderValueVersionGif89 |
		EncoderValue::EncoderValueCompressionLZW |
		EncoderValue::EncoderValueFlush;

	EncoderParameters param;
	param.Count = 1;
	param.Parameter[ 0 ].Guid = EncoderSaveFlag;
	param.Parameter[ 0 ].Value = &iValue;
	param.Parameter[ 0 ].Type = EncoderParameterValueTypeLong;
	param.Parameter[ 0 ].NumberOfValues = 1;

	// writing to the same file will fail, so save to a corrected folder
	// below the image being corrected
	const CString csCorrected = GetCorrectedFolder();
	const CString csFolder = GetFolder( lpszPathName ) + csCorrected;
	if ( !::PathFileExists( csFolder ) )
	{
		if ( !CreatePath( csFolder ) )
		{
			return false;
		}
	}

	// filename plus extension
	const CString csData = GetDataName( lpszPathName );

	// create a new path from the pieces
	const CString csPath = csFolder + _T( "\\" ) + csData;

	// use the extension member class to get the class ID of the file
	CLSID clsid = m_Extension.ClassID;

	// save the image to the corrected folder
	Status status = pImage->Save( T2CW( csPath ), &clsid, &param );

	// return true if the save worked
	return status == Ok;
} // Save

/////////////////////////////////////////////////////////////////////////////
// crawl through the directory tree looking for supported image extensions
void RecursePath( LPCTSTR path )
{
	USES_CONVERSION;

	// valid file extensions
	const CString csValidExt = _T( ".jpg;.jpeg;.png;.gif;.bmp;.tif;.tiff" );

	// the new folder under the image folder to contain the corrected images
	const CString csCorrected = GetCorrectedFolder();
	const int nCorrected = GetCorrectedFolderLength();

	CString csPathname( path );
	csPathname.TrimRight( _T( "\\" ) );

	// build a string with wildcards
	CString strWildcard;
	strWildcard.Format( _T( "%s\\*.*" ), path );

	// start trolling for files we are interested in
	CFileFind finder;
	BOOL bWorking = finder.FindFile( strWildcard );
	while ( bWorking )
	{
		bWorking = finder.FindNextFile();

		// skip "." and ".." folder names
		if ( finder.IsDots() )
		{
			continue;
		}

		// if it's a directory, recursively search it
		if ( finder.IsDirectory() )
		{
			const CString str = finder.GetFilePath();

			// do not recurse into the corrected folder
			if ( str.Right( nCorrected ) == csCorrected )
			{
				continue;
			}

			// recurse into the new directory
			RecursePath( str );

		} else // write the properties if it is a valid extension
		{
			const CString csPath = finder.GetFilePath();
			const CString csExt = GetExtension( csPath ).MakeLower();

			if ( -1 != csValidExt.Find( csExt ) )
			{
				m_Extension.FileExtension = csExt;

				CStdioFile fout( stdout );
				fout.WriteString( csPath + _T( "\n" ) );

				// modify our date/time information with the modified time
				// of this file. This is to keep the names unique and in the
				// orginal sequence, but depending on the source of the image
				// will probably not be the same as the time taken which
				// is unknown.
				{
					// the file's status contains the information we are 
					// looking for.
					CFileStatus fs;

					// if successful, write the modification time to the
					// member date class
					if ( CFile::GetStatus( csPath, fs ))
					{
						m_Date.Hour = fs.m_mtime.GetHour();
						m_Date.Minute = fs.m_mtime.GetMinute();
						m_Date.Second = fs.m_mtime.GetSecond();
					}
				}

				// get the date and time from the member date class which
				// now contains the parameter date information as well 
				// as the orginal modification time.
				COleDateTime oDT = m_Date.DateAndTime;
				COleDateTime::DateTimeStatus eStatus = oDT.GetStatus();

				// error out if the date / time data is invalid
				if ( eStatus != COleDateTime::valid )
				{
					fout.WriteString
					(
						_T( "Invalid time extracted from filename.\n" )
					);
					continue;
				}

				// rename the file and return the new path or an empty string 
				// on failure
				CString csOutput;
				const CString csNew = RenameFile( csPath );
				if ( csNew.IsEmpty())
				{
					fout.WriteString
					(
						_T( "The file could not be renamed.\n" )
					);
					continue;

				} else // let the user know about the new filename
				{
					const CString csData = GetDataName( csNew );
					csOutput.Format( _T( "File renamed to: %s\n" ), csData );
					fout.WriteString( csOutput );
				}

				// this formatted date will be written into the date
				// properties of the new file
				CString csDate = oDT.Format( _T( "%Y:%m:%d %H:%M:%S" ) );

				// update the user about the date being used
				csOutput.Format( _T( "New Date: %s\n" ), csDate );
				fout.WriteString( csOutput );

				// smart pointer to the image representing this file
				unique_ptr<Gdiplus::Image> pImage =
					unique_ptr<Gdiplus::Image>
					(
						Gdiplus::Image::FromFile( T2CW( csNew ) )
					);

				// smart pointer to the original date property item
				unique_ptr<Gdiplus::PropertyItem> pOriginalDateItem =
					unique_ptr<Gdiplus::PropertyItem>( new Gdiplus::PropertyItem );
				pOriginalDateItem->id = PropertyTagExifDTOrig;
				pOriginalDateItem->type = PropertyTagTypeASCII;
				pOriginalDateItem->length = csDate.GetLength() + 1;
				pOriginalDateItem->value = csDate.GetBuffer( pOriginalDateItem->length );

				// smart pointer to the digitized date property item
				unique_ptr<Gdiplus::PropertyItem> pDigitizedDateItem =
					unique_ptr<Gdiplus::PropertyItem>( new Gdiplus::PropertyItem );
				pDigitizedDateItem->id = PropertyTagExifDTDigitized;
				pDigitizedDateItem->type = PropertyTagTypeASCII;
				pDigitizedDateItem->length = csDate.GetLength() + 1;
				pDigitizedDateItem->value = csDate.GetBuffer( pDigitizedDateItem->length );

				// if these properties exist they will be replaced
				// if these properties do not exist they will be created
				Gdiplus::Status eOriginal = 
					pImage->SetPropertyItem( pOriginalDateItem.get() );
				Gdiplus::Status eDigitized = 
					pImage->SetPropertyItem( pDigitizedDateItem.get() );

				// save the image to the new path
				Save( csNew, pImage.get() );

				// release the date buffer
				csDate.ReleaseBuffer();
			}
		}
	}

	finder.Close();

} // RecursePath

/////////////////////////////////////////////////////////////////////////////
// set the current file extension which will automatically lookup the
// related mime type and class ID and set their respective properties
void CExtension::SetFileExtension( CString value )
{
	m_csFileExtension = value;

	if ( m_mapExtensions.Exists[ value ] )
	{
		MimeType = *m_mapExtensions.find( value );

		// populate the mime type map the first time it is referenced
		if ( m_mapMimeTypes.Count == 0 )
		{
			UINT num = 0;
			UINT size = 0;

			// gets the number of available image encoders and 
			// the total size of the array
			Gdiplus::GetImageEncodersSize( &num, &size );
			if ( size == 0 )
			{
				return;
			}

			ImageCodecInfo* pImageCodecInfo =
				(ImageCodecInfo*)malloc( size );
			if ( pImageCodecInfo == nullptr )
			{
				return;
			}

			// Returns an array of ImageCodecInfo objects that contain 
			// information about the image encoders built into GDI+.
			Gdiplus::GetImageEncoders( num, size, pImageCodecInfo );

			// populate the map of mime types the first time it is 
			// needed
			for ( UINT nIndex = 0; nIndex < num; ++nIndex )
			{
				CString csKey;
				csKey = CW2A( pImageCodecInfo[ nIndex ].MimeType );
				CLSID classID = pImageCodecInfo[ nIndex ].Clsid;
				m_mapMimeTypes.add( csKey, new CLSID( classID ) );
			}

			// clean up
			free( pImageCodecInfo );
		}

		ClassID = *m_mapMimeTypes.find( MimeType );

	} else
	{
		MimeType = _T( "" );
	}
} // CExtension::SetFileExtension

/////////////////////////////////////////////////////////////////////////////
// a console application that can crawl through the file
// system and troll for image metadata properties
int _tmain( int argc, TCHAR* argv[], TCHAR* envp[] )
{
	HMODULE hModule = ::GetModuleHandle( NULL );
	if ( hModule == NULL )
	{
		_tprintf( _T( "Fatal Error: GetModuleHandle failed\n" ) );
		return 1;
	}

	// initialize MFC and error on failure
	if ( !AfxWinInit( hModule, NULL, ::GetCommandLine(), 0 ) )
	{
		_tprintf( _T( "Fatal Error: MFC initialization failed\n " ) );
		return 2;
	}

	CStdioFile fOut( stdout );
	if ( argc != 5 )
	{
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString
		(
			_T( "RenameByDate, Copyright (c) 2020, " )
			_T( "by W. T. Block.\n" )
		);
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( _T( "Usage:\n" ) );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( _T( ".  RenameByDate pathname year month day\n" ) );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( _T( "Where:\n" ) );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString
		(
			_T( ".  pathname is the root of the tree to be scanned\n" )
		);
		fOut.WriteString
		(
			_T( ".  year is the four digit year the date\n" )
			_T( ".  month is the month of the year (1..12)\n" )
			_T( ".  day is the day of the month (1..31)\n" )
			_T( ".\n" )
			_T( ".The program will error out if the date is invalid, \n" )
			_T( ".  for example day 31 in the month of September, or" )
			_T( ".  day 29 in a non-leap year.\n" )
		);
		fOut.WriteString( _T( ".\n" ) );
		return 3;
	}

	// display the executable path
	CString csMessage;
	csMessage.Format( _T( "Executable pathname: %s\n" ), argv[ 0 ] );
	fOut.WriteString( _T( ".\n" ) );
	fOut.WriteString( csMessage );
	fOut.WriteString( _T( ".\n" ) );

	// retrieve the pathname and validate the pathname exists
	CString csPath = argv[ 1 ];
	if ( !::PathFileExists( csPath ) )
	{
		csMessage.Format( _T( "Invalid pathname: %s\n" ), csPath );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( csMessage );
		fOut.WriteString( _T( ".\n" ) );
		return 4;

	} else
	{
		csMessage.Format( _T( "Given pathname: %s\n" ), csPath );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( csMessage );
		fOut.WriteString( _T( ".\n" ) );
	}


	// record the given year
	m_Date.Year = _tstol( argv[ 2 ] );

	// record the given month of the year
	m_Date.Month = _tstol( argv[ 3 ] );

	// record the given day of the month
	m_Date.Day = _tstol( argv[ 4 ] );

	// If all of the date and time information is present,
	// the Okay status of the date class will be set to true.
	if ( m_Date.Okay == false )
	{
		// let the user know the parameters did not make a valid date
		// and error out
		csMessage.Format
		( 
			_T( "Invalid date parameter(s) Year: %d, Month: %d, Day: %d\n" ), 
			m_Date.Year, m_Date.Month, m_Date.Day
		);
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( csMessage );
		fOut.WriteString( _T( ".\n" ) );
		return 5;

	} else // the given parameters worked
	{
		csMessage.Format
		( 
			_T( "The date parameters yielded: %s\n" ), 
			m_Date.Date 
		);
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( csMessage );
		fOut.WriteString( _T( ".\n" ) );
	}

	// start up COM
	AfxOleInit();
	::CoInitialize( NULL );

	// create a reference to GDI+
	InitGdiplus();

	// crawl through directory tree defined by the command line
	// parameter trolling for supported image files
	RecursePath( csPath );

	// clean up references to GDI+
	TerminateGdiplus();

	// all is good
	return 0;

} // _tmain

/////////////////////////////////////////////////////////////////////////////
