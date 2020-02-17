#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>
#include <math.h>
#include <d3d.h>
#include <io.h>
#include <conio.h>
#include <direct.h>

struct PCX_HEADER_TYPE {
	char manufacturer;
	char version;
	char encoding;
	char bits_per_pixel;
	short  sx, sy, ex, ey;
	short  hres, vres;
	char pallete[48];
	char reserved;
	char color_planes;
	short  bytes_per_line;
	short  pallete_type;
	char filler[58];
} PCX_HEADER;

struct PCXpalette {
	char r, g, b;
} palette[256];

#define __32BIT__

#define NOFILES		-1

#define	SCREEN_XSIZE	3000
#define SCREEN_YSIZE	3000

// TGA 파일을 로딩할 버퍼 
DWORD	buf[SCREEN_XSIZE*SCREEN_YSIZE*2];
DWORD	buf32[SCREEN_XSIZE*SCREEN_YSIZE*2];
BYTE	buf8[SCREEN_XSIZE*SCREEN_YSIZE];

BYTE	alpha[SCREEN_XSIZE*SCREEN_YSIZE*2];
WORD	key[SCREEN_XSIZE*SCREEN_YSIZE*2];
BYTE	buf2[SCREEN_XSIZE*SCREEN_YSIZE * 4];
WORD	KEYCOLOR;

int CutX, CutY, Xsize, Ysize, TGAxsize, TGAysize;
bool g_alphaflag = false, g_nocolorkey = false;

void DecodePCX(unsigned char *buffer, int bpl, FILE *fp)
{
	int i, data, num;

	for(i = 0; i< bpl;) {
		data = fgetc(fp);
		if (data >= 0xc0) {
			num = data & 0x3f;
			data = fgetc(fp);
			while(num--)
				buffer[i++] = data;
		}
		else buffer[i++] = data;
	}
}


void LoadPCXFile(char * fname)
{
	FILE *fp;
	char AppTitle[80];

	fp = fopen(fname,"rb");

	fread(&PCX_HEADER, sizeof(PCX_HEADER), 1, fp);

	int width = PCX_HEADER.ex - PCX_HEADER.sx + 1;
	int height = PCX_HEADER.ey - PCX_HEADER.sy + 1;

	DecodePCX(buf8, width * height, fp);
	fgetc(fp);
	fread(palette, 256 * 3, 1, fp);

	fclose( fp );
}


int ConvertSquare(int val)
{
	int count = 1;
	while( count < val ) count <<= 1;
	return count;
}

//-----------------------------------------------------------------------------
// Name: int GetSizes( int xs, int ys )
// Desc: 실제 이미지 부분만을 잘라서 압축하여 buf2 에 저장
//-----------------------------------------------------------------------------
int GetSizesAlpha( int xs, int ys )
{
	int i, j;

	// 실제 사용할 이미지의 최 상단 좌표를 얻어낸다.
	for ( j = 0; j < ys; j++ )
		for ( i = 0; i < xs; i++ )
		{
			if ( alpha[ j * xs + i ] > 0 )
			{
				CutY = j;
				goto next1;
			}
		}

next1:

		// 실제 사용할 이미지의 최좌측 좌표를 얻어낸다.
		for ( i = 0; i < xs; i++ )
			for ( j = 0; j < ys; j++ )
			{
				if ( alpha[ j * xs + i ] > 0 )
				{
					CutX = i;
					goto next2;
				}
			}

next2:

			// 실제 사용할 이미지의 가로 크기를 얻어낸다.
			for ( i = xs - 1; i > -1; i-- )
				for ( j = 0; j < ys; j++ )
				{
					if ( alpha[ j * xs + i ] > 0 )
					{
						Xsize = i - CutX + 1;
						goto next3;
					}
				}

next3:

				// 실제 사용할 이미지의 세로 크기를 얻어낸다.
				for ( j = ys - 1 ; j > -1; j-- )
					for ( i = 0; i < xs; i++ )
					{
						if ( alpha[ j * xs + i ] > 0 )
						{
							Ysize = j - CutY + 1;
							goto next4;
						}
					}

next4:
					/*	int tempXsize = ConvertSquare( Xsize ) - Xsize;
					int tempYsize = ConvertSquare( Ysize ) - Ysize;

					Xsize = ConvertSquare( Xsize );
					Ysize = ConvertSquare( Ysize );
					*/
					BYTE tempalpha, alphav, r, g, b;
					DWORD	temp;
					int cnt = 2, flag = 0, cnta = 0, cntb = 0, orgcntb = 0, set = 0, cnt4 = 0;

					// 실제 이미지 사각 영역만을 검사하여 압축하는 부분
					for ( j = CutY; j < CutY + Ysize; j++ )
					{
						for ( i = CutX; i < CutX + Xsize; i++ )
						{
							tempalpha = alpha[j * xs + i];
							temp = buf[ j * xs + i ];
							BYTE	pal = buf8[ j * xs + i ];
//							DWORD pal = buf8[ j * xs + i ] * 3;
//							DWORD temp2 = (tempalpha<<24)|(palette[pal].r<<16)|(palette[pal].g<<8)|(palette[pal].b);

							switch( flag )
							{
								// 투명 칼라 체크
							case 0:
								// 만약 투명 칼라를 계속 체크하던 중, 
								// 투명 칼라가 아닌 실제 이미지가 나왔다면, 총 투명 칼라수 저장
								if ( tempalpha > 0 )
								{
									buf2[cnt+1] = (BYTE)(cnta>>24)&0xff;
									buf2[cnt] = (BYTE)(cnta>>16)&0xff;
									buf2[cnt+3] = (BYTE)(cnta>>8)&0xff;
									buf2[cnt+2] = (BYTE)(cnta&0xff);
									cnt+=4;
									orgcntb = cnt;
									cnt+=2;
									cntb = 0;
									flag = 1;
									i--;
								}
								else
									cnta++;
								break;
								// 실제 이미지 체크
							case 1:
								// 만약 실제 이미지 수를 체크하던 중,
								// 실제 이미지가 아닌, 투명 칼라가 나왔다면, 실제 이미지 수 저장
								if ( tempalpha < 1 )
								{
									buf2[orgcntb+1] = (BYTE)(cntb>>8);
									buf2[orgcntb] = (BYTE)(cntb&0xff);
									cnta = 0;
									flag = 0;
									set++;
									i--;
								}
								else
									// 계속 실제 이미지가 계속 나오는 상태라면, 실제 이미지 계속 저장
								{
/*									buf2[cnt+3] = (BYTE)(temp>>24)&0xff;
									buf2[cnt+2] = (BYTE)(temp>>16)&0xff;
									buf2[cnt+1] = (BYTE)(temp>>8)&0xff;
									buf2[cnt] = (BYTE)(temp)&0xff;
									cnt+=4;
*/
									buf2[cnt] = tempalpha;	// 알파값과
									buf2[cnt+1] = pal;	// 팔레트 인덱스를 넣어준다.
									cnt+=2;

									cntb++;
								}
								break;
							}
						}
						// 이미지의 한 줄이 압축이 끝났다.

						switch( flag )
						{
							// 만약, 투명 칼라를 체크하다가 한 줄의 압축이 끝났다면, 
							// 다음 라인의 처음까지 투명칼라이므로, 이 다음 라인까지의 바이트 수를 투명 칼라수에 더해준다.
						case 0:
							buf2[cnt+1] = (BYTE)(cnta>>24)&0xff;
							buf2[cnt] = (BYTE)(cnta>>16)&0xff;
							buf2[cnt+3] = (BYTE)(cnta>>8)&0xff;
							buf2[cnt+2] = (BYTE)(cnta&0xff);
							cnt+=4;
							buf2[cnt] = (BYTE)0xff;
							buf2[cnt+1] = (BYTE)0xff;
							cnt+=2;
							buf2[cnt+1] = (BYTE)(Xsize>>8);
							buf2[cnt] = (BYTE)(Xsize&0xff);
							cnt+=2;
							cntb = 0;
							cnta = 0;
							set++;
							break;
							// 만약, 실제 이미지 칼라를 체크하다가 한 줄의 압축이 끝났다면,
							// 더이상 실제 이미지 칼라가 아니므로, 실제 이미지 수를 저장하고, 투명 칼라 체크 플레그로 셋팅한다.
						case 1:
							buf2[orgcntb+1] = (BYTE)(cntb>>8);
							buf2[orgcntb] = (BYTE)(cntb&0xff);
							set++;

							buf2[cnt] = 0;
							buf2[cnt+1] = 0;
							buf2[cnt+2] = 0;
							buf2[cnt+3] = 0;
							cnt+=4;

							buf2[cnt+1] = (BYTE)0xff;
							buf2[cnt] = (BYTE)0xff;
							cnt+=2;

							buf2[cnt+1] = (BYTE)(Xsize>>8);
							buf2[cnt] = (BYTE)(Xsize&0xff);
							cnt+=2;

							set++;
							cntb = 0;
							cnta = 0;
							flag = 0;
							break;
						}

						// 다음줄을 계속 체크한다.
					}

					// 투명칼라-실제이미지, 이 두가지가 하나의 셋트이며, 총 몇 셋트인지를 저장한다.
					buf2[1] = (BYTE)(set>>8);
					buf2[0] = (BYTE)(set&0xff);

					return cnt;
}






















//-----------------------------------------------------------------------------
// Name: char LoadTGAFile( CHAR* strFilename )
// Desc: TGA 파일 로딩해서, 어두운 글로우 깨끗이 정리한 후, 하위 폴더에 복사하는 함수
//-----------------------------------------------------------------------------
char LoadTGAFile( CHAR* strFilename )
{
	BYTE r, g, b, color[10];
	char	buff1[12];
	WORD	bitperpixel = 0;
	char	temp2[FILENAME_MAX];
	FILE	*fp;
	int length = strlen( strFilename ) - strlen(strrchr( strFilename, '.' ));
	int	framecnt = -1;
	int i, j;
	int	namecnt = 0, flip = 0;

	strcpy( temp2, strFilename);
	temp2[3] = NULL;

	fp = fopen( strFilename, "rb" );

	if ( !fp )
		return false;
	// TGA 파일의 헤더읽기
	fread( buff1, 12, 1, fp );

	// 파일의 가로 크기
	fread( &TGAxsize, 2, 1, fp );

	// 파일의 세로 크기
	fread( &TGAysize, 2, 1, fp );

	// Dummy Data
	fread( &bitperpixel, 2, 1, fp );
	bitperpixel &= 0xff;
	/*
	for ( i = TGAysize - 1; i > -1; i-- )
	fread( &buf[ i * TGAxsize ], TGAxsize * bitperpixel / 8, 1, fp );
	*/

	if ( buff1[7] )
		flip = true;
	else
		flip = false;

	if ( flip )
	{
		for ( i = 0; i < TGAysize; i++ )
		{
			switch( bitperpixel )
			{
			case 32:
				for ( j = 0; j < TGAxsize; j++ )
				{
					fread( color, (bitperpixel / 8), 1, fp );

					memcpy( &buf[j + i * TGAxsize], color, 4 );
					alpha[j + i * TGAxsize] = color[3];
					g_alphaflag = true;
				}
				break;
			default:
				return false;
				break;
			}
		}
	}
	else
		for ( i = TGAysize - 1; i > -1; i-- )
		{
			switch( bitperpixel )
			{
			case 32:
				for ( j = 0; j < TGAxsize; j++ )
				{
					fread( color, (bitperpixel / 8), 1, fp );

					memcpy( &buf[j + i * TGAxsize], color, 4 );
					alpha[j + i * TGAxsize] = color[3];
					g_alphaflag = true;
				}
				break;
			default:
				return false;
			}
		}





		fclose( fp );

		return TRUE;
}

//-----------------------------------------------------------------------------
// Name: void main( void )
// Desc: 메인 함수
//-----------------------------------------------------------------------------
void main( void )
{
	_finddata_t fileinfo;

	bool g_colorkey = false;
	char	start = 0;
	long	handle, TickCount;
	char	Filename[FILENAME_MAX] = "*.tga";
	int first = 0;
	int	filecnt = 0;
	int cnt = 0;
	char	temp[FILENAME_MAX];
	char	savefilename[FILENAME_MAX];

	int i, j, k, l, m, animation, direction;
	int origin_x, origin_y;

	int frames[99][33];
	FILE *fp, *fp2;
	short realx, realy;

	printf( "----------------------------------------------------------\n\n");
	printf( "--0000-0000--00000-0000----00---000000------000-------000-\n\n");
	printf( "-0-----0---0-0-----0---0--0--0----0--------0---0-----0---O\n\n");
	printf( "-0-----0000--0000--0---0--0000----0------------0---------O\n\n");
	printf( "-00----0--00-00----00--0-00---0---00----------0---------0-\n\n");
	printf( "-00----0--00-00----00--0-00---0---00--------00---00----00-\n\n");
	printf( "--0000-0--00-00000-0000--00---0---00-------00000-00--00000\n\n");
	printf( "----------------------------------------------------------\n\n");
	printf( "-------------ver 3.0--------------------------------------\n\n");

	fp = fopen( "origin.txt", "rt" );

	if ( !fp )
	{
		origin_x = -1;
		origin_y = -1;
	}
	else
	{
		fscanf( fp, "%d", &origin_x );
		fscanf( fp, "%d", &origin_y );
		fclose( fp );
	}

	if ( fp = fopen( "nocolorkey", "rt" ) )
	{
		fclose( fp );
		g_nocolorkey = true;
	}
	else
		g_nocolorkey = false;

	fp = fopen( "colorkey.dat", "rt" );

	if ( fp )
	{
		fclose( fp );

		g_colorkey = true;

		LoadTGAFile( "colorkey.dat" );

		memcpy( key, buf, sizeof( buf ) );
	}

	int startframe[100], compare_maxframe[100], compareframe = 0, comparedirection = 0;
	bool compareflag = false;

	fp = fopen( "default.dat", "rt" );

	if ( fp )
	{
		compareflag = true;
		ZeroMemory( startframe, sizeof( startframe ) );
		ZeroMemory( compare_maxframe, sizeof( compare_maxframe ) );

		// 모든 에니메이션, 방향을 통틀어 총 프레임 수를 저장한다.
		fscanf( fp, "%d\n", &compareframe );		// 총 프레임수 

		fscanf( fp, "%d\n", &comparedirection );		// 총 방향수 

		while( true )
		{
			if ( fscanf( fp, "%d", &i ) == EOF )
				break;

			fscanf( fp, "%d", &startframe[i] );

			fscanf( fp, "%d\n", &compare_maxframe[i] );
		}

		fclose( fp );
	}

	// BAN 파일에 저장될 각 에니메이션별 프레임에 대한 정보를 저장할 공간을 초기화
	memset( frames, 0, sizeof( int ) * 99 * 33 );

	// 현재 폴더에서 *.TGA 파일을 최초에 검색한다.
	if( (handle = _findfirst( Filename, &fileinfo ) ) == NOFILES )
	{
		// TGA 파일이 전혀 없을 경우
		printf( "------------- TGA Files Not Found !!! -----------------\n\n");
		while( !_kbhit() );

		exit(0);
	}

	// TGA 파일이 검색되었을 경우, 캐릭터 이름 ( 캐릭터 번호 + 부위 번호 )만을 얻어낸다. 
	//		-> 차후 폴더를 생성하는데 사용
	strcpy( savefilename, fileinfo.name );
	savefilename[5] = NULL;

	int max_direction = 0, max_frames[100], nn, frame = 0;

	// 파일 모든 정보 체크
	while( 1 )
	{
		strcpy( temp, &fileinfo.name[5] );
		temp[8] = NULL;

		// 현재 TGA 파일의 에니메이션 번호를 얻어냄
		animation = atoi( temp ) / 1000000;

		// 현재 TGA 파일의 방향 변호를 얻어냄
		direction = (atoi( temp ) / 10000) % 100;

		frame = (atoi( temp ) % 10000) + 1;

		if ( direction > max_direction )
			max_direction = direction;

		// 해당 에니메이션의 해당 방향이 총 몇프레임인지 알아내기 위해서 카운터 증가
		if ( frame > frames[animation][direction] )
			frames[animation][direction] = frame;

		// 다음 *.TGA 파일을 검사한다.
		if( (_findnext( handle, &fileinfo ) ) == NOFILES )
			break;

		// 모든 TGA 파일을 검사할때까지 루프.
	}

	// 모든 TGA파일을 검사하였다.
	_findclose( handle );

	max_direction++;

	for ( i = 0; i < 100; i++ )
		max_frames[i] = 0;

	// 각 에니메이션의 최대 프레임수 얻기
	for ( j = 0; j < 33; j++ )
		for ( i = 0; i < 99; i++ )
		{
			if ( compareflag && j < comparedirection && compare_maxframe[i] > frames[i][j] )
				frames[i][j] = compare_maxframe[i];

			if ( frames[i][j] > max_frames[i] )
				max_frames[i] = frames[i][j];

		}


		cnt = 0;
		for ( i = 0; i < 100; i++ )
			cnt+=max_frames[i];

		cnt *= max_direction;		// 총 프레임수

		// 해당 캐릭터 이름의 데이터파일( *.dat : 에니메이션 툴에서 사용된다. )을 생성한다.
		sprintf( temp, "%s.dat", savefilename );

		fp = fopen( temp, "wt" );

		// 모든 에니메이션, 방향을 통틀어 총 프레임 수를 저장한다.
		fprintf( fp, "%d\n", cnt );		// 총 프레임수 

		fprintf( fp, "%d\n", max_direction );		// 총 방향수 

		cnt = 0;
		for ( i = 0; i < 99; i++ )
			// 해당 번호의 에니메이션의 프레임 정보를 저장한다.
			if ( max_frames[i] )
			{
				// 저장 형식 : 에니메이션 번호, 총 프레임중 시작 프레임, 프레임 수 
				fprintf( fp, "%d %d %d\n", i, cnt, max_frames[i] );
				cnt+=max_frames[i];
			}

			cnt *= max_direction;

			fclose( fp );

			// 해당 캐릭터의 AN 파일을 만들기 위해 파일 오픈
			sprintf( Filename, "%s.an3", savefilename );
			fp2 = fopen( Filename, "wb" );

			i = 0xffffffff;	// 임시로 채우기
			fwrite( &i, 1, sizeof( int ), fp2 );

			i = 0x1001;		// 해당 AN3파일의 파일 포맷
			fwrite( &i, 1, sizeof( int ), fp2 );

			// 총 프레임 수
			fwrite( &cnt, 1, sizeof( int ), fp2 );

			for ( j = 0; j < max_direction; j++ )
				for ( i = 0; i < 99; i++ )
					if ( frames[i][j] )
					{
						for ( k = 0; k < frames[i][j]; k++ )
						{
							sprintf( Filename, "%s%02d%02d%04d.tga", savefilename, i, j, k );

							printf( "-------- %s file Now Loading..............\n\n", Filename );

							// TGA 파일 열기
							if ( LoadTGAFile( Filename ) )
							{
								sprintf( Filename, "%s%02d%02d%04d.pcx", savefilename, i, j, k );

								LoadPCXFile( Filename );

								printf( "------------------ %s File Now Converting\n\n", Filename );
							}
							else
							{
								realx = 30000;		// 없는 프레임으로 설정해준다.
								fwrite( &realx, 2, 1, fp2 );
								continue;

								//						printf( "---------- Converting Error !!! --->>>>>>> Check File !!! \n" );
								//						while( !_kbhit() );

								//						exit( 1 );
							}

							if ( g_colorkey )
							{
								for ( m = 0; m < TGAysize; m++ )
									for ( l = 0; l < TGAxsize; l++ )
									{
										if ( key[l + m * TGAxsize] == 0x7c1f )
											buf[l + m * TGAxsize] = 0x7c1f;
									}
							}

							// 그래픽 데이터중 실제 이미지 부분만을 떼어서 압축하여 buf2 메모리에 저장한다.
							if ( g_alphaflag )
								l = GetSizesAlpha( TGAxsize, TGAysize );
							//					else
							//						l = GetSizes( TGAxsize, TGAysize );

							if ( origin_x == -1 )
							{
								realx = (short)CutX - ( TGAxsize / 2 );
								realy = (short)CutY - ( TGAysize / 2 );
							}
							else
							{
								realx = (short)CutX - origin_x;
								realy = (short)CutY - origin_y;
							}

							// 알파값이 있는 놈 처리
							if ( g_alphaflag )
							{
								nn = 30002;
								fwrite( &nn, 2, 1, fp2 );
							}

							// 실제 이미지의 왼쪽 면 좌표
							fwrite( &realx, 2, 1, fp2 );

							// 실제 이미지의 위쪽 면 좌표
							fwrite( &realy, 2, 1, fp2 );

							// 실제 이미지의 가로 크기
							fwrite( &Xsize, 2, 1, fp2 );

							// 실제 이미지의 세로 크기
							fwrite( &Ysize, 2, 1, fp2 );

							// 실제 이미지가 압축된 크기 바이트수
							fwrite( &l, 4, 1, fp2 );

							// 실제 압축된 이미지
							fwrite( buf2, l, 1, fp2 );	

							// 다음 프레임으로..
						}
					}
					// 해당 에니메이션의 해당 방향에 이미지가 없다면, 더미로 저장
					else
						if ( max_frames[i] )
						{
							for ( k = 0; k < max_frames[i]; k++ )
							{
								short realx = 30000;

								// 실제 이미지의 왼쪽 면 좌표( 30000 인 경우는 없는 프레임으로 간주된다. )
								fwrite( &realx, 2, 1, fp2 );
							}
						}

		// 마지막에 팔레트 저장

		fwrite( palette, sizeof(palette), 1, fp2 );


						// 최종 AN 파일을 저장한다.
						fclose( fp2 );

						printf( "No Errors ...................... complete .........................\n" );

						TickCount = GetTickCount();		

						while( GetTickCount() - TickCount < 1000 ){};

}
