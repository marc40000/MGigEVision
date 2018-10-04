// MGigEVisionDev.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include <iostream>
#include "MGEV/MGEV.h"
#include "MBMP.h"


// FLIR Camera 320x256x14
#define CAMERA14BITS

// Manta Camera 1292x964xRGB8
//#define CAMERARGB8BITS



class Callback : public MGEVCameraCallback
{
public:
	void Recv(unsigned short int msgtype, unsigned short int msgsize, unsigned short int msgid, unsigned char * msg)
	{
		printf("Usercallback: msgtype=0x%x, msgsize=%u, msgid=%u\n", (unsigned int)msgtype, (unsigned int)msgsize, (unsigned int)msgid);
	}
	
	void RecvFrame(StreamDataFrameHeader & header, StreamDataFrameFooter & footer, const unsigned int pn, unsigned char * p, const unsigned int packetslost)
	{
		printf("Frame received: id=%u, x=%u, y=%u, size=%u, packetslost=%u\n", header.framenumber, (unsigned int)header.xdim, (unsigned int)header.ydim, pn, packetslost);
		
		if ((header.framenumber % 128) == 0)
		{
			TCHAR filename[1024];
			_stprintf(filename, _T("imgs\\%08u.bmp"), header.framenumber);

#ifdef CAMERA14BITS
			static unsigned char t[1024 * 1024 * 32];
			unsigned int i;
			for (i = 0; i < pn / 2; i++)
			{
				t[i] = ((((unsigned short int*)p)[i]) >> 6);
			}
			MBMPSave13(filename, t, header.xdim, pn / (2 * header.xdim));
#endif
#ifdef CAMERARGB8BITS
			MBMPSave33(filename, p, header.xdim, pn / (3 * header.xdim));
#endif
		}
	}
};

int main(int argc, char ** argv)
{
    std::cout << "MGigeVisionDev\n";

	WSADATA data;
	int err = WSAStartup(MAKEWORD(2, 2), &data);

	MGEVCamera cam;
	Callback callback;
	cam.Init(12220, MGEVIPCreate(10, 0, 5, 1), &callback);
	cam.Start("genicam.xml", "genicam.zip");

	int state = 0;
	while (true)
	{
		cam.Process();
		cam.ProcessStream();

		switch (state)
		{
		case 0:
		{
			// waiting for cam to be initialized
			if (cam.initialized)
			{
				state = 1;

				cam.NUCDisable();
				cam.NUCActionLong();

				cam.StartGrab(MGEVIPCreate(10,0,0,6), 12221, 8000);
			}
			break;
		}
		case 1:
		{
			// running
			break;
		}
		}
		Sleep(1);
	}

}

