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

// TGA ������ �ε��� ���� 
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
// Desc: ���� �̹��� �κи��� �߶� �����Ͽ� buf2 �� ����
//-----------------------------------------------------------------------------
int GetSizesAlpha( int xs, int ys )
{
	int i, j;

	// ���� ����� �̹����� �� ��� ��ǥ�� ����.
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

		// ���� ����� �̹����� ������ ��ǥ�� ����.
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

			// ���� ����� �̹����� ���� ũ�⸦ ����.
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

				// ���� ����� �̹����� ���� ũ�⸦ ����.
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

					// ���� �̹��� �簢 �������� �˻��Ͽ� �����ϴ� �κ�
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
								// ���� Į�� üũ
							case 0:
								// ���� ���� Į�� ��� üũ�ϴ� ��, 
								// ���� Į�� �ƴ� ���� �̹����� ���Դٸ�, �� ���� Į��� ����
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
								// ���� �̹��� üũ
							case 1:
								// ���� ���� �̹��� ���� üũ�ϴ� ��,
								// ���� �̹����� �ƴ�, ���� Į�� ���Դٸ�, ���� �̹��� �� ����
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
									// ��� ���� �̹����� ��� ������ ���¶��, ���� �̹��� ��� ����
								{
/*									buf2[cnt+3] = (BYTE)(temp>>24)&0xff;
									buf2[cnt+2] = (BYTE)(temp>>16)&0xff;
									buf2[cnt+1] = (BYTE)(temp>>8)&0xff;
									buf2[cnt] = (BYTE)(temp)&0xff;
									cnt+=4;
*/
									buf2[cnt] = tempalpha;	// ���İ���
									buf2[cnt+1] = pal;	// �ȷ�Ʈ �ε����� �־��ش�.
									cnt+=2;

									cntb++;
								}
								break;
							}
						}
						// �̹����� �� ���� ������ ������.

						switch( flag )
						{
							// ����, ���� Į�� üũ�ϴٰ� �� ���� ������ �����ٸ�, 
							// ���� ������ ó������ ����Į���̹Ƿ�, �� ���� ���α����� ����Ʈ ���� ���� Į����� �����ش�.
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
							// ����, ���� �̹��� Į�� üũ�ϴٰ� �� ���� ������ �����ٸ�,
							// ���̻� ���� �̹��� Į�� �ƴϹǷ�, ���� �̹��� ���� �����ϰ�, ���� Į�� üũ �÷��׷� �����Ѵ�.
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

						// �������� ��� üũ�Ѵ�.
					}

					// ����Į��-�����̹���, �� �ΰ����� �ϳ��� ��Ʈ�̸�, �� �� ��Ʈ������ �����Ѵ�.
					buf2[1] = (BYTE)(set>>8);
					buf2[0] = (BYTE)(set&0xff);

					return cnt;
}






















//-----------------------------------------------------------------------------
// Name: char LoadTGAFile( CHAR* strFilename )
// Desc: TGA ���� �ε��ؼ�, ��ο� �۷ο� ������ ������ ��, ���� ������ �����ϴ� �Լ�
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
	// TGA ������ ����б�
	fread( buff1, 12, 1, fp );

	// ������ ���� ũ��
	fread( &TGAxsize, 2, 1, fp );

	// ������ ���� ũ��
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
// Desc: ���� �Լ�
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

		// ��� ���ϸ��̼�, ������ ��Ʋ�� �� ������ ���� �����Ѵ�.
		fscanf( fp, "%d\n", &compareframe );		// �� �����Ӽ� 

		fscanf( fp, "%d\n", &comparedirection );		// �� ����� 

		while( true )
		{
			if ( fscanf( fp, "%d", &i ) == EOF )
				break;

			fscanf( fp, "%d", &startframe[i] );

			fscanf( fp, "%d\n", &compare_maxframe[i] );
		}

		fclose( fp );
	}

	// BAN ���Ͽ� ����� �� ���ϸ��̼Ǻ� �����ӿ� ���� ������ ������ ������ �ʱ�ȭ
	memset( frames, 0, sizeof( int ) * 99 * 33 );

	// ���� �������� *.TGA ������ ���ʿ� �˻��Ѵ�.
	if( (handle = _findfirst( Filename, &fileinfo ) ) == NOFILES )
	{
		// TGA ������ ���� ���� ���
		printf( "------------- TGA Files Not Found !!! -----------------\n\n");
		while( !_kbhit() );

		exit(0);
	}

	// TGA ������ �˻��Ǿ��� ���, ĳ���� �̸� ( ĳ���� ��ȣ + ���� ��ȣ )���� ����. 
	//		-> ���� ������ �����ϴµ� ���
	strcpy( savefilename, fileinfo.name );
	savefilename[5] = NULL;

	int max_direction = 0, max_frames[100], nn, frame = 0;

	// ���� ��� ���� üũ
	while( 1 )
	{
		strcpy( temp, &fileinfo.name[5] );
		temp[8] = NULL;

		// ���� TGA ������ ���ϸ��̼� ��ȣ�� ��
		animation = atoi( temp ) / 1000000;

		// ���� TGA ������ ���� ��ȣ�� ��
		direction = (atoi( temp ) / 10000) % 100;

		frame = (atoi( temp ) % 10000) + 1;

		if ( direction > max_direction )
			max_direction = direction;

		// �ش� ���ϸ��̼��� �ش� ������ �� ������������ �˾Ƴ��� ���ؼ� ī���� ����
		if ( frame > frames[animation][direction] )
			frames[animation][direction] = frame;

		// ���� *.TGA ������ �˻��Ѵ�.
		if( (_findnext( handle, &fileinfo ) ) == NOFILES )
			break;

		// ��� TGA ������ �˻��Ҷ����� ����.
	}

	// ��� TGA������ �˻��Ͽ���.
	_findclose( handle );

	max_direction++;

	for ( i = 0; i < 100; i++ )
		max_frames[i] = 0;

	// �� ���ϸ��̼��� �ִ� �����Ӽ� ���
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

		cnt *= max_direction;		// �� �����Ӽ�

		// �ش� ĳ���� �̸��� ����������( *.dat : ���ϸ��̼� ������ ���ȴ�. )�� �����Ѵ�.
		sprintf( temp, "%s.dat", savefilename );

		fp = fopen( temp, "wt" );

		// ��� ���ϸ��̼�, ������ ��Ʋ�� �� ������ ���� �����Ѵ�.
		fprintf( fp, "%d\n", cnt );		// �� �����Ӽ� 

		fprintf( fp, "%d\n", max_direction );		// �� ����� 

		cnt = 0;
		for ( i = 0; i < 99; i++ )
			// �ش� ��ȣ�� ���ϸ��̼��� ������ ������ �����Ѵ�.
			if ( max_frames[i] )
			{
				// ���� ���� : ���ϸ��̼� ��ȣ, �� �������� ���� ������, ������ �� 
				fprintf( fp, "%d %d %d\n", i, cnt, max_frames[i] );
				cnt+=max_frames[i];
			}

			cnt *= max_direction;

			fclose( fp );

			// �ش� ĳ������ AN ������ ����� ���� ���� ����
			sprintf( Filename, "%s.an3", savefilename );
			fp2 = fopen( Filename, "wb" );

			i = 0xffffffff;	// �ӽ÷� ä���
			fwrite( &i, 1, sizeof( int ), fp2 );

			i = 0x1001;		// �ش� AN3������ ���� ����
			fwrite( &i, 1, sizeof( int ), fp2 );

			// �� ������ ��
			fwrite( &cnt, 1, sizeof( int ), fp2 );

			for ( j = 0; j < max_direction; j++ )
				for ( i = 0; i < 99; i++ )
					if ( frames[i][j] )
					{
						for ( k = 0; k < frames[i][j]; k++ )
						{
							sprintf( Filename, "%s%02d%02d%04d.tga", savefilename, i, j, k );

							printf( "-------- %s file Now Loading..............\n\n", Filename );

							// TGA ���� ����
							if ( LoadTGAFile( Filename ) )
							{
								sprintf( Filename, "%s%02d%02d%04d.pcx", savefilename, i, j, k );

								LoadPCXFile( Filename );

								printf( "------------------ %s File Now Converting\n\n", Filename );
							}
							else
							{
								realx = 30000;		// ���� ���������� �������ش�.
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

							// �׷��� �������� ���� �̹��� �κи��� ��� �����Ͽ� buf2 �޸𸮿� �����Ѵ�.
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

							// ���İ��� �ִ� �� ó��
							if ( g_alphaflag )
							{
								nn = 30002;
								fwrite( &nn, 2, 1, fp2 );
							}

							// ���� �̹����� ���� �� ��ǥ
							fwrite( &realx, 2, 1, fp2 );

							// ���� �̹����� ���� �� ��ǥ
							fwrite( &realy, 2, 1, fp2 );

							// ���� �̹����� ���� ũ��
							fwrite( &Xsize, 2, 1, fp2 );

							// ���� �̹����� ���� ũ��
							fwrite( &Ysize, 2, 1, fp2 );

							// ���� �̹����� ����� ũ�� ����Ʈ��
							fwrite( &l, 4, 1, fp2 );

							// ���� ����� �̹���
							fwrite( buf2, l, 1, fp2 );	

							// ���� ����������..
						}
					}
					// �ش� ���ϸ��̼��� �ش� ���⿡ �̹����� ���ٸ�, ���̷� ����
					else
						if ( max_frames[i] )
						{
							for ( k = 0; k < max_frames[i]; k++ )
							{
								short realx = 30000;

								// ���� �̹����� ���� �� ��ǥ( 30000 �� ���� ���� ���������� ���ֵȴ�. )
								fwrite( &realx, 2, 1, fp2 );
							}
						}

		// �������� �ȷ�Ʈ ����

		fwrite( palette, sizeof(palette), 1, fp2 );


						// ���� AN ������ �����Ѵ�.
						fclose( fp2 );

						printf( "No Errors ...................... complete .........................\n" );

						TickCount = GetTickCount();		

						while( GetTickCount() - TickCount < 1000 ){};

}
