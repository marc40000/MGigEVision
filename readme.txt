------------------------------------------------------------------------------------------------
MGEV - MGigEVision v1 2018

MGEV is an open source free to use library that allows to communicate with GigEVision compatible
cameras. It has no external dependencies so it should be easy to integrate in other projects.
The interface is - hopefully - kept simple. Atm Windows is supported, but Linux support can be
easily added (a few defines and a few replacements of includes and functions - I just have not
have the need for it yet.)

MGEV implements the underlying communication to the cameras over Ethernet. The functions you can
call on the cameras are defined in GeniCam and MGEV uses the GenICam Reference Implementation.
The cameras store an xml file that defines addresses of values you can read and functions you
can execute in the cameras. Upon connect, MGEV downloads this file and hands it to GeniCam's
GenApi for decoding. You can ask GenApi for Addresses of values or functions and the read/write/
execute them using MGEV.

Look at MGigEVisionDev.cpp for an usage example. You have to implement MGEVCameraCallback.
GigEVision uses 2 udp ports for communication: One for control and one for the streaming.
Recv() will be called when a packet for the control arrives, RecvFrame when a whole frame
over the stream port got received. The example code writes a bmp every 128 received frames.
The packetslost parameter indicates how many packets of the current frame got lost. Depending
on what you want to do with the images, you might want to drop frames where packetslost != 0.
In my experience, if you connect the cameras to the pc on a dedicated eth interface or through
one switch, packet loss is not an issue.

If you include MGEV in your project, don't forget to initialize windows networking by
calling WSAStartup(). None of the udp functions called will do anyhing.

Now you simple call MGEVCamrea::Init() with the local pc port for the control udp port,
the ip address of the camera and a pointer to your callback.

Next you call Start(). It takes 2 optional parameters which let you save the GemiCam File
downloaded from the camera on your harddrive if you want to take a look at it. I found that
those xml files can be stored in plain text on the camera as well as zipped. Therefore, you
get two parameters.

Afterwards, you have to call Process() (for the control udp port) and ProcessStream() (for
the data udp port) regularly. Note that the example just calls both in the same thread. For
real applications, I suggest to call ProcessStream() in a seperate priority enhanced thread
that does nothing else.

Also, when writing this code, I was working with ir cameras. They have to calibrate themselves
from time to time and you can control that by some functions defined in GenICam. I implemented
functions for this in MGEV called NUCDisable(), NUCAction() etc. which you should comment out
for optical cameras.

Call StartGrab() after the cam is initlaized. Basicaly, initialized gets true after the genicam
file got downloaded and all the functions become available. StartGrab() takes the target ip
address and port of the stream - which in most cases will be the ip address of the local pc. It
probably can be another ip though. I have not tested this thought. It also takes a packet size.
Theoretically, packetsizes can be negotiated autoamtically. For some reason though, with some
cameras sometimes the result is normal size packets even though everything involved has jumbo
frames enabled. So I force the packet size there. Theoretically, the max packet size is 9014
minus size of some headers. I had an issue with one camera though that started to drop a lot of
packet if the packetsize gets larger approcimately 8000. So the example usese 8000.

If you issue any command by writing to an address or reading an address, you get the answer in
the Recv() callback. The Write and Read methods return a messageid which you can use in the
Recv() callback to identify the corresponding answer. I didnt't want to use blocking io for
this. That would be a lot easier to use but also would be more limiting.

There is also MBMP.h included which is a crude implementation of bmp reading and writing
functions. You can use it as well under the MIT license. However I suggest to use some library
like FreeImage instead. I just included this because it has no dependencies.

To use MGEV in your own project include all the header files in MGEV to you project. Also add
the GenICam directory to your project path. Then include #include "MGEV/MGEV.h".
You should link the GenICam GCBase_MD_VC120_v3_1.lib and GenApi_MD_VC120_v3_1.lib manually.
There are pragmas in the GeniCam headers that link them for you. However, they try to link
debug versions that are not publicly available if you compile your project in debug.
Therefore MGEV defines GENICAM_NO_AUTO_IMPLIB which disables these automatik linking
functionality. You also have to link to zlib.

The ydim parameter of the Recv Callback for some reason is always 0. Maybe the offsset is
wrong. You can calculate the ydim of the (amount of bytes received) / (bytes per pixel * xdim).

Special thanks to the author of https://github.com/wjl12/wjl12-opengigevision . That code was
very helpful when writing MGEV. (I didn't use just that becauseit has no windows support and
has boost and vigra dependency.)

I tested MGEV with a FLIR AX5, a Manta_G-125C and a Teledyne Dalsa Spyder GigE SG-14.

Marc Rochel
------------------------------------------------------------------------------------------------
