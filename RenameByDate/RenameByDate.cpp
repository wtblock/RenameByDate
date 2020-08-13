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
// given an image pointer and an ASCII property ID, return the property value
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
// The date and time when the original image data was generated.
// For a digital still camera, this is the date and time the picture 
// was taken or recorded. The format is "YYYY:MM:DD HH:MM:SS" with time 
// shown in 24-hour format, and the date and time separated by one blank 
// character (hex 20).
// this method will set the m_Date member using a Date Taken formatted
// string
void CDate::SetDateTaken( CString csDate )
{
	// reset the m_Date to undefined state
	Year = -1;
	Month = -1;
	Day = -1;
	Hour = 0;
	Minute = 0;
	Second = 0;
	bool value = Okay;

	// parse the date into a vector of string tokens
	const CString csDelim( _T( ": " ) );
	int nStart = 0;
	vector<CString> tokens;

	do
	{
		const CString csToken = 
				csDate.Tokenize( csDelim, nStart ).MakeLower();
		if ( csToken.IsEmpty() )
		{
			break;
		}

		tokens.push_back( csToken );

	} while ( true );

	// there should be six tokens in the proper format of
	// "YYYY:MM:DD HH:MM:SS"
	const size_t tTokens = tokens.size();
	if ( tTokens != 6 )
	{
		return;
	}

	// populate the date and time members with the values
	// in the vector
	TOKEN_NAME eToken = tnYear;
	int nToken = 0;

	for ( CString csToken : tokens )
	{
		int nValue = _tstol( csToken );

		switch ( eToken )
		{
			case tnYear:
			{
				Year = nValue;
				break;
			}
			case tnMonth:
			{
				Month = nValue;
				break;
			}
			case tnDay:
			{
				Day = nValue;
				break;
			}
			case tnHour:
			{
				Hour = nValue;
				break;
			}
			case tnMinute:
			{
				Minute = nValue;
				break;
			}
			case tnSecond:
			{
				Second = nValue;
				break;
			}
		}

		nToken++;
		eToken = (TOKEN_NAME)nToken;
	}

	// this will be true if all of the values define a proper date and time
	value = Okay;

} // SetDateTaken

/////////////////////////////////////////////////////////////////////////////
// get the current date taken, if any, from the given filename
CString GetCurrentDateTaken( LPCTSTR lpszPathName )
{
	USES_CONVERSION;

	CString value;

	// smart pointer to the image representing this file
	unique_ptr<Gdiplus::Image> pImage =
		unique_ptr<Gdiplus::Image>
		(
			Gdiplus::Image::FromFile( T2CW( lpszPathName ) )
		);

	// test the date properties stored in the given image
	CString csOriginal =
		GetStringProperty( pImage.get(), PropertyTagExifDTOrig );
	CString csDigitized =
		GetStringProperty( pImage.get(), PropertyTagExifDTDigitized );

	// officially the original property is the date taken in this
	// format: "YYYY:MM:DD HH:MM:SS"
	m_Date.DateTaken = csOriginal;
	if ( m_Date.Okay )
	{
		value = csOriginal;

	} else // alternately use the date digitized
	{
		m_Date.DateTaken = csDigitized;
		if ( m_Date.Okay )
		{
			value = csDigitized;
		}
	}

	return value;
} // GetCurrentDateTaken

/////////////////////////////////////////////////////////////////////////////
// rename the given file by the date time properties in CDate member class
// called m_Date.
CString RenameFile( LPCTSTR lpszPathName )
{
	CString value;
	const CString csFolder = GetFolder( lpszPathName );
	const CString csExtension = GetExtension( lpszPathName );
	COleDateTime oDT = m_Date.DateAndTime;
	const CString csDate = oDT.Format( _T( "%Y_%m_%d_%H_%M_%S" ));

	int nCount = 0;

	// loop until a unique filename is generated
	do 
	{
		// the first time through, do the normal generation
		if ( nCount++ == 0 )
		{
			value.Format( _T( "%s%s%s" ), csFolder, csDate, csExtension );

		} else // after the first attempt, add the count to the filename
		{
			value.Format( _T( "%s%s_%02d%s" ), csFolder, csDate, nCount, csExtension );
		}

		// if the original pathname matches the generated name
		// there is nothing to do
		if ( value == lpszPathName )
		{
			return value;
		}

		// if the path does not exist, break out of the loop
		if ( !::PathFileExists( value ))
		{
			break;
		}

	} while ( true );

	// rename the file to the generated value
	try
	{
		CFile::Rename( lpszPathName, value );

	// if the rename fails for any reason, return a blank value
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

	// get the folder which will trim any wild card data
	CString csPathname = GetFolder( path );

	// wild cards are in use if the pathname does not equal the given path
	const bool bWildCards = csPathname != path;
	csPathname.TrimRight( _T( "\\" ) );
	CString csData;

	// build a string with wild-cards
	CString strWildcard;
	if ( bWildCards )
	{
		csData = GetDataName( path );
		strWildcard.Format( _T( "%s\\%s" ), csPathname, csData );

	} else // no wild cards, just a folder
	{
		strWildcard.Format( _T( "%s\\*.*" ), csPathname );
	}

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
			// do not recurse into the corrected folder
			const CString str = 
				finder.GetFilePath().TrimRight( _T( "\\" ) );;
			if ( str.Right( nCorrected ) == csCorrected )
			{
				continue;
			}

			// if wild cards are in use, build a path with the wild cards
			if ( bWildCards )
			{
				CString csPath;
				csPath.Format( _T( "%s\\%s" ), str, csData );

				// recurse into the new directory with wild cards
				RecursePath( csPath );

			} else // recurse into the new directory
			{
				RecursePath( str );
			}

		} else // write the properties if it is a valid extension
		{
			const CString csPath = finder.GetFilePath();
			const CString csExt = GetExtension( csPath ).MakeLower();
			const CString csFile = GetFileName( csPath );

			if ( -1 != csValidExt.Find( csExt ) )
			{
				m_Extension.FileExtension = csExt;

				CStdioFile fout( stdout );
				fout.WriteString( csPath + _T( "\n" ) );

				// get the file's Date Taken metadata first
				const CString csDateTaken = GetCurrentDateTaken( csPath );

				// modify our date/time information with the modified time
				// of this file. This is to keep the names unique and in the
				// original sequence, but depending on the source of the image
				// will probably not be the same as the date taken which
				// is unknown if csDateTaken is empty.
				if ( csDateTaken.IsEmpty())
				{
					fout.WriteString
					(
						_T( "\n" )
						_T( "Missing Date Taken metadata.\n" )
						_T( "\n" )
					);
					continue;
				}

				// if we are using the file's Date Taken metadata
				if ( m_bUseDateTaken )
				{
					// set the date by the date taken
					m_Date.Date = csDateTaken;

				} else // use the parameter date information
				{
					// protect the command line date information
					const int nYear = m_Date.Year;
					const int nMonth = m_Date.Month;
					const int nDay = m_Date.Year;

					// set the date by the date taken
					m_Date.Date = csDateTaken;

					// restore the command line date information
					m_Date.Year = nYear;
					m_Date.Month = nMonth;
					m_Date.Day = nDay;
				}

				// get the date and time from the member date class which
				// now contains the parameter date information as well 
				// as the original modification time.
				COleDateTime oDT = m_Date.DateAndTime;
				COleDateTime::DateTimeStatus eStatus = oDT.GetStatus();

				// error out if the date / time data is invalid
				if ( eStatus != COleDateTime::valid )
				{
					fout.WriteString
					(
						_T( "\n" )
						_T( "Invalid date and time.\n" )
						_T( "\n" )
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
						_T( "\nThe file could not be renamed.\n\n" )
					);
					continue;

				} else if ( csNew == csPath ) // the filename did no change
				{
					const CString csData = GetDataName( csNew );
					csOutput.Format( _T( "Filename unchanged: %s\n" ), csData );

				} else // let the user know about the new filename
				{
					const CString csData = GetDataName( csNew );
					csOutput.Format( _T( "File renamed to: %s\n" ), csData );
				}

				fout.WriteString( csOutput );

				// if we are using the file's Date Taken metadata, then we do
				// not need to create a corrected file with a new Date Taken
				if ( m_bUseDateTaken )
				{
					continue;
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

	// five arguments if specifying year, month, and day
	// two arguments if using the existing date taken
	if ( argc != 5 && argc != 2 )
	{
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString
		(
			_T( "RenameByDate, Copyright (c) 2020, " )
			_T( "by W. T. Block.\n" )
		);

		fOut.WriteString
		( 
			_T( ".\n" ) 
			_T( "Usage:\n" )
			_T( ".\n" )
			_T( ".  RenameByDate pathname [year month day]\n" )
			_T( ".\n" )
			_T( "Where:\n" )
			_T( ".\n" )
		);

		fOut.WriteString
		(
			_T( ".  pathname is the root of the tree to be scanned, but\n" )
			_T( ".  may contain wild cards like the following:\n" )
			_T( ".    \"c:\\Picture\\DisneyWorldMary2 *.JPG\"\n" )
			_T( ".  will process all files with that pattern, or\n" )
			_T( ".    \"c:\\Picture\\DisneyWorldMary2 231.JPG\"\n" )
			_T( ".  will process a single defined image file.\n" )
			_T( ".  (NOTE: using wild cards will prevent recursion\n" )
			_T( ".    into sub-folders because the folders will likely\n" )
			_T( ".    not fall into the same pattern and therefore\n" )
			_T( ".    sub-folders will not be found by the search).\n" )
		);

		fOut.WriteString
		(
			_T( ".  year, month, and day are optional:\n" )
			_T( ".    if one is specified they all must\n" )
			_T( ".      be specified.\n" )
			_T( ".    if they are not specified, the date\n" )
			_T( ".      taken metadata will be used instead.\n" )
			_T( ".  year is the four digit year of the date\n" )
			_T( ".  month is the month of the year (1..12)\n" )
			_T( ".  day is the day of the month (1..31)\n" )
			_T( ".\n" )
			_T( ".The program will error out if the date is invalid, \n" )
			_T( ".  for example day 31 in the month of September, or\n" )
			_T( ".  day 29 of February in a non-leap year.\n" )
			_T( ".\n" )
		);
		return 3;
	}

	// display the executable path
	CString csMessage;
	//csMessage.Format( _T( "Executable pathname: %s\n" ), argv[ 0 ] );
	//fOut.WriteString( _T( ".\n" ) );
	//fOut.WriteString( csMessage );
	//fOut.WriteString( _T( ".\n" ) );

	// retrieve the pathname which may include wild cards
	CString csPath = argv[ 1 ];

	// trim off any wild card data
	const CString csFolder = GetFolder( csPath );

	// test for current folder character (a period)
	bool bExists = csPath == _T( "." );

	// if it is a period, add a wild card of *.* to retrieve
	// all folders and files
	if ( bExists )
	{
		csPath = _T( ".\\*.*" );

	// if it is not a period, test to see if the folder exists
	} else
	{
		if ( ::PathFileExists( csFolder ) )
		{
			bExists = true;
		}
	}

	if ( !bExists )
	{
		csMessage.Format( _T( "Invalid pathname:\n\t%s\n" ), csPath );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( csMessage );
		fOut.WriteString( _T( ".\n" ) );
		return 4;

	} else
	{
		csMessage.Format( _T( "Given pathname:\n\t%s\n" ), csPath );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( csMessage );
	}

	// use Date Taken is true if there are only two parameters
	m_bUseDateTaken = argc == 2;
	if ( m_bUseDateTaken )
	{
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString
		( 
			_T( ".\n" )
			_T( "No date provided, using Date Taken\n" )
			_T( "from each file instead, which means\n" )
			_T( "there will not be a \"Corrected\" sub-\n" )
			_T( "folder since no changes will be made\n" )
			_T( "to the existing file's Date Taken.\n" )
			_T( ".\n" )
		);

	} else // year, month and day were provided
	{
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
			// let the user know the parameters did not make
			// a valid date and error out
			csMessage.Format
			(
				_T( "Invalid date parameter(s) Year:" )
				_T( " %d, Month: %d, Day: %d\n" ),
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
			fOut.WriteString( csMessage );
			fOut.WriteString( _T( ".\n" ) );
		}
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
