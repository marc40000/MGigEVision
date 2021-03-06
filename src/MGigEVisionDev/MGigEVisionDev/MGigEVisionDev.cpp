// MGigEVisionDev.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include <iostream>
#include "MGEV/MGEV.h"
#include "MBMP.h"


// FLIR Camera 320x256x14
//#define CAMERA14BITS

// Manta Camera 1292x964xRGB8
//#define CAMERARGB8BITS

// Jenoptik VarioCam HD Head 700
#define JENOPTIK


class Callback : public MGEVCameraCallback
{
public:
	unsigned int packetslost;
	unsigned int framenumberlast;

	inline Callback()
	{
		packetslost = 0;
		framenumberlast = 0;
	}

	void Recv(unsigned short int msgtype, unsigned short int msgsize, unsigned short int msgid, unsigned char * msg)
	{
		printf("Usercallback: msgtype=0x%x, msgsize=%u, msgid=%u\n", (unsigned int)msgtype, (unsigned int)msgsize, (unsigned int)msgid);
	}
	
	void RecvFrame(StreamDataFrameHeader & header, StreamDataFrameFooter & footer, const unsigned int pn, unsigned char * p, const unsigned int packetslost)
	{
		// Frame number are incremented 16 bit values and the 0 gets skipped.
		// Some cameras start at frame number 0. Some continue where they stopped last time.
		if ((header.framenumber - framenumberlast != 1) && (!((header.framenumber == 1) && (framenumberlast == 65535))))
		{
			wprintf(_T("   frame dropped: idnow=%u   idlast=%u\n"), header.framenumber, framenumberlast);
		}
		framenumberlast = header.framenumber;

		this->packetslost += packetslost;
		printf("Frame received: id=%u, x=%u, y=%u, size=%u, pixelformat=0x%x, packetslost=%u | all=%u\n", header.framenumber, header.xdim, header.ydim, pn, header.pixelformat, packetslost, this->packetslost);

		if ((header.framenumber % 64) == 0)
		{
			TCHAR filename[1024];
			_stprintf(filename, _T("imgs\\%08u.bmp"), header.framenumber);

#ifdef CAMERA14BITS
			static unsigned char t[1024 * 1024 * 32];
			unsigned int i;
			for (i = 0; i < header.xdim * header.ydim; i++)
			{
				t[i] = ((((unsigned short int*)p)[i]) >> 6);
			}
			MBMPSave13(filename, t, header.xdim, header.ydim);
#endif
#ifdef CAMERARGB8BITS
			MBMPSave33(filename, p, header.xdim, header.ydim));
#endif
#ifdef JENOPTIK
			static unsigned char t[1024 * 1024 * 32];
			unsigned int i;
			for (i = 0; i < header.xdim * header.ydim; i++)
			{
				//t[i] = ((((unsigned short int*)p)[i]) >> 8);
				t[i] = ((((unsigned short int*)p)[i]) >> 4);
			}
			MBMPSave13(filename, t, header.xdim, header.ydim - 4);
#endif
		}
	}
};

int main(int argc, char ** argv)
{
    std::cout << "MGigEVisionDev\n";

	WSADATA data;
	int err = WSAStartup(MAKEWORD(2, 2), &data);

	MGEVCamera cam;
	Callback callback;
	//cam.Init(MGEVIPCreate(10,0,0,6), 12220, MGEVIPCreate(10, 0, 5, 5), &callback);
	cam.Init(MGEVIPCreate(192,168,2,6), 12220, MGEVIPCreate(192, 168, 2, 201), &callback);
	cam.SetNUCType(MGEVNUCTypeJenoptik);
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

				unsigned int addr_calibselect = cam.GenICamGetAddress("CalibSelect");
				cam.WriteConfigMemory(addr_calibselect, 7001);
				Sleep(1);
				unsigned int addr_calibchange = cam.GenICamGetAddress("CalibChange");
				cam.WriteConfigMemory(addr_calibchange, 1);
				Sleep(1);

#ifdef JENOPTIK
				unsigned int addr_focus = cam.GenICamGetAddress("FocDistM");
				cam.WriteConfigMemory(addr_focus, 1987);		// between 120 and 300000; actual value * 1000
				Sleep(1);
#endif


				cam.NUCDisable();
				Sleep(1);
				cam.NUCActionLong();
				Sleep(1);

				cam.StartGrab(12221, 4000);
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
