#pragma once

#include <time.h>
#include <malloc.h>
#include <stdio.h>
#include "MGEVUDP.h"
#include "MGEVAddresses.h"
#include "MGEVValues.h"
#include "MGEVNUCTypes.h"
#include "MGEVZip.h"
#include "zlib.h"

// Genicam has pragmas in its headers that automatically links the libs. However, in debug it wants to link the debug libs which are not
// released. So I define GENICAM_NO_AUTO_IMPLIB to prevent autolinking and link the libs manually.
#define GENICAM_NO_AUTO_IMPLIB
#include "GenApi/GenApi.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

const unsigned short int MSGType_DeviceDetect = 0x02;
const unsigned short int MSGType_ReadConfigMemory = 0x80;
const unsigned short int MSGType_WriteConfigMemory = 0x82;
const unsigned short int MSGType_ReadConfigMemoryBlock = 0x84;

const unsigned short int gvcpremoteport = 3956;				// GVCP: GigE Vision Control Protocol
// the other protocol is called GVSP: GigE Vision streaming Protocol


const unsigned int RecvBlockMax = 528;		// inclusive header of 12
const unsigned int RecvBlockHeader = 12;


#pragma push
#pragma pack(1)
class StreamDataFrameHeader		// 44 bytes in size
{
public:
	unsigned int framenumber;
	unsigned short int t0;
	unsigned short int t1;
	unsigned int t2;
	unsigned int t3;
	unsigned int timestamp;
	/*unsigned char t4[6];
	unsigned short int xdim;
	unsigned short int t5;
	unsigned short int ydim;
	unsigned char t6[10];*/
	unsigned int pixelformat;
	unsigned int xdim;
	unsigned int ydim;
	unsigned char t6[10];

	inline void Invert()
	{
		framenumber = ntohl(framenumber);
		t0 = ntohs(t0);
		t1 = ntohs(t1);
		t2 = ntohl(t2);
		t3 = ntohl(t3);
		timestamp = ntohl(timestamp);
		pixelformat = ntohl(pixelformat);
		xdim = ntohl(xdim);
		ydim = ntohl(ydim);
	}
};

class StreamDataFrameData		// 8 bytes in size
{
public:
	unsigned int framenumber;
	unsigned short int t0;
	unsigned short int packetnumberinsideframe;

	inline void Invert()
	{
		framenumber = ntohl(framenumber);
		t0 = ntohs(t0);
		packetnumberinsideframe = ntohs(packetnumberinsideframe);
	}
};

class StreamDataFrameFooter		// 16 bytes in size
{
public:
	unsigned int framenumber;
	unsigned short int t0;
	unsigned short int packetnumberinsideframe;
	unsigned int t1;
	unsigned int t2;

	inline void Invert()
	{
		framenumber = ntohl(framenumber);
		t0 = ntohs(t0);
		packetnumberinsideframe = ntohs(packetnumberinsideframe);
		t1 = ntohl(t1);
		t2 = ntohl(t2);
	}
};
#pragma pop


class MGEVCameraCallback
{
public:
	virtual void Recv(unsigned short int msgtype, unsigned short int msgsize, unsigned short int msgid, unsigned char * msg) = 0;

	virtual void RecvFrame(StreamDataFrameHeader & header, StreamDataFrameFooter & footer, const unsigned int pn, unsigned char * p, const unsigned int packetslost) = 0;
};



class MGEVCamera
{
private:
	// Control callback
	class MGEVCameraVCP : public MGEVUDPCallBack
	{
	public:
		MGEVCamera * camera;
		inline void Recv(unsigned int pn, unsigned char * p)
		{
			camera->RecvVCP(pn, p);
		}
	};

	// Stream callback
	class MGEVCameraVSP : public MGEVUDPCallBack
	{
	public:
		MGEVCamera * camera;
		inline void Recv(unsigned int pn, unsigned char * p)
		{
			camera->RecvVSP(pn, p);
		}
	};


public:

	MGEVCameraCallback * callback;

	MGEVCameraVCP udpcallback;
	MGEVUDP udp;
	unsigned short int messageid;


	// read block in parts
	unsigned int blockstartaddress;
	unsigned int recvbufn;
	unsigned char * recvbuf;
	unsigned int recvbufofs;

	unsigned int xmln;
	char * xml;

	bool initialized;


	bool streamenabled;
	MGEVCameraVSP udpstreamcallback;
	MGEVUDP udpstream;

	StreamDataFrameHeader streamdataframeheader;
	StreamDataFrameFooter streamdataframefooter;
	unsigned char * streamdataframedata;
	unsigned int streamdataframedatan;
	unsigned short int streamdataframelastpacketnumberinsideframe;
	unsigned int streamdataframepacketslost;

private:
	unsigned short int internalmessageid;
	unsigned int internaladdress;

	GenApi::CNodeMapRef genapi;

	char genicamxmlfilename[1024];
	char genicamzipfilename[1024];

	
	unsigned int genicamaddr_AcquisitionStart;
	unsigned int genicamaddr_AcquisitionStop;
	
	unsigned int nuctype;
	unsigned int genicamaddr_NUCMode;
	unsigned int genicamaddr_NUCAction;
	unsigned int genicamaddr_NUCActionLong;


public:
	inline int Init(const unsigned int localip, const unsigned short localport, const unsigned int remoteip, MGEVCameraCallback * callback)
	{
		initialized = false;

		udpcallback.camera = this;
		udpstreamcallback.camera = this;
		streamenabled = false;
		nuctype = MGEVNUCTypeNA;

		int res = udp.Init(localip, localport, remoteip, 3956, &udpcallback);
		if (res != 0)
		{
			return res;
		}

		this->callback = callback;

		messageid = 1;
		internalmessageid = 0;
		internaladdress = 0;
		recvbuf = new unsigned char[0];
		xml = new char[0];
		streamdataframedata = new unsigned char[1024 * 1024 * 32];
		streamdataframedatan = 0;

		return -1;
	}

	inline void Destroy()
	{
		delete[] xml;
		delete[] recvbuf;
		udp.Destroy();
	}

	inline void Process()
	{
		udp.Process();

		unsigned int now = time(0);
		static unsigned int timelast = 0;
		if (now != timelast)
		{
			SendControl();
			timelast = now;
		}
	}

	inline void IncMessageID()
	{
		messageid++;
		if (messageid == 0)
		{
			// 0 is not allowed
			messageid++;
		}
	}

	inline int SendMsg(const unsigned short int msgtype, const unsigned short int pn, unsigned char * p)
	{
		unsigned char t[1024 * 8];
		unsigned short int * s = (unsigned short int*)t;
		t[0] = 0x42;
		t[1] = 0x01;
		s[1] = htons(msgtype);
		s[2] = htons(pn);
		s[3] = htons(messageid);
		memcpy(t + 4 * sizeof(unsigned short int), p, pn);
		int ret = udp.Send(4 * sizeof(unsigned short int) + pn, t);

		IncMessageID();
		return ret;
	}

	inline const unsigned short int GetLastMessageID() const
	{
		unsigned short int lastmessageid = messageid - 1;
		if (lastmessageid == 0)
		{
			lastmessageid--;
		}
		return lastmessageid;
	}

	
	inline int ReadConfigMemory(const unsigned int address, unsigned short int * messageid = 0)
	{
		if (messageid != 0)
		{
			*messageid = this->messageid;
		}
		unsigned int t[1];
		t[0] = htonl(address);
		int ret = SendMsg(MSGType_ReadConfigMemory, sizeof(unsigned int) * 1, (unsigned char *)t);
		return ret;
	}

	inline int ReadConfigMemorys(const unsigned int adressesn, const unsigned int * addresses, unsigned short int * messageid = 0)
	{
		if (messageid != 0)
		{
			*messageid = this->messageid;
		}
		// Note you can read multiple addresses at once by concatenating the addresses
		unsigned int * t = (unsigned int*)_malloca(sizeof(unsigned int) * adressesn);
		unsigned int i;
		for (i = 0; i < adressesn; i++)
		{
			t[i] = htonl(addresses[i]);
		}
		int ret = SendMsg(MSGType_ReadConfigMemory, sizeof(unsigned int) * adressesn, (unsigned char *)t);
		_freea(t);
		return ret;
	}

	inline int WriteConfigMemory(const unsigned int address, const unsigned int value, unsigned short int * messageid = 0)
	{
		if (messageid != 0)
		{
			*messageid = this->messageid;
		}
		unsigned int t[2];
		t[0] = htonl(address);
		t[1] = htonl(value);
		int ret = SendMsg(MSGType_WriteConfigMemory, sizeof(unsigned int) * 2, (unsigned char *)t);
		return ret;
	}

	inline int ReadConfigMemoryBlock(const unsigned int startaddress, const unsigned int n, unsigned short int * messageid = 0)
	{
		if (messageid != 0)
		{
			*messageid = this->messageid;
		}

		delete[] recvbuf;
		recvbuf = new unsigned char[n];
		recvbufn = n;
		recvbufofs = 0;

		unsigned int ofs = 0;
		unsigned int t[2];
		int ret = 0;


		t[0] = htonl(startaddress + ofs);
		unsigned int requestn;
		if ((RecvBlockMax - RecvBlockHeader) < (n - ofs))
		{
			requestn = RecvBlockMax - RecvBlockHeader;
		}
		else
		{
			requestn = n - ofs;
		}
		t[1] = htonl(requestn);

		ret |= SendMsg(MSGType_ReadConfigMemoryBlock, sizeof(unsigned int) * 2, (unsigned char *)t);
			
		ofs += requestn;

		blockstartaddress = startaddress;

		return ret;
	}

	inline int ReadConfigMemoryBlockContinue()
	{
		unsigned int t[2];
		int ret = 0;


		t[0] = htonl(blockstartaddress + recvbufofs);
		unsigned int requestn;
		if ((RecvBlockMax - RecvBlockHeader) < (recvbufn - recvbufofs))
		{
			requestn = RecvBlockMax - RecvBlockHeader;
		}
		else
		{
			requestn = recvbufn - recvbufofs;
		}
		requestn = ((requestn + 3) / 4) * 4;			// make it the next multiple of 4. the dalsa camera doesn't like odd amount of bytes.
		t[1] = htonl(requestn);
		//printf("request %u\n", blockstartaddress + recvbufofs);

		ret |= SendMsg(MSGType_ReadConfigMemoryBlock, sizeof(unsigned int) * 2, (unsigned char *)t);


		return ret;
	}

	void RecvVCP(unsigned int pn, unsigned char * p)
	{
		if (pn < 8)
		{
			printf("Message received with less than 8 bytes. All packets should have at least this lenght.");
			return;
		}
		
		unsigned short int * s = (unsigned short int*)p;

		unsigned short int msgtype = ntohs(s[1]);
		unsigned short int msgsize = ntohs(s[2]);
		unsigned short int msgid = ntohs(s[3]);
		unsigned char * msg = p + 8;

		if (internalmessageid == msgid)
		{
			// handle it internally
			if (msgtype == MSGType_ReadConfigMemoryBlock + 1)
			{
				// append to recvbuf
				unsigned int requestn;
				if (recvbufofs + RecvBlockMax - RecvBlockHeader <= recvbufn)
				{
					requestn = RecvBlockMax - RecvBlockHeader;
				}
				else
				{
					requestn = recvbufn - recvbufofs;
				}
				//printf("appending %u bytes\n", requestn);
				memcpy(recvbuf + recvbufofs, p + RecvBlockHeader, requestn);
				recvbufofs += requestn;


				if (recvbufofs != recvbufn)
				{
					// request the next part of the block
					internalmessageid = messageid;
					ReadConfigMemoryBlockContinue();
				}
				else
				{
					// transfer complete
		
					switch (internaladdress)
					{
					case MGEVAddressGenICamZipFileInfoAddress:
					{
						// the string you get here looks like this: Local:Tau_1_5_7_616D2DFF.zip;3fff8000;aa23
						char * addrs = strchr((char*)msg + 4, ';') + 1;
						char * sizes = strchr(addrs, ';') + 1;
						unsigned int addr, size;
						sscanf_s(addrs, "%x", &addr);
						sscanf_s(sizes, "%x", &size);

						internalmessageid = messageid;
						internaladdress = -1;			// special for GenICam Zipfile download
						ReadConfigMemoryBlock(addr, size);

						break;
					}
					case -1:
					{
						if ((recvbuf[0] == '<') && (recvbuf[1] == '?') && (recvbuf[2] == 'x') && (recvbuf[3] == 'm') && (recvbuf[4] == 'l'))
						{
							// file is plain xml
							xml = new char[recvbufn + 1];
							memcpy(xml, recvbuf, recvbufn);
							xmln = recvbufn;
							xml[xmln] = 0;
						}
						else
						{
							// it's zipped xml

							if (genicamzipfilename[0] != 0)
							{
								FILE * f = fopen(genicamzipfilename, "w+b");
								if (f != 0)
								{
									fwrite(recvbuf, 1, recvbufn, f);
									fclose(f);
								}
							}

							// https://stackoverflow.com/questions/4538586/how-to-compress-a-buffer-with-zlib


							unsigned int ofs;
							unsigned int sizecompressed;
							unsigned int sizeuncompressed;
							ZipFileGetOfsAndSize(recvbuf, ofs, sizecompressed, sizeuncompressed);
							xmln = sizeuncompressed;
							delete[] xml;
							xml = new char[xmln + 1];		// + 1 for manually adding a 0 at the end for termination

							/*// uncompress needs a gzip header so let's fake one http://www.onicos.com/staff/iz/formats/gzip.html
							unsigned char * gzipheader = recvbuf - 10;
							gzipheader[0] = 0x1f;
							gzipheader[1] = 0x8b;
							gzipheader[2] = 8;
							gzipheader[3] = 0;
							*((unsigned int*)(gzipheader + 4)) = 0;
							gzipheader[8] = 0;
							gzipheader[9] = 0;*/

							int ret = uncompressraw((Bytef*)xml, (uLongf*)&xmln, recvbuf + ofs, sizecompressed);
							xml[xmln] = 0;
						}

						if (genicamxmlfilename[0] != 0)
						{
							FILE * f = fopen(genicamxmlfilename, "w+b");
							if (f != 0)
							{
								fwrite(xml, 1, xmln, f);
								fclose(f);
							}
						}


						//setenv("GENICAM_ROOT_V3_1", ".", true);
						//putenv("GENICAM_ROOT_V3_1=.");
						//SetGenICamLogConfig(".");
						//putenv("GENICAM_ROOT=.");
						//putenv("GENICAM_LOG_CONFIG_V3_1=.");
						genapi._LoadXMLFromString(xml);

						internalmessageid = 0;
						internaladdress = 0;


						// get some addresses

						genicamaddr_AcquisitionStart = GenICamGetAddress("AcquisitionStart");
						genicamaddr_AcquisitionStop = GenICamGetAddress("AcquisitionStop");

						if (nuctype == MGEVNUCTypeFlir)
						{
							genicamaddr_NUCMode = GenICamGetAddress("NUCMode");
							genicamaddr_NUCAction = GenICamGetAddress("NUCAction");
							genicamaddr_NUCActionLong = GenICamGetAddress("NUCActionLong");
						}
						if (nuctype == MGEVNUCTypeJenoptik)
						{
							genicamaddr_NUCMode = GenICamGetAddress("ShutterIntv");
							genicamaddr_NUCAction = GenICamGetAddress("NUC");
							genicamaddr_NUCActionLong = GenICamGetAddress("NUCLong");
						}



						initialized = true;

						break;
					}
					}
				}
			}


		}
		else
		{
			// hand it to the user
			callback->Recv(msgtype, msgsize, msgid, msg);
		}
	}

	void GenICamGetXMLFile()
	{
		internalmessageid = messageid;
		internaladdress = MGEVAddressGenICamZipFileInfoAddress;
		ReadConfigMemoryBlock(MGEVAddressGenICamZipFileInfoAddress, 512);
	}

	unsigned int GenICamGetAddress(const char * key)
	{
		unsigned int addr = 0;
		GenApi::INode * node = genapi._GetNode(key);
		if (node == 0)
		{
			// key not found in genicam xml file.
			return -1;
		}
		else
		{
			GenICam::gcstring val, att;
			if (node->GetProperty("Address", val, att))
			{
				// GenApi seems to convert hex to dec automatically upon xml file load. You get a string representing the address in decimal here even though it is hex in the xml file.
				sscanf_s(val.c_str(), "%u", &addr);
			}
			else
			if (node->GetProperty("pAddress", val, att) && val.length() > 0)
			{
				GenApi::INode* pAddrNode = genapi._GetNode(val.c_str());
				GenICam::gcstring sAddr, sAtt;
				pAddrNode->GetProperty("Value", sAddr, sAtt);
				sscanf_s(sAddr.c_str(), "%u", &addr);
			}
			else
			if (node->GetProperty("pValue", val, att) && val.length() > 0)
			{
				addr = GenICamGetAddress(val.c_str());
			}
			else
			if (node->GetProperty("pVariable", val, att) && val.length() > 0)
			{
				// This also has an expression attached to it, like /10, but that's really complicated to implement
				addr = GenICamGetAddress(val.c_str());
			}
		}
		return addr;
	}

	void Start(const char * xmlfilename = 0, const char * zipfilename = 0)
	{
		SendControl();
		Sleep(1);			// We should wait for the ok, but this seems to be enough.

		if (xmlfilename == 0)
		{
			genicamxmlfilename[0] = 0;
		}
		else
		{
			strcpy(genicamxmlfilename, xmlfilename);
		}
		if (zipfilename == 0)
		{
			genicamzipfilename[0] = 0;
		}
		else
		{
			strcpy(genicamzipfilename, zipfilename);
		}

		GenICamGetXMLFile();
	}



	void SendControl()
	{
		WriteConfigMemory(MGEVAddressControl, MGEVValueExclusiveAccess);
	}

	inline void SetNUCType(const unsigned int nuctype)
	{
		this->nuctype = nuctype;
	}
	inline void NUCEnable()
	{
		switch (nuctype)
		{
		case MGEVNUCTypeFlir:
		{
			WriteConfigMemory(genicamaddr_NUCMode, 1);
			break;
		}
		case MGEVNUCTypeJenoptik:
		{
			WriteConfigMemory(genicamaddr_NUCMode, 60);		// 60 s is the default for Jenoptik
			break;
		}
		}
	}
	inline void NUCDisable()
	{
		switch (nuctype)
		{
		case MGEVNUCTypeFlir:
		{
			WriteConfigMemory(genicamaddr_NUCMode, 0);
			break;
		}
		case MGEVNUCTypeJenoptik:
		{
			WriteConfigMemory(genicamaddr_NUCMode, 0);		// 0 s seems to disable it for Jenoptik
			break;
		}
		}
	}
	inline void NUCAction()
	{
		switch (nuctype)
		{
		case MGEVNUCTypeFlir:
		case MGEVNUCTypeJenoptik:
		{
			WriteConfigMemory(genicamaddr_NUCAction, 1);
			break;
		}
		}
	}
	inline void NUCActionLong()
	{
		switch (nuctype)
		{
		case MGEVNUCTypeFlir:
		case MGEVNUCTypeJenoptik:
		{
			WriteConfigMemory(genicamaddr_NUCActionLong, 1);
			break;
		}
		}
	}

	int StartGrab(const unsigned short localport, const unsigned short packetsize = 0x00000438)
	{
		streamenabled = true;

		int res = udpstream.Init(udp.localip, localport, udp.remoteip, 3956, &udpstreamcallback, 0, 1024 * 1024 * 32);
		if (res != 0)
		{
			return res;
		}


		streamdataframeheader.xdim = -1;


		SendControl();
		WriteConfigMemory(MGEVAddressDataStreamTargetIP, htonl(udp.localip));
		WriteConfigMemory(MGEVAddressDataStreamTargetPort, localport);
		WriteConfigMemory(MGEVAddressDataStreamPacketSize, packetsize);



		//WriteConfigMemory(0x0001000c, 0x01000000);
		WriteConfigMemory(genicamaddr_AcquisitionStart, 0x01);
		// read some stuff
		//Sleep(3000);
		//WriteConfigMemory(0x00010204, 0x00000000);

		return 0;
	}

	void StopGrab()
	{
		SendControl();
		WriteConfigMemory(genicamaddr_AcquisitionStop, 0x01);

		streamenabled = false;
	}

	inline void ProcessStream(const unsigned int timeoutus = 0)
	{
		if (streamenabled)
		{
			udpstream.Process(timeoutus);
		}
	}

	void RecvVSP(unsigned int pn, unsigned char * p)
	{
		//printf("streamdata: pn=%u\n", pn);

		unsigned short int packettype = p[4];
		switch (packettype)
		{
		case 1:
		{
			StreamDataFrameHeader * header = (StreamDataFrameHeader *)p;
			streamdataframeheader = *header;
			streamdataframeheader.Invert();

			streamdataframedatan = 0;
			streamdataframelastpacketnumberinsideframe = 0;
			streamdataframepacketslost = 0;
			break;
		}
		case 2:
		{
			StreamDataFrameFooter * footer = (StreamDataFrameFooter *)p;
			streamdataframefooter = *footer;
			streamdataframefooter.Invert();

			// in case we miss a bit of the first frame and the header is invalid
			if (streamdataframeheader.xdim != -1)
			{
				callback->RecvFrame(streamdataframeheader, streamdataframefooter, streamdataframedatan, streamdataframedata, streamdataframepacketslost);
			}

			break;
		}
		case 3:
		{
			StreamDataFrameData * data = (StreamDataFrameData *)p;
			data->Invert();

			unsigned int datasize = pn - sizeof(StreamDataFrameData);

			// just appends data
			memcpy(streamdataframedata + streamdataframedatan, p + sizeof(StreamDataFrameData), datasize);
			streamdataframedatan += datasize;

			// puts data to where it should be according to the packet number in the StreamDataFrameData
			// this is theoretically better but more complicated. It's not implemented correctly right now
			//memcpy(streamdataframedata + datasize * (data->packetnumberinsideframe - 1), p + sizeof(StreamDataFrameData), datasize);
			//streamdataframedatan = datasize * data->packetnumberinsideframe;

			if (streamdataframelastpacketnumberinsideframe + 1 != data->packetnumberinsideframe)
			{
				streamdataframepacketslost += data->packetnumberinsideframe - (streamdataframelastpacketnumberinsideframe + 1);
			}
			streamdataframelastpacketnumberinsideframe = data->packetnumberinsideframe;

			break;
		}
		}

	}

};
