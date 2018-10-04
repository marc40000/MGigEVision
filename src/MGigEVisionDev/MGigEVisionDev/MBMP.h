#pragma once

#include <memory.h>
#include <stdio.h>
#include <TCHAR.h>

// save one channel data as 3 channel bmp
inline void MBMPSave13(TCHAR * filename, unsigned char * p, unsigned int x, unsigned int y)
{
	//unsigned char * buf = new unsigned char[54 + 4 * x * y];
	static unsigned char buf[54 + 4 * 2 * 2048 * 2 * 2048];
	memset(buf,0,54);

	buf[0]=0x42;
	buf[1]=0x4D;
	*((unsigned int*)(buf+2))=54+((x+3)%4)*y*3;

	buf[10]=54;
	buf[14]=40;

	*((unsigned int *)(buf+18))=x;
	*((unsigned int *)(buf+22))=y;

	buf[26]=1;
	buf[28]=24;

	// unsigned char pline[4096 * 4];

	unsigned int t=0;
	int xp=(4-((x*3)%4));

	unsigned int i, ix, ofs = 54;
	for (i=0;i<y;++i)
	{
		for (ix = 0; ix < x; ++ix)
		{
			buf[ofs++] = p[(y - 1 - i) * x + ix];
			buf[ofs++] = p[(y - 1 - i) * x + ix];
			buf[ofs++] = p[(y - 1 - i) * x + ix];
		}
		if (xp!=4)
		{
			for (ix = 0; ix < xp; ix++)
				buf[ofs++] = 0; 
		}
	}
	buf[ofs++] = 0; 
	buf[ofs++] = 0; 

	FILE * f;
	_wfopen_s(&f, filename, _T("wb"));
	if (f != 0)
	{
		fwrite(buf, 1, ofs, f);
		fclose(f);
	}

	//delete [] buf;
}

// loads a 3 channel bmp and returns a 1 channel data
inline int MBMPLoad31(TCHAR * filename, unsigned char * p, unsigned int *x, unsigned int *y)
{
	//unsigned char * buf = new unsigned char[54 + 4 * x * y];
	static unsigned char buf[54 + 4 * 2 * 2048 * 2 * 2048];

	FILE * f;
	_wfopen_s(&f, filename, _T("rb"));
	if (f == 0)
		return 1;
	fread(buf, 1, sizeof(buf), f);
	fclose(f);

	*x = *((unsigned int *)(buf+18));
	*y = *((unsigned int *)(buf+22));

	unsigned int t=0;
	int xp=(4-((*x*3)%4));

	unsigned int i, ix, ofs = 54, v;
	for (i=0;i<*y;i++)
	{
		for (ix = 0; ix < *x; ix++)
		{
			v = (buf[ofs] + buf[ofs + 1] + buf[ofs + 2]) / 3;
			ofs += 3;
			p[(*y - 1 - i) * *x + ix] = v;
		}
		if (xp!=4)
		{
			for (ix = 0; ix < xp; ix++)
				++ofs; 
		}
	}

	//delete [] buf;

	return 0;
}

// loads a 1 channel bmp and returns a 1 channel data
inline int MBMPLoad11(TCHAR * filename, unsigned char * p, unsigned int *x, unsigned int *y)
{
	//unsigned char * buf = new unsigned char[54 + 4 * x * y];
	static unsigned char buf[54 + 4 * 2048 * 2048];

	FILE * f;
	_wfopen_s(&f, filename, _T("rb"));
	if (f == 0)
		return 1;
	fread(buf, 1, sizeof(buf), f);
	fclose(f);

	*x = *((unsigned int *)(buf+18));
	*y = *((unsigned int *)(buf+22));

	unsigned int t=0;
	int xp=(4-((*x*3)%4));

	unsigned int i, ix, ofs = 54, v;
	for (i=0;i<*y;i++)
	{
		for (ix = 0; ix < *x; ix++)
		{
			v = buf[ofs];
			++ofs;
			p[(*y - 1 - i) * *x + ix] = v;
		}
		if (xp!=4)
		{
			for (ix = 0; ix < xp; ix++)
				++ofs; 
		}
	}

	//delete [] buf;

	return 0;
}

inline int MBMPLoadX1(TCHAR * filename, unsigned char * p, unsigned int *x, unsigned int *y)
{
	static unsigned char buf[29];

	FILE * f;
	_wfopen_s(&f, filename, _T("rb"));
	if (f == 0)
		return 1;
	fread(buf, 1, sizeof(buf), f);
	fclose(f);

	if (buf[28] == 8)
	{
		return MBMPLoad11(filename, p, x, y);
	}
	else
	if (buf[28] == 24)
	{
		return MBMPLoad31(filename, p, x, y);
	}
	else
	{
		return 2;
	}
}

// save one channel data as one channel bmp
inline void MBMPSave11(TCHAR * filename, unsigned char * p, unsigned int x, unsigned int y)
{
	//unsigned char * buf = new unsigned char[54 + 4 * x * y];
	static unsigned char buf[54 + 4 * 2048 * 2048];
	memset(buf,0,54);

	buf[0]=0x42;
	buf[1]=0x4D;
	*((unsigned int*)(buf+2))=54+((x+3)%4)*y;

	buf[10]=54;
	buf[14]=40;

	*((unsigned int *)(buf+18))=x;
	*((unsigned int *)(buf+22))=y;

	buf[26]=1;
	buf[28]=8;

	// unsigned char pline[4096 * 4];

	unsigned int t=0;
	int xp=(4-((x)%4));

	unsigned int i, ix, ofs = 54;
	for (i=0;i<y;++i)
	{
		for (ix = 0; ix < x; ++ix)
		{
			buf[ofs++] = p[(y - 1 - i) * x + ix];
		}
		if (xp!=4)
		{
			for (ix = 0; ix < xp; ix++)
				buf[ofs++] = 0; 
		}
	}
	buf[ofs++] = 0; 
	buf[ofs++] = 0; 

	FILE * f;
	_wfopen_s(&f, filename, _T("wb"));
	fwrite(buf, 1, ofs, f);
	fclose(f);

	//delete [] buf;
}

// save 3 channel data as 3 channel bmp
inline void MBMPSave33(TCHAR * filename, unsigned char * p, unsigned int x, unsigned int y)
{
	//unsigned char * buf = new unsigned char[54 + 4 * x * y];
	static unsigned char buf[54 + 4 * 2 * 2048 * 2 * 2048];
	memset(buf,0,54);

	buf[0]=0x42;
	buf[1]=0x4D;
	*((unsigned int*)(buf+2))=54+((x+3)%4)*y*3;

	buf[10]=54;
	buf[14]=40;

	*((unsigned int *)(buf+18))=x;
	*((unsigned int *)(buf+22))=y;

	buf[26]=1;
	buf[28]=24;

	// unsigned char pline[4096 * 4];

	unsigned int t=0;

	unsigned int iy, ix, ofs = 54;
	/*for (iy = 0; iy < y; ++iy)
	{
		for (ix = 0; ix < x; ++ix)
		{
			buf[ofs++] = p[((y - 1 - iy) * x + ix) * 3];
			buf[ofs++] = p[((y - 1 - iy) * x + ix) * 3 + 1];
			buf[ofs++] = p[((y - 1 - iy) * x + ix) * 3 + 1];
		}
	}*/
	memcpy(buf + ofs, p, x * y * 3);
	ofs += x * y * 3;
	//buf[ofs++] = 0; 
	//buf[ofs++] = 0; 

	FILE * f;
	_wfopen_s(&f, filename, _T("wb"));
	if (f != 0)
	{
		fwrite(buf, 1, ofs, f);
		fclose(f);
	}

	//delete [] buf;
}

// save 3 channel data as 3 channel bmp
inline void MBMPSave33(const char * filename, unsigned char * p, unsigned int x, unsigned int y)
{
	//unsigned char * buf = new unsigned char[54 + 4 * x * y];
	static unsigned char buf[54 + 4 * 2 * 2048 * 2 * 2048];
	memset(buf,0,54);

	buf[0]=0x42;
	buf[1]=0x4D;
	*((unsigned int*)(buf+2))=54+((x+3)%4)*y*3;

	buf[10]=54;
	buf[14]=40;

	*((unsigned int *)(buf+18))=x;
	*((unsigned int *)(buf+22))=y;

	buf[26]=1;
	buf[28]=24;

	// unsigned char pline[4096 * 4];

	unsigned int t=0;

	unsigned int ofs = 54;
	/*for (iy = 0; iy < y; ++iy)
	{
		for (ix = 0; ix < x; ++ix)
		{
			buf[ofs++] = p[((y - 1 - iy) * x + ix) * 3];
			buf[ofs++] = p[((y - 1 - iy) * x + ix) * 3 + 1];
			buf[ofs++] = p[((y - 1 - iy) * x + ix) * 3 + 1];
		}
	}*/
	memcpy(buf + ofs, p, x * y * 3);
	ofs += x * y * 3;
	//buf[ofs++] = 0; 
	//buf[ofs++] = 0; 

	FILE * f = fopen(filename, "wb");
	if (f != 0)
	{
		fwrite(buf, 1, ofs, f);
		fclose(f);
	}

	//delete [] buf;
}

