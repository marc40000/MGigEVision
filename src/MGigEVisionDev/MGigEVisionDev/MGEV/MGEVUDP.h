#pragma once

#include <Winsock2.h>

inline unsigned int MGEVIPCreate(const unsigned int a, const unsigned int b, const unsigned int c, const unsigned int d)
{
	return a | (b << 8) | (c << 16) | (d << 24);
}

class MGEVUDPCallBack
{
public:
	virtual void Recv(unsigned int pn, unsigned char * p) = 0;
};

class MGEVUDP
{
public:
	unsigned short localport;
	unsigned int remoteip;
	unsigned short remoteport;

	SOCKET sockfd;
	sockaddr_in remote;

	MGEVUDPCallBack * callback;

	inline int Init(const unsigned short localport, const unsigned int remoteip, const unsigned short remoteport, MGEVUDPCallBack * callback, const int sndbuf = 0, const int rcvbuf = 0)
	{
		this->localport = localport;
		this->remoteip = remoteip;
		this->remoteport = remoteport;
		this->callback = callback;

		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd == -1)
		{
			return 1;
		}

		int yes = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(int)) == -1)
		{
			return 2;
		}

		if (sndbuf != 0)
		{
			setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)& sndbuf, sizeof(int));
		}
		if (rcvbuf != 0)
		{
			setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)& rcvbuf, sizeof(int));
		}

		listen(sockfd, 5);

		sockaddr_in local;
		memset(&local, 0, sizeof(local));
		local.sin_family = AF_INET;
		local.sin_port = htons(localport);
		if (bind(sockfd, (sockaddr*)&local, sizeof(local)) == -1)
		{
			return 3;
		}

		memset(&remote, 0, sizeof(remote));
		remote.sin_family = AF_INET;
		remote.sin_port = htons(remoteport);
#if defined(WIN32) || defined(_WIN64)
		remote.sin_addr.S_un.S_addr = remoteip;// inet_addr("192.168.1.210");
#else
		remote.sin_addr.s_addr = remoteip;// inet_addr("192.168.1.210");
#endif

		return 0;
	}

	inline int Destroy()
	{
		closesocket(sockfd);
		return 0;
	}

	inline int Send(const unsigned int pn, const unsigned char * p)
	{
		int res = sendto(sockfd, (char*)p, pn, 0, (sockaddr*)&remote, sizeof(sockaddr_in));
		/*if (res == -1)
		{
			return WSAGetLastError();
		}*/
		return res;
	}

	inline void Process(const unsigned int timeoutus = 0)
	{
		int ret;
		do
		{
			fd_set setread, setwrite, seterror;
			FD_ZERO(&setread);
			FD_ZERO(&setwrite);
			FD_ZERO(&seterror);

			FD_SET(sockfd, &setread);

			timeval timeout_directly = { 0, 0 };
			timeout_directly.tv_usec = timeoutus;
			ret = select(sockfd + 1, &setread, &setwrite, &seterror, &timeout_directly);
			if (ret == 1)
			{
				sockaddr_in from;
#if defined(WIN32) || defined(_WIN64)
				int fromlen;
#else
				socklen_t fromlen;
#endif
				fromlen = sizeof(sockaddr_in);
				const unsigned int tn = 1028 * 16;
				unsigned char t[tn];
				int recvd = recvfrom(sockfd, (char*)t, tn, 0, (sockaddr*)&from, &fromlen);

				// Handle packet
				callback->Recv(recvd, t);
			}
		}
		while (ret == 1);
	}
};
