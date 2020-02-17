#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>
#include <math.h>
#include <d3d.h>
#include <io.h>
#include <conio.h>
#include <direct.h>

#define NOFILES		-1

//-----------------------------------------------------------------------------
// Name: void main( void )
// Desc: 메인 함수
//-----------------------------------------------------------------------------
void main( void )
{
	_finddata_t fileinfo;

	char	start = 0;
	long	handle, TickCount;
	char	Filename[13] = "*.tga";
	int first = 0;
	int	filecnt = 0;
	int cnt = 0;
	int size;
	char	temp[FILENAME_MAX];
	char	temp2[FILENAME_MAX], pathname[FILENAME_MAX];

	int i;
	FILE *fp;

	fp = fopen( "filelist.lst", "wt" );

	i = 0;
	// 현재 폴더에서 *.TGA 파일을 최초에 검색한다.
	if( (handle = _findfirst( Filename, &fileinfo ) ) == NOFILES )
	{
		printf( "------------- TGA Files Not Found !!! -----------------\n\n");
		while( !_kbhit() );

		exit(0);
	}

	while( 1 )
	{
		printf( "-------- %s file Now Checking..............\n\n", fileinfo.name );

		fprintf( fp, "%s\n", fileinfo.name );
		i++;
		
		// 계속 다음 TGA 파일을 검사
		if( (_findnext( handle, &fileinfo ) ) == NOFILES )
			break;
	}

	// 모든 TGA 파일 검사 완료
	_findclose( handle );

	fclose( fp );

//	strcpy( temp, "smackerw.exe batch.sbt" );

	strcpy( temp, "GraphPr filelist.lst pcxfiles.pcx /T2/D2/O\n" );

	system( temp );

	printf( "No Errors ...................... Next Renaming .........................\n" );

	fp = fopen( "filelist.lst", "rt" );

	int counter = 0;

	while( 1 )
	{
		if ( fscanf( fp, "%s\n", temp ) == EOF )
			break;

		temp[strlen(temp)-4] = NULL;
		strcat( temp, ".pcx" );

		if ( i/10000 > 0 )
			sprintf( temp2, "pcxfiles%05d.pcx", counter );
		else
		if ( i/1000 > 0 )
			sprintf( temp2, "pcxfiles%04d.pcx", counter );
		else
		if ( i/100 > 0 )
			sprintf( temp2, "pcxfiles%03d.pcx", counter );
		else
		if ( i/10 > 0 )
			sprintf( temp2, "pcxfiles%02d.pcx", counter );
		else
			sprintf( temp2, "pcxfiles%d.pcx", counter );

		printf( "-------- %s -> %s Renaming..............\n\n", temp2, temp );

		remove( temp );
		rename( temp2, temp );

		counter++;
	}

	TickCount = GetTickCount();		

	while( GetTickCount() - TickCount < 1000 ){};
}

