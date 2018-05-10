// UDP_Server_Win32.cpp : Defines the entry point for the application.
// (C) Todor "Twenkid" Arnaudov, 7/2014
// http://twenkid.com
// http://research.twenkid.com
// Licensed to Incept Development
// MIT License since May 2018

/*
//Note for the license file in the installation
FFmpeg

This software uses the FFmpeg library, which is licensed under the GNU Lesser General Public License (LGPL) version 2.1 or later.
The above license applies for the redistributed set of files:

avcodec-55.dll
avformat-55.dll
avutil-52.dll
ffmpeg.exe
ffplay.exe
ffprobe.exe
postproc-52.dll
swresample-0.dll
swscale-2.dll
avdevice-55.dll
avfilter-3.dll

This software is not doing any static or dynamic linking to FFmpeg library files, though - it uses it like a user, through the operating system.
You can redistribute the FFmpeg library files and you have other rights according to LGPL 2.1 or later. Please consult http://ffmpeg.org/legal.html for details.


*/
//#include <iostream>
//#include <stdlib.h>
//#include <mmsystem.h>
//#include "ThreadID.h" It's not required
//#include <winsock2.h>
//#include "Hooks.h"
//#include "HookDLL.h"

#include "stdafx.h"
#include "UDP_Server_Win32.h"
#include "CudpUtils.h"
#include <string.h>
#include <vector>
#include <map>
#include "WebService.h"
#include <commctrl.h>
#include "CaptureWindowBMP.h"
#include "MDIChild.h"
#include <objidl.h> //GDI+
#include <gdiplus.h>
#include "CommDlg.h"> //1.2
#include <iostream>
#include <fstream>
#include <direct.h> 
#include <shellapi.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;
using namespace Gdiplus;

//#define PHP_SCRIPTS 1 //undef for the release for Incept Development
//#define SET_HOOK_TEST_ABOUT 1  //-- undef for Incept --

#define MAX_LOADSTRING 250
#define UPDATE_WINDOW_TITLE 0
#define DEFAULT_UDP_LOGIN_FILTER	"rtsp"
#define GENERATE_REGISTER_FILE 0

bool bUpdateWindowTitle = UPDATE_WINDOW_TITLE;

char sAuthenticateInstallationStr[512];
char sUpdateClubStreamingSettings[512];


// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

//Declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Join(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Register(HWND, UINT, WPARAM, LPARAM);
int UpdateSettings();
void SetTransparency(HWND hw, unsigned char value, int repaint);
void SetOpaque(HWND hw, int repaint);
void CreatePopupWindow();
void MaximizeCurrentRestorePrevious(HWND hwnd);
void SelectVideo(HWND hw, int mode=0); //0 == open file dialog, 1 == directly -- in Init  //1.2
string DoFileOpenSave(HWND hw);

//1.2, 5-7-2014 {
string sIdleVideo;  //}
HWND hwIdleVideo = 0;

#define RECEIVER_CONFIG_IDLE_VIDEO  "config\\idleVideo.txt"
#define RECEIVER_CONFIG_ID "config\\clubid.txt"
void IdleVideoPath();
void IdleVideoPathSave();
char workDir[1024];

void SaveClubId(unsigned int id);
unsigned int LoadClubId();



UDPConnectionServer udp;

int maxStreams = 8; //Change in config
const int maxStreamsMAX = 250;

vector<DWORD> createdProcesses;
HWND hwndVideo[maxStreamsMAX];
vector<HWND> hwndList; //Includes consoles, which are hidden
vector<DWORD> originalWndProc;

int topStream = 0;

typedef map<HWND, DWORD, less<int> > HWND2DWORD;  ////typedef map<string, int, less<string> > STRING2INT;

HWND2DWORD hwndToProc;

typedef map<HWND, int, less<HWND> > HWND2INT;
HWND2INT mapHwndStream;

DWORD ThreadSocket(LPVOID param);
DWORD ThrID;
DWORD RestartIdleVideoID = 0;
DWORD RestartIdleVideo(LPVOID);

typedef struct ConnectionData{
	//string ip; //stub
	char ip[255];
};

ConnectionData con;

extern HWND m_hWnd;  //HWND m_hWnd; in .h in order to be accessible by other modules
HWND hwndOut; //Selected Stream Win
HWND hwndActive; //Skeleton where the outWin is output
HWND hwCanvas = NULL;
HWND hwndPrevActive =  NULL; //Previous active
int m_nCmdShow = SW_SHOWMAXIMIZED;
HWND hwndDesktop = HWND_DESKTOP;
HWND hwMDIClient; //MDI test
extern HWND hwImageOverlay;
//int image_overlay_period = 333;

int index = 0;

PROCESS_INFORMATION pi;

//typedef char* LPCHAR;
//const int maxStrings = 512;
//LPCHAR urls[maxStrings];

int numUrls = 0;
DWORD processID = 0; //to find hwnd and implant it
DWORD dwProcessID;

DWORD ThreadInitialEmbrace(LPVOID param);

//This value should be overriden in the Config file!
const char pathToExeDefault[] = "F:\\Lib\\ffmpeg_win32-shared\\bin\\ffplay.exe";
const char exeTemplate[] = ".exe";
char pathToExe[500]; //readfrom config.txt
int InitialWait = 10000; //readfrom config.txt

DWORD ThrEmbraceID;

//bool bRandomRotateRunning = false;
bool bRndRotate = false;  //Rotation of streams on the Main window

//Various timer, see WM_TIMER
int timerPeriod = 5000; //1333;  
DWORD timerID = 0;
DWORD timerIDPopup = 0; //external window copy, 12-5-2013
DWORD timerEnumWindowsID = 0;
DWORD timerIDImageOverlay = 0;
int iPopupUpdatePeriod = 200;
int prevStreamToShow = 0;
//NO, use enum_windows_period int iTimerEnumWindowsPeriod = 7000;
int iAddedProcesses = 0;

typedef map<string, int, less<string> > STRING2INT;

STRING2INT activeIPs; // = new STRING2INT;  //ip to int

vector<string> liveStreams; //5-4-2013 running  -- problems with map.find, iterators --> can't find a reason
int guiParamsCols = 3; //5-4-  number of columns for the tile, in config
bool bServerStarted = false;

void UpdateStreamMenu(); //6-4-2013 
int defW=640, defH = 480; //Window's initial dimensions

int iMonitor = 0; //30-4-2013
vector<RECT> rectMonitors; //Monitors dimensions

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

void TileWindows();
void OpenStream(char* url, int strID, char* ip);
void StartUDP(const ConnectionData* conn);

bool bHideConsoles = true; //!!!! Should be hidden after the connection was completed. Otherwise it may crash the system process creation!

#define RECEIVER_REGISTER_PATH "config\\receiverData.bin"

#define RECEIVER_CONFIG_PATH_B "config\\receiverConfigB.txt"

//////  Configurables ///////
char sVersion[128];  
char sBaseURL[1024];
char sClubID[200];
int iClubID;
char sScriptType[32]; //add .php to the path or don't, PHP or something else
//////////////////
char sServerIP[100];
char sServerPort[100];
int iStartServerAutomatically; //flag, 1 -- on UpdateSettings etc. // 9-5-2013 -- notice for user that the server has started? 
int iTransparency = 1;
int iTransitionSleep = 40;
//Adjustments for the main window in the external monitor in order to hide the frame
int offset_main_window_y = 29; //	29
int offset_main_window_x = 20; //	5
int offset_main_window_x_left = 8; //-8
int iPort = 8888;
int iLastProcessAdded = 0;
//int offset_main_window_w =	30;

int process_rush_watchdog_max = 4;
int process_rush_period = 4000; 
int load_listed_delay = 3000;
int debug_webservice_authorize = 0;
int debug_webservice_update	= 0;
int enum_windows_period = 2500;
int debug_socket_init = 0;
char forced_ip[50] = "10.0.1.11";
int use_forced_ip = 0;
int clear_registration = 0;
int fullscreen_disabled = 0; //when only one monitor is available or for simulating such a condition
int force_fullscreen_disabled = 0;

int enable_image_overlay = 0;
char image_overlay_path[255] =	"config\\imageOverlay.png";
#define L_IMAGE_OVERLAY L"config\\imageOverlay.png"

int	image_overlay_x	= 0;
int image_overlay_y	= 0;
int image_overlay_width = 128;
int image_overlay_height = 128;
int image_overlay_period = 333;
//int iRandomRotation = 0;
int tests_random_rotation = 100;
int force_new_streams_to_main = 0;
int iSkipRandomCycle = 0;
int debug_show_count_active_streams = 0;
int process_rush_close_after = 5000;
int udp_sleep_periodA = 30;
int udp_sleep_periodB = 30;
int random_rotation_on = 0; //param, when Starting server - sets it
char udp_login_filter[100]; //"rtsp"
int enable_udp_login_filter = 0;

vector<string> streamsToOpen; //24-5  Preloading a list of streams from the config file

///HHOOK hhook;
//in the DLL
///extern HHOOK hhk_dll;
///extern HWND hhk_hwnd_settext;
//extern BOOL WINAPI setMouseHook(HWND); -- in hooks.h

int maxProcesses = 20;  //per given period //!!! 4-5-2013 -- Bug that causes generation of processes that leads to crash!

char serviceStatusStr[]  = "\"status\":{";
char serviceCodeStr[]  = "\"code\":";
char serviceErrorMessageStr[]  = "\"error_message\":";
char serviceSuccessMessageStr[]  = "\"success_message\":";
int errorCodeMaxLen = 7;

Gdiplus::Image *image;

RAWINPUTDEVICE Rid[2]; //11-2013

FILE* flog; //27-7-2014
void OpenLogFile();
void CloseLogFile();

struct NewProcessExchangeT{
	  char* url;
	  int strID;
	  char* ip;
  };

NewProcessExchangeT newProcessExchange;

//This class watches for excessive spawning of processes which may occur sometimes as a bug
//or as a DoS attack. If there's no watchdog, the entire system may crash.
 class ProcessWatchDog{
 public:
	 int spawnedProcesses;
	 long time;
	 int maxProcessesAllowed;
	 long perPeriod;
	 long timeOfSpawn[500];
	 long ticks;
	 int tickLen; //ms ~
	 int maxSpawnedProcesses; //then rotate in the array

	 void Set(int maxProcessesAllowedIn=4, long perPeriodIn=4000){
		 maxProcessesAllowed = maxProcessesAllowedIn;
		 perPeriod = perPeriodIn;
	 }
	 ProcessWatchDog(int maxProcessesAllowedIn=4, long perPeriodIn=4000){ //!!! in ms
		 maxProcessesAllowed = maxProcessesAllowedIn;
		 perPeriod = perPeriodIn;
		 tickLen = 15;
		 ticks = perPeriodIn/tickLen; //e.g. 100 ticks
		 maxSpawnedProcesses = 500 - maxProcessesAllowed;

	 }
	 
	 bool CheckForProcessRush(){ //int num){
		 int minDiff = 0;
		 int back;
		 if (spawnedProcesses < maxProcessesAllowed) return true;
		 int timeDiff = timeOfSpawn[spawnedProcesses] - timeOfSpawn[spawnedProcesses - (maxProcessesAllowed - 1)];
		 if (timeDiff < perPeriod){
			 SetWindowTextA(m_hWnd, "WATCH DOG! PROCESS RUSH! EXITING in a few seconds!!!");
			 Sleep(process_rush_close_after);
			 exit(0);
		 }
		 return true;		 
	 }
	 void Spawn(){
         timeOfSpawn[spawnedProcesses] = GetTickCount();
		 Roll(); //if reaches to the end of the array
         CheckForProcessRush();
		 spawnedProcesses++;		 
	 }

	 void Roll(){
		 if (spawnedProcesses > maxSpawnedProcesses) {
			 int j=0;  
			 for (int i=spawnedProcesses - maxProcessesAllowed; i<spawnedProcesses; i++){
                 timeOfSpawn[j] = timeOfSpawn[i];
				 j++;				 
			 }
             spawnedProcesses = j;
		 }
	 }
 };

  ProcessWatchDog watchDog;
  int iRushDefencePeriod = 100; //override from the config
 ////////////////////////////////

  void IncTopStream(){
	if (topStream == maxStreams)  
	{
		//Close 0... open in zero...
		topStream = 0;
	   //MessageBoxA(0, "TopStream = 0", 0, 0); -- when > than max streams
	}else topStream++;    
}

//receiverData.bin
void GenerateRegisterFile(){		
	fprintf(flog, "GenerateRegisterFile(), FILE* f = fopen(RECEIVER_REGISTER_PATH, ""wb""\r\n"); //#27-7-2014
	FILE* f = fopen(RECEIVER_REGISTER_PATH, "wb");
	char c[5000];
	if (f!=NULL){       
	   for (int i=0; i<4096;i++){
		   c[i] = rand() & 0xFF;
	   }
	   fprintf(flog, "GenerateRegisterFile(), Before >>>    fwrite((DWORD*) c, 1, 4096, f);\r\n"); //#27-7-2014
	   fwrite((DWORD*) c, 1, 4096, f);
	   fprintf(flog, "GenerateRegisterFile(), Before >>>     fclose(f);  \r\n"); //#27-7-2014
	   fclose(f);  
	   fprintf(flog, "GenerateRegisterFile(), After >>>     fclose(f);  \r\n"); //#27-7-2014
	}
	
}

//Offsets for the auth Key
int offsKey[] = {1667, 1678, 1730, 1532, 1821, 1111, 1888, 2456, 2974, 2999, 1590, 2344, 1208, 1215, 1230, 1946, 2053, 1486, 1660, 3386, 2621};
//0x683, 0x68E, 0x6C2, 
const int maxAuthKeySymbols = 21;


//997
void MarkRegistered(char* authKey = NULL){	
	
	FILE* f = fopen(RECEIVER_REGISTER_PATH, "wb");
		fprintf(flog, "FILE* f = fopen(RECEIVER_REGISTER_PATH, ""wb""\r\n"); //#27-7-2014
	char c[5000];		
	if (f!=NULL){
		 fprintf(flog, "if (f!=NULL){...\r\n"); //#27-7-2014
	   for (int i=0; i<4096;i++){
		   c[i] = rand() & 0xFF;
	   }
	   //fread(c, 1, 4096, f);
		DWORD* dwPt = (DWORD*) c;
		unsigned char* btPt = (unsigned char*) c;
		dwPt[256] = 997; //!!! 1024 byte -- can be more random!
		btPt[1024] = 0xE5;
		btPt[1025] = 0x03;
	  
		//Write auth key
		int maxSymbols = maxAuthKeySymbols - 1 ; //last is NULL
					
		if (authKey!=NULL){
			int len = strlen(authKey);
	
			if (len>maxSymbols) len = maxSymbols;

			int k=0; //it's used after the loop
			for (k=0; k<len; k++){
			  c[offsKey[k]] = authKey[k];
			}
			c[offsKey[k]] = 0; //last char
		}
	
		fprintf(flog, "Before:  int wr = fwrite((DWORD*) c, 1, 4096, f);\r\n"); //#27-7-2014
		int wr = fwrite((DWORD*) c, 1, 4096, f);

		fprintf(flog, "Before: >>>>> fclose(f);\r\n"); //#27-7-2014
	  //char cb[200]; 
	  // sprintf(cb, "%d written, %d %d", wr, btPt[1024], btPt[1025]);
	  // MessageBoxA(0, cb, 0, 0);
		fclose(f);
		fprintf(flog, "After:  fclose(f); >>>>>\r\n"); //#27-7-2014
	}else
	{
		fprintf(flog, "Unable to create receiverConfigB.bin! Please check users' permissions for writing to files!\r\n"); //#27-7-2014
		MessageBoxA(0, "Unable to create receiverConfigB.bin! Please check users' permissions for writing to files!\r\n", "Error", 0);
	}
		//was fclose even if f=NULL ?
}
//997
int IsRegistered(){
	FILE* f = fopen(RECEIVER_REGISTER_PATH, "rb"); //"receiverConfig.txt" //rb !!! READ BINARY!
	char c[5000];	
	if (f!=NULL){			
		fread(c, 1, 4096, f);	    	   
		DWORD* dwPt = (DWORD*) c;
		unsigned char* btPt = (unsigned char*) c;
		//dwPt[256] = 997; //!!! 1024 byte  -- more complex!
		unsigned char low, high; //byte
		low = btPt[1024]; // = 0xE5;
		high = btPt[1025]; // = 0x03;
		if ((low == 0xE5) && (high = 0x03))
		{					
			MessageBoxA(0, "This copy is licensed!", "Fusen Receiver", MB_ICONINFORMATION);
			fclose(f);  	
			return 1;
		}
		else {
             char cb[200] = " ";	        
			 sprintf(cb, "This copy is not activated!");			 
			 MessageBoxA(0,cb, "Error",0);
			 fclose(f);  	
			 return 0;	   	     
		    }
	}
			char cb[200] = " ";	        
			 sprintf(cb, "This copy is not activated!");			 
			 MessageBoxA(0,cb, "Error",0);
	return 0;	
}

//RECEIVER_CONFIG_PATH_B
void InitConfigReadable(){	
	int i = 0;
	char sA[1024], sB[1024], sC[1024];
	//if there are comments -- first clean them
	
	  DWORD lenSound = 0;
	  //DWORD mult = (aud->hdrwav.bits/8)*aud->hdrwav.channels;
	  //NoteCommandT note;
	  int tone = 0;
	  double time = 0.0;
	  //float srcLev = 1.0, dstLev = 1.0;
	  float f1, f2, f3;
	  int i1,i2,i3;
	
	int maxStreamsIn;
	char cb[1000];

	for(int i=0; i<maxStreams; i++) hwndVideo[i] = 0;	
	strcpy(udp_login_filter, DEFAULT_UDP_LOGIN_FILTER); //"rtsp"

	FILE* f = fopen(RECEIVER_CONFIG_PATH_B, "rt"); //"receiverConfig.txt"

	if (f!=NULL){

		i1 = fscanf(f, "%s %s", &sA, &sB);//
		while(i1 == 2){
			sprintf(cb, "InitConifg... %s %s", sA, sB);
		  //   MessageBoxA(0, cb, "Conig...",0 ); 


			if ((strcmp(sA, "#") == 0) || (strcmp(sB, "#") ==0)){
				break;
			}
		
		if (strcmp(sA, "ffplay_path") == 0){
			strcpy(pathToExe, sB);
		}else
		if (strcmp(sA, "rotation_period") == 0){
			sscanf(sB, "%d", &timerPeriod);						
		}else
			if (strcmp(sA, "initial_wait") == 0){
			sscanf(sB, "%d", &InitialWait);						
		}else if (strcmp(sA, "max_streams") == 0){
			sscanf(sB, "%d", &maxStreamsIn);
			 maxStreams = maxStreamsIn;	   
		}else
			if (strcmp(sA, "tile_columns") == 0){
			sscanf(sB, "%d", &guiParamsCols);						
		}else
			if (strcmp(sA, "window_width") == 0){
			sscanf(sB, "%d", &defW);
		}else if (strcmp(sA, "window_height") == 0){
			sscanf(sB, "%d", &defH);
		}			
			else if (strcmp(sA, "version") == 0) { strcpy(sVersion, sB);}
			else if (strcmp(sA, "base_url") == 0) { strcpy(sBaseURL, sB);}
			else if (strcmp(sA, "club_id") == 0) {
				strcpy(sClubID, sB);
				sscanf(sClubID, "%d", &iClubID);
			}
			else if (strcmp(sA, "web_script_extension") == 0) {
				//NULL = no extension
				strcpy(sScriptType, sB); //PHP or otherphp etc. 				
			}else if (strcmp(sA, "transition_sleep") == 0){
			 sscanf(sB, "%d", &iTransitionSleep);
		     }
			else if (strcmp(sA, "stream_open") == 0){
				streamsToOpen.push_back(sB);
			}
			else if (strcmp(sA, "offset_main_window_y") == 0){
				sscanf(sB, "%d", &offset_main_window_y);
			}
			else if (strcmp(sA, "offset_main_window_x") == 0){
				sscanf(sB, "%d", &offset_main_window_x);
			} else if (strcmp(sA, "offset_main_window_x_left") == 0){
				sscanf(sB, "%d", &offset_main_window_x_left);
			} else if (strcmp(sA, "main_update_period") == 0){
				sscanf(sB, "%d", &iPopupUpdatePeriod);			    
			} else if (strcmp(sA, "port") == 0){
				sscanf(sB, "%d", &iPort);	
			}
			else if (strcmp(sA, "process_rush_watchdog_max") == 0){
				sscanf(sB, "%d", &process_rush_watchdog_max);	
			} else if (strcmp(sA, "process_rush_period") == 0){
				sscanf(sB, "%d", &process_rush_period);	
			} else if (strcmp(sA, "load_listed_delay") == 0){
				sscanf(sB, "%d", &load_listed_delay);	
			} else if (strcmp(sA, "debug_webservice_authorize") == 0){
				sscanf(sB, "%d", &debug_webservice_authorize);	
			} else if (strcmp(sA, "debug_webservice_update") == 0){
				sscanf(sB, "%d", &debug_webservice_update);	
			} else if (strcmp(sA, "enum_windows_period") == 0){
				sscanf(sB, "%d", &enum_windows_period);	
			}else if (strcmp(sA, "debug_socket_init") == 0){
				sscanf(sB, "%d", &debug_socket_init);	
			}else if (strcmp(sA, "use_forced_ip") == 0){
				sscanf(sB, "%d", &use_forced_ip);	
			}else if (strcmp(sA, "forced_ip") == 0){
				strcpy(forced_ip, sB);	
			} else if (strcmp(sA, "clear_registration") == 0){ //delete .bin ...
				sscanf(sB, "%d", &clear_registration);
			} else if (strcmp(sA, "force_fullscreen_disabled") == 0){
				sscanf(sB, "%d", &force_fullscreen_disabled);	
			}	else if (strcmp(sA, "enable_image_overlay") == 0){
				sscanf(sB, "%d", &enable_image_overlay);	
			} else if (strcmp(sA, "image_overlay_x") == 0){
				sscanf(sB, "%d", &image_overlay_x);
			} else if (strcmp(sA, "image_overlay_y") == 0){
				sscanf(sB, "%d", &image_overlay_y);
			} else if (strcmp(sA, "image_overlay_path") == 0){
				strcpy(image_overlay_path, sB);				
			}  else if (strcmp(sA, "image_overlay_width") == 0){
				sscanf(sB, "%d", &image_overlay_width);
			}  else if (strcmp(sA, "image_overlay_height") == 0){
				sscanf(sB, "%d", &image_overlay_height);
			}   else if (strcmp(sA, "image_overlay_period") == 0){
				sscanf(sB, "%d", &image_overlay_period);
			}    else if (strcmp(sA, "tests_random_rotation") == 0){
				sscanf(sB, "%d", &tests_random_rotation);
			}     else if (strcmp(sA, "debug_show_count_active_streams") == 0){
				sscanf(sB, "%d", &debug_show_count_active_streams);
			} 	else if (strcmp(sA, "force_new_streams_to_main") == 0){
				sscanf(sB, "%d", &force_new_streams_to_main);
			}  	else if (strcmp(sA, "process_rush_close_after") == 0){
				sscanf(sB, "%d", &process_rush_close_after);
			}  	else if (strcmp(sA, "udp_sleep_periodA") == 0){
				sscanf(sB, "%d", &udp_sleep_periodA);
			}   else if (strcmp(sA, "udp_sleep_periodB") == 0){
				sscanf(sB, "%d", &udp_sleep_periodB);
			}  	else if (strcmp(sA, "random_rotation_on") == 0){
				sscanf(sB, "%d", &random_rotation_on);
			}  	else if (strcmp(sA, "udp_login_filter") == 0) {		
				strcpy(udp_login_filter, sB);
			}  	else if (strcmp(sA, "enable_udp_login_filter") == 0){
				sscanf(sB, "%d", &enable_udp_login_filter);
			}  	

			int topMonitor;
			if (rectMonitors.size() > 1) topMonitor = 1;
			else topMonitor = 0;

			if (image_overlay_x < 0) image_overlay_x = (rectMonitors[topMonitor].right -  rectMonitors[topMonitor].left) + image_overlay_x - image_overlay_width; //image->GetWidth();
			if (image_overlay_y < 0) image_overlay_y = rectMonitors[topMonitor].bottom + image_overlay_y ;

			i1 = fscanf(f, "%s %s", &sA, &sB);//
		}//while


		 if (strcmp(sScriptType, "PHP")==0) {
		   strcpy(sAuthenticateInstallationStr, "authenticate-installation.php");
		   strcpy(sUpdateClubStreamingSettings, "update-ip.php");
	   }
	   else {
		   strcpy(sAuthenticateInstallationStr, "authenticate-installation");
		   strcpy(sUpdateClubStreamingSettings, "update-ip");
	   }
	     fclose(f);   
	} //f!=NULL	   	  	   
	else {
		MessageBoxA(0, "Can't find the config file!", RECEIVER_CONFIG_PATH_B, 0); 
		//strcpy(pathToExe, pathToExeDefault);
	}	  
	
	if ((clear_registration) || (GENERATE_REGISTER_FILE == 1)) GenerateRegisterFile();	

	if (!IsRegistered()) {
		DialogBox(hInst, MAKEINTRESOURCE(IDD_REGISTER), m_hWnd, Register);
		//MessageBoxA(0, "Unlicensed copy! Please enter valid 
		//Call Registration dialog
	}
	else
		LoadClubId(); //1.2, #7-7-2014, overrides iClubID, OK

    //char cc[200]; sprintf(cc, "%d", iClubID);
	//MessageBoxA(0, cc, "clubID?", 0);

	watchDog.Set(process_rush_watchdog_max, process_rush_period); 
}
 
 const char isProtocolTemplate[] = "://";

 inline bool isContained(HWND hw, HWND* hwList, int max){
	 if (hw == 0) return false;
	 for (int i=0; i<max; i++)
		  if (hw == hwList[i]) return true;
	 return false;
 }

 void EmbraceStreamsSimple(HWND hwSkip = NULL )	// handle to parent window	// application-defined value)
 {
	 for (int i=0; i<maxStreams; i++) {
		 if (hwSkip == hwndVideo[i]) continue;
		 if ((hwndVideo[i] != 0) && (IsWindow(hwndVideo[i]))){
			    //SetTransparency(hwndVideo[i], 255, 1);
			 SetParent(hwndVideo[i], m_hWnd); //!!! with hwMDIClient); -- after 20 min the window doesn't responds, console appears line of ... //m_hWnd);	
			    //SetOpaque(hwndVideo[i], 1);			 
		 }
	 }
	 //DestroyWindow(hwMDIChild); //ShowWindow(hwMDIChild, SW_HIDE);
	 ShowWindow(hwMDIChild, SW_HIDE);
	 //Flag, don't copy screen!!!
	 //!!! NO!!!! hwMDIChild = NULL;
	 hwndPrevActive = NULL; //4-5-2013
	 hwndActive = NULL; //24-5-2013
	 TileWindows(); //30-4-2013
	 UpdateStreamMenu(); //6-4-2013			 
 }

 void UpdateStreamMenu(){ //6-4-2013 
     char s[255];
	 HWND hw;
	 int j=0; //Number of items	 

	 HMENU hMainMenu = GetMenu(m_hWnd);	 
	 HMENU hMenu = GetSubMenu(hMainMenu, 3);
        	 
	 for(int i=0; i<maxStreams; i++){
		 hw = hwndVideo[i];

		 if (IsWindow(hw))
		 {	 				 
	       GetWindowTextA(hw, s, 250);	 
		   //MessageBoxA(0, "Modify by Command IsWindow(hw)", s,0);
	        const char* protocol; 
			protocol = strstr ((const char*) s, (const char*) isProtocolTemplate);
			if (protocol!=0){				
					ModifyMenuA(hMenu, IDM_STR0 + i, MF_BYCOMMAND, IDM_STR0 + i, s); 					
				j++;
			}			
		 }
		 else ModifyMenuA(hMenu, IDM_STR0 + i, MF_BYCOMMAND, IDM_STR0 + i, "Offline"); 
	 }
 }

 int IsInProcesses(DWORD dwID){
	 for (int i=0; i<createdProcesses.size(); i++)
	    if (createdProcesses[i] == dwID) return 1;

	 return -1;
 }

  int RemoveInProcesses(DWORD dwID){
	 for (int i=0; i<createdProcesses.size(); i++)
	    if (createdProcesses[i] == dwID) createdProcesses[i] = 0;

	 return -1;
 }

  int iEnumerating = 0;  
  int CountActiveStreamsOnly();
  int iEnumPerFeed = 0; //2 to finish enumeration

 BOOL CALLBACK EnumWindowsProc( HWND hwnd,LPARAM lParam )	// handle to parent window	// application-defined value)
{
	DWORD prevProc = NULL;
	char s[255];
	bool isListed= false;	

	iEnumerating = 1;	
	GetWindowThreadProcessId(hwnd, &dwProcessID);  //DWORD ThrID = 
   
    isListed = isContained(hwnd, hwndVideo, maxStreams);  //if (dwProcessID == procSleepessID){ //if ((dwProcessID == processID) && (!isContained(hwnd, hwndVideo, maxStreams))){
   
    if ((dwProcessID == processID) && (!isListed)){ //up to 29-5 *******

    SetParent(hwnd, m_hWnd); //let's see... will it work...

	   hwndList.push_back(hwnd);
	   GetWindowTextA(hwnd, s, 255);	 
	     const char* stats;
	     const char* protocol = strstr ((const char*) s, (const char*) isProtocolTemplate); // :// -- is there a protocol template in the title
		 if (protocol!=0) {
			 ///// if (mapHwndStream.find(hwnd) == mapHwndStream.end() ) //Not found  == WRONG BEHAVIOR
			 if (!isContained(hwnd, hwndVideo, maxStreams))
			 {
				if (IsWindow(hwndVideo[topStream])) 
				{
					SendMessage(hwndVideo[topStream], WM_CLOSE, 0, 0);
					mapHwndStream.erase(mapHwndStream.find(hwnd));
				}
							    
				hwndVideo[topStream] = hwnd; //hwndVideo.push_back(hwnd); //Only HWND to the windows of the video stream // ////hwndVideo[index] = hwnd; //hwndVideo.push_back(hwnd); //Only HWND to the windows of the video stream			  			  
			    mapHwndStream.insert ( std::pair<HWND,int>(hwnd,1) );    //mapHwndStream.insert(hwnd, 1); // mapHwndStream.insert(HWND2INT::value_type(hwnd, 1));
				IncTopStream(); //5-4-2013
			 }
			 iEnumPerFeed++;

			 UpdateStreamMenu();
      
        TileWindows();
		
		if ( (bRndRotate) &&  ((force_new_streams_to_main==1) || (CountActiveStreamsOnly()==1))){ //24-6-2013
				 hwndActive = hwnd;
				 MaximizeCurrentRestorePrevious(hwnd);
				 //SetWindowTextA(m_hWnd, " if ( (bRndRotate) && (force_new_streams_to_main==1)){ //24-6-2013");
			 }
			}
		 else{
			  stats = strstr ((const char*) s, (const char*) exeTemplate);
		       if (stats!=0){  
				   if (bHideConsoles){
                    ShowWindow(hwnd, SW_HIDE);
				 }
				   else
				   ShowWindow(hwnd, SW_MINIMIZE);				 
		      }
			   iEnumPerFeed++;
		 }

	    iEnumerating = 0;
		 
		if (iEnumPerFeed == 2) {iEnumPerFeed = 0; return false;} //stop Enum

		return true;
   	
   } //process
	   
   return true;	   
 }

 void RestoreConsoles(int op=0){ //1 -- hide, 0 -- restore
     char s[255];
	 HWND hw;
	 int j=0, jStep = 100;
	 for(int i=0; i<hwndList.size(); i++){
		 hw = hwndList[i];
		 if (IsWindow(hw))
		 {	 		 
	       GetWindowTextA(hw, s, 250);	 
	       const char* stats;	     
			stats = strstr ((const char*) s, (const char*) exeTemplate);
			if (stats!=0) {
				if (op==0)ShowWindow(hw, SW_RESTORE);
				else if (op==1) ShowWindow(hw, SW_HIDE);
				//MoveWindow(hw, j, 10, 200, 100, true);
				InvalidateRect(hw, 0, true);
				j+=jStep;
			}
		 }
	 }
 }

 void HideConsoles(){
    RestoreConsoles(1);	
 } 

 void DoConsoles(int command=1){ //5-4-2013
     char s[255];
	 HWND hw;
	 int j=0, jStep = 100;
	 for(int i=0; i<hwndList.size(); i++){
		 hw = hwndList[i];
		 if (IsWindow(hw))
		 {	 		 
	       GetWindowTextA(hw, s, 250);	 
	       const char* stats;	     
			stats = strstr ((const char*) s, (const char*) exeTemplate);
			if (stats!=0) {
				if (command==1)
				{
				 ShowWindow(hw, SW_RESTORE);
				 //MoveWindow(hw, j, 10, 200, 100, true);
				 InvalidateRect(hw, 0, true);
				 j+=jStep;
				}
				else if (command==0){
					SendMessageA(hw, WM_CLOSE, 0,0);
				}
			}
		 }
	 }
 }

 void InitRandomRotation(){ //26-6-2013
	 //if (random_rotation_on==1){
	  bRndRotate = random_rotation_on!=0;
	  CheckMenuItem((HMENU) GetMenu(m_hWnd), IDM_RANDOMROTATE, bRndRotate ? MF_CHECKED : MF_UNCHECKED);	 
 }

 //Returns the number of active streams
 int CountActiveStreamsOnly(){
	  int i=0, j=0;	  
	  while (i<maxStreams){
		  if (IsWindow(hwndVideo[i])) j++;
	      i++;
	  }
	 return j; //activeIndices contain the active hwndVideo[activeIndices[0]] etc.
 }

 //Returns the number of active streams and the indices of the active ones
 int CountActiveStreams(int* activeIndices){
	  int i=0, j=0;
	  int topActive = 0;
	  while (i<maxStreams){
		  if (IsWindow(hwndVideo[i])) {
			  topActive=i;	
			  activeIndices[j]=i;
			  j++;
		  }
		  i++;
	  }
	 return j; //activeIndices contain the active hwndVideo[activeIndices[0]] etc.
 }

//  ProcessWatchDog watchDog;
//  int iRushDefencePeriod = 100;
////////////////////////////////
void NewProcessB(const char* path,  char* url, int id, char* ip){ //2, 3, 4, 5

  char cmd[1024];
  sprintf(cmd, "-i %s", url);

  STARTUPINFOA si;   

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
	
	int i;
		 
	{
	  i=0;
	  while (IsWindow(hwndVideo[i]) && (i<maxStreams)){ //Find free slot
        i++;
	  }
	   if (i == maxStreams) { IncTopStream(); id = topStream; }	 //Normal sequential
	   else { topStream = i; id = i; }
	}

	  if ((hwndVideo[id]!=NULL) && IsWindow(hwndVideo[id]))
		SendMessage(hwndVideo[id], WM_CLOSE, 0, 0);  			

	Sleep(iRushDefencePeriod);
	if( !CreateProcessA( pathToExe, cmd, //"-i rtsp://127.0.0.1:8554/2", // path, //NULL,   // No module name (use command line)
    //    cmd, //argv[1],        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
		) 
    {
        //printf( "CreateProcess failed (%d).\n", GetLastError() );
		DWORD err = GetLastError();
		char c[200];
		//sprintf(c, "%d", err);
		sprintf(c, "CreateProcess failed (%d)", err); //!!
		MessageBoxA(0, c, 0, 0);
		processID = 0;
        return;
    }else
		{           
		   processID = pi.dwProcessId;
		   createdProcesses.push_back(processID);
		 
		   string str(ip);
		   
		   liveStreams.push_back(str); 

		   if (liveStreams.size() > 500) {
			   MessageBoxA(0, "Error while Spawning New Process, liveStreams.size() > 500", 0, 0);
			   exit(0); //!!!***********
		   }

	       watchDog.Spawn(); //! 4-5-2013
		   CloseHandle(pi.hThread);
	}
}

//Gdiplus::Image *image;
VOID OnPaintImageOverlay(HDC hdc) 
{
	if (enable_image_overlay==0) return;

	Gdiplus::Graphics graphics(hdc);
	if (image==NULL) return;
	else 
		if (enable_image_overlay){
			int topMonitor;
			if (rectMonitors.size() > 1) topMonitor = 1;
			else topMonitor = 0;

		  ///* //When the window is as big as hwPopupMain
		  int x = image_overlay_x >= 0 ? image_overlay_x : (rectMonitors[topMonitor].right -  rectMonitors[topMonitor].left) + image_overlay_x - image->GetWidth();
		  int y = image_overlay_y >= 0 ? image_overlay_y : rectMonitors[topMonitor].bottom + image_overlay_y ;
		  graphics.DrawImage(image, x, y, image->GetWidth(), image->GetHeight());
		}		
		graphics.ReleaseHDC(hdc); //?
}

void InitRawInput(HWND hw){	 // Init to receive WM_INPUT
	Rid[0].usUsagePage = 0x01; 
	Rid[0].usUsage = 0x02; 
	Rid[0].dwFlags = RIDEV_INPUTSINK; // RIDEV_NOLEGACY; adds HID mouse and also ignores legacy mouse messages
	Rid[0].hwndTarget = hw; //m_hWnd;
	
    if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE) {       
		MessageBoxA(0, "RegisterRawInputDevices failed - Right Click for activating video streams won't work.", "Non critical error", MB_ICONEXCLAMATION);
     }
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

   //////////GDI+
   GdiplusStartupInput gdiplusStartupInput;
   ULONG_PTR           gdiplusToken;
   
   // Initialize GDI+.
   GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL); //19-3-2013
   image = NULL; //Intro 
   image = Image::FromFile(L_IMAGE_OVERLAY); //only Unicode!
   if (image==NULL) {
	   MessageBoxA(0, "Can't load image!", 0,0);
   } //else 	   MessageBoxA(0, "Image loaded!", 0,0);
   
   ////////////// GDI+ end
	 
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_UDP_SERVER_WIN32, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	InitRawInput(m_hWnd);

	////////////
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_UDP_SERVER_WIN32));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		/*
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		*/

		 if (!TranslateMDISysAccel(hwMDIClient, &msg)) //MDI used
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
	}

	 GdiplusShutdown(gdiplusToken); //GDIPlus Shutdown

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_UDP_SERVER_WIN32));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_UDP_SERVER_WIN32);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
   hInst = hInstance; // Store instance handle in our global variable
   
   OpenLogFile(); //#27-7-2014
   _getcwd(workDir, 1024); //#include <direct.h>
   InitConfigReadable(); //it reads some path for the ffmpeg exe, but later override to %workdir%/ffplay.exe
   IdleVideoPath(); //1.2, #6-7-2014
   sprintf(pathToExe, "%s/ffmpeg/ffplay.exe", workDir); //1.2 #6-7-2014
   
   
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, defW, defH, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   if (iTransparency==1){
     SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED); //9-5-2013
     SetTransparency(hWnd, 255, 1);
   }

    CLIENTCREATESTRUCT ccs;

    ccs.hWindowMenu  = GetSubMenu(GetMenu(hWnd), 0); //Edit!
    ccs.idFirstChild = ID_MDI_FIRSTCHILD;

    hwMDIClient = CreateWindowExA(WS_EX_CLIENTEDGE, "mdiclient", NULL,
        WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hWnd, (HMENU)IDC_MAIN_MDI, GetModuleHandle(NULL), (LPVOID)&ccs);
   
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


void StartUDP(const ConnectionData* conn){
	 udp.SetPortUDP(iPort);
     udp.StartWS();
	 udp.CreateSocket();
	 udp.SetupAddress();
	 
     CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadSocket,
                              (LPVOID) conn, THREAD_TERMINATE, &ThrID); 	 
}

DWORD ThreadSocket(LPVOID param){
	udp.SetHWND(m_hWnd);
	udp.ListenSocket();
	return 0;
}

void FinishUDP(const ConnectionData* conn){ 
	udp.PostFinish();
}


void SetOpaque(HWND hw, int repaint=1){
	if (iTransparency==0) return;
	SetWindowLong(hw, GWL_EXSTYLE, GetWindowLong(hw, GWL_EXSTYLE) & (0xffffffff & ~WS_EX_LAYERED)); //(0xffffffff & 	
	if (repaint==1) InvalidateRect(hw, 0, true);
}

void SetTransparency(HWND hw, unsigned char value, int repaint){
  if (iTransparency==0) return;
   SetWindowLong(hw, GWL_EXSTYLE, GetWindowLong(hw, GWL_EXSTYLE) | WS_EX_LAYERED);
   // Make this window value% alpha 0 - 255, 0 == transparent, 255 = opaque
   //SetLayeredWindowAttributes(hw, 0, (255 * value) / 100, LWA_ALPHA);
   SetLayeredWindowAttributes(hw, 0, value, LWA_ALPHA);
   //if (repaint==1) UpdateWindow(hw);
    if (repaint==1) InvalidateRect(hw, 0, true);

}

DWORD ThrOverlayID = NULL;
DWORD ThrOpenStreamsID = NULL;
int iShowHwndActive = 0;
int imageOverlaySet = 0;
//Read from config Timer Period and streams!
void RandomSelectAndShow();


DWORD ThreadOverlay(LPVOID param)
{

		BringWindowToTop(hwPopupMain);
		BringWindowToTop(hwImageOverlay);
		for (int k=0; k<=255; k+=11){		     
			SetTransparency(hwPopupMain, k, 1);			
			Sleep(iTransitionSleep);
		}
			SetTransparency(hwPopupMain, 255, 1);
			Sleep(100);
			 
	   for (int k=255; k>=0; k-=24){				 
				 SetTransparency(hwPopupMain, k, 1);			 
				 Sleep(iTransitionSleep);
		}
			SetTransparency(hwPopupMain, 0, 1);
			  
			{
				BringWindowToTop(hwImageOverlay); //SetWindowPos,HWND_TOP, rectMonitors[1].left, 0, 
				imageOverlaySet = 1;
			 }
		return 0;			 
}

DWORD ThreadOpenStreams(LPVOID param) //Automatically Load Streams on start
{
	int len = streamsToOpen.size();

	if (len!=0) {
		for (int k=0; k<len; k++){

			OpenStream((char*) streamsToOpen[k].c_str(), k, "ip" ); //ip -- not used! // //		   Sleep(5000);//InitialWait + 1500);
			Sleep(load_listed_delay); //3000 InitialWait + 1500);
		}
	}
  return 0;
}

void MaximizeCurrentRestorePrevious(HWND hwnd){	//Sets the Main window stream
	Sleep(20); //17-6-2013 //imageOverlaySet = 0;
	if (!IsWindow(hwnd)) return;
	if (!IsWindow(hwPopupMain)) SetTimer(m_hWnd, timerIDPopup, iPopupUpdatePeriod, NULL); //CreatePopupWindow();
	
	if (timerIDPopup==0)  SetTimer(m_hWnd, timerIDPopup, iPopupUpdatePeriod, NULL);  //CreatePopupWindow(); //Timer
	if (IsWindow(hwMDIChild)) ShowWindow(hwMDIChild, SW_SHOW);

	iStoppedPopup=0; //1; //Don't update
		
	 hwndActive = hwnd;
		
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadOverlay, (LPVOID) 0, THREAD_TERMINATE, &ThrOverlayID); 		

	if (hwndActive==hwndPrevActive) return; //ShowWindow(hwndActive, SW_HIDE); //++ 4-5-2013
	char c[250];			            
    ShowWindow(hwndActive, SW_HIDE); //++ 4-5-2013 ///9-5-2013

	 HWND setpar = SetParent(hwndActive, HWND_DESKTOP); //hwndDesktop) ;//   HWND_DESKTOP);				
		    		
	if ( (rectMonitors.size()<2) || (force_fullscreen_disabled) ){			
			  SetWindowPos(hwndActive,hwPopupMain, rectMonitors[0].left - offset_main_window_x_left, - offset_main_window_y, rectMonitors[0].right -  rectMonitors[0].left + offset_main_window_x,  rectMonitors[0].bottom -  rectMonitors[0].top + offset_main_window_y*2,SWP_NOACTIVATE); // false); //true); //false); //don't repaint, then MAXIMIZE			  
	} else { //second monitor		  
			  SetWindowPos(hwndActive,hwPopupMain, rectMonitors[1].left - offset_main_window_x_left, - offset_main_window_y, rectMonitors[1].right -  rectMonitors[1].left + offset_main_window_x,  rectMonitors[1].bottom -  rectMonitors[1].top + offset_main_window_y*2,SWP_NOACTIVATE); // false); //true); //false); //don't repaint, then MAXIMIZE			  
		  }

	if (setpar == NULL)
	{
	  DWORD err = GetLastError();		
	  sprintf(c, "SetParent: %d", err);
	  MessageBoxA(0, c, 0, 0);
	}
		
		  ////////////////////////////////////
		  //Move last? to hdie flicker?
			///if ( (hwndPrevActive != NULL) && (hwndPrevActive != hwndActive)) { // && (hwndPrevActive!=hwndActive)){ //moved up
			if ( (hwndPrevActive != NULL) && (hwndPrevActive != hwndActive)) {
			   ShowWindow(hwndPrevActive, SW_HIDE);
			   SetParent(hwndPrevActive, m_hWnd);

			   // SetParent(hwndPrevActive, hwMDIClient); //MDI! 15-5-2013
               //////  SetTransparency(hwndPrevActive, 255, 1); //! 9-5-2013
			   //SetOpaque(hwndPrevActive, 1);
			   //MoveWindow(hwndPrevActive, 0,0, 320, 240, false); //don't repaint, then MAXIMIZE
			   
			   InvalidateRect(hwndPrevActive, 0, true);
			   ShowWindow(hwndPrevActive, SW_SHOW);			   			   
			   TileWindows();
		     } 
			else {
                //Remove && (hwndPrevActive != hwndActive) ?
		    	///	  SetParent(hwndPrevActive, m_hWnd);
               ////    SetTransparency(hwndPrevActive, 255, 1); //! 9-5-2013
			   ////   SetOpaque(hwndPrevActive, 1);
			   ////   ShowWindow(hwndPrevActive, SW_SHOW);
			}

			hwndPrevActive = hwndActive;
			iStoppedPopup=0; //Don't update
			iShowHwndActive = 1; //inform the thread				
}

void RandomRotate(){

	if (bRndRotate){
	 KillTimer(m_hWnd, timerID);
	 bRndRotate = false;
	 CheckMenuItem((HMENU) GetMenu(m_hWnd), IDM_RANDOMROTATE, MF_UNCHECKED);

	}
	else
	{
		timerID = SetTimer(m_hWnd, IDT_TIMER, timerPeriod, 0); // lpTimerFunc); //timerID
		char c[200];
		sprintf(c, "TimerID = %d", timerID);
		OutputDebugStringA(c);
		bRndRotate = true;
		CheckMenuItem((HMENU) GetMenu(m_hWnd), IDM_RANDOMROTATE, MF_CHECKED);
		RandomSelectAndShow();
	}
}

//!!! There's also a Win32 function, that's a custom implementation
void TileWindows(){
	int x=0, y=0;
	int w = 320, h = 240;
	int numHw = 0;
	bool bActive[maxStreamsMAX];
	int iActive[maxStreamsMAX];
	int iActiveTop = 0;
	float fAspect = 1.33;	

	for(int i=0; i<maxStreams; i++){		
		if (IsWindow(hwndVideo[i]) && (hwndVideo[i]!=hwndActive)) {
			bActive[i] = true;
			iActive[iActiveTop++] = i;
			numHw++;
		}else bActive[i] = false;
	}	

	RECT lpRect;
	//GetWindowRect(m_hWnd, &lpRect); 	// address of structure for window coordinates, without MDI
	GetWindowRect(hwMDIClient, &lpRect); 	// address of structure for window coordinates
    int dx = abs(lpRect.left - lpRect.right);
	int dy = abs(lpRect.bottom - lpRect.top);

    float fdx = (float) dx;
	float fdy = (float) dy;
	
	//int cols = guiParamsCols;	
	//if (numHw < guiParamsCols) cols = numHw; //move after mdi!
	//if (cols == 0) cols = 1;
		    
    int j,k, mdi = 0; //mdi -- if active, set to 1
    j=0;
	k=j; //because of the hwMDIChild
	//First - main window //15-2-2013
	//if ( (hwndPrevActive!=NULL) && (IsWindow(hwMDIChild))){		 //hwndPrevActive -- after EmbraceWindows is NULL, if it's not null - don't tile the MDI window
	if (IsWindow(hwMDIChild) && (hwndPrevActive!=NULL)){		 //hwndPrevActive -- after EmbraceWindows is NULL, if it's not null - don't tile the MDI window		
		mdi = 1;
		k++;
	}

	int cols = guiParamsCols;	
	if ((numHw+mdi) < guiParamsCols) cols = (numHw+mdi); //move after mdi!
	if (cols == 0) cols = 1;	
   
    int rows = (numHw + mdi)/cols;
	if (rows == 0) rows = 1;

	w = dx / cols;
	h = (int) ((float) w/fAspect);

	//if (IsWindow(hwMDIChild)){
	if (mdi == 1){
		MoveWindow(hwMDIChild,  x, y, w, h, true);
		  //ShowWindow(hwMDIChild, SW_SHOW);
		  //InvalidateRect(hwMDIChild, 0, true);
		x+=w;		
	}

	while (j<numHw){
		
		MoveWindow(hwndVideo[iActive[j]],  x, y, w, h, true);
		ShowWindow(hwndVideo[iActive[j]], SW_SHOW);
		//InvalidateRect(hwndVideo[iActive[j]], 0, true);
		j++;
		k++;
		if (k%cols == 0) { 
			y+=h;
			x =0;
		}
		else x+=w;
		
		//ShowWindow(hwMDIChild, SW_HIDE); -- No, Hide during Embrace	
	}
}

void OpenStream(char* url, int strID, char* ip)
{
	//#### Some Sleep or EnterCriticalSection? ###    
	NewProcessExchangeT* processExchange = new NewProcessExchangeT();
	processExchange->strID = strID;
	processExchange->url = url;
	processExchange->ip = ip;
	    
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadInitialEmbrace,
                              (LPVOID) processExchange, THREAD_TERMINATE, &ThrEmbraceID);   	
}

DWORD ThreadInitialEmbrace(LPVOID param) //Moving new process here doesn't fix it  //Critical Section?
{	
	NewProcessExchangeT* prData = (NewProcessExchangeT*) param; //Created with new in OpenStream()
	//NewProcessB("...", url, strID, ip); //index); //F:\\Lib\\ffmpeg_win32-shared\\bin\\ffplay.exe
	NewProcessB("...", prData->url, prData->strID, prData->ip); //index); //F:\\Lib\\ffmpeg_win32-shared\\bin\\ffplay.exe
	Sleep(InitialWait);
	
	iAddedProcesses++; //After the initial wait!	
	delete prData;   //Created in OpenStream		
	return 0;
}

void CloseIP(const char* ip){ //192.168.1.103
	char title[256];
	
	for (int i=0; i<maxStreams; i++){
       GetWindowTextA(hwndVideo[i], title, 255);
	   //MessageBoxA(0, title, ip, 0);
	   const char* protocol;
	   //protocol = strstr ((const char*) title, (const char*) ip); // isProtocolTemplate);
	   protocol = strstr (title, (const char*) ip); // isProtocolTemplate);
	   //if (strcmp(title, (const char*) ip) == 0){ // isProtocolTemplate);
	   if (protocol!=NULL){
           SendMessage(hwndVideo[i], WM_CLOSE, 0, 0);
		   //CAN't CALL DestroyWindow(hwndVideo[i]) in another Process!!!
		   break; //only one
	   }
	}

	UpdateStreamMenu(); //6-4-2013       
}

void CloseAllWindows(){
	    for(int i=0; i<hwndList.size(); i++){
			  //DestroyWindow(hwndList[i]); not allowed!
			 SendMessage(hwndList[i], WM_CLOSE, 0, 0);
			
			//createdProcesses.push_back(processID);
		}
		for(int i=0; i<maxStreams; i++){
			SendMessage(hwndVideo[i], WM_CLOSE, 0, 0);
		}
}

void SaveMsgLog(DWORD* msgLog, int n){
	FILE* f = fopen("msgLog.txt", "wt");
	if (f!=0){
		fprintf(f, "%d messages\n", n);
		for (int i=0; i<n; i++){
			fprintf(f, "%d\n", msgLog[i]);
		}
	}
	fclose(f);
}

VOID CALLBACK EnumWindowsTimerProc(
    HWND hwnd,	// handle of window for timer messages 
    UINT uMsg,	// WM_TIMER message
    UINT idEvent,	// timer identifier
    DWORD dwTime 	// current system time
	){
			char c[200];
		    //if (iEnumerating==1) return; //wait
			if (iAddedProcesses > 0) {
				//MessageBoxA(0, "EnumWindows addedProcesses??", 0,0);				
				processID = createdProcesses[iLastProcessAdded];

				//Critical Section?!				
				EnumWindows(EnumWindowsProc, 0);
				/// sprintf(c,"AddedProcess, iLastProcAdded = %d, %d", iAddedProcesses, iLastProcessAdded);
				//MessageBoxA(0, c, "EnumWindows Timer?",0);
				////SetWindowTextA(m_hWnd, c);				
				iLastProcessAdded++;
				iAddedProcesses--;
				iEnumerating = 0;
				
				//if (force_new_streams_to_main == 1){ //NO
				//	if (bRndRotate) iSkipRandomCycle = 1;
				//}										
			}
		
			//sprintf(c,"AddedProcess, iLastProcAdded = %d, %d", iAddedProcesses, iLastProcessAdded);
			//MessageBoxA(0, c, "EnumWindows Timer?",0);
			//SetWindowText(m_hWnd, c); //"EnumWindows Timer?",0);
}

void  RandomSelectAndShow(){
	static int topActiveStream;
	static int activeIndices[100];

	topActiveStream = CountActiveStreams(activeIndices);
	if (topActiveStream==0) return;

	for (int k=0; k<tests_random_rotation; k++) //max attempts anyway
	{
        int streamToShow = rand()%topActiveStream; //%maxStreams; //maxStreams; //livingStreams;		
		if ((IsWindow(hwndVideo[activeIndices[streamToShow]])) && (hwndActive != hwndVideo[activeIndices[streamToShow]]) ){
		      //SetWindowPos(hwImageOverlay,HWND_TOP, rectMonitors[1].left, 0, rectMonitors[1].right,  rectMonitors[1].bottom, SWP_NOACTIVATE); // false); //true); //false); //don't repaint, 
			hwndActive = hwndVideo[activeIndices[streamToShow]]; //hwndVideo[streamToShow]; //wmId - IDM_STR0];
		 
			MaximizeCurrentRestorePrevious(hwndActive); //hwndVideo[streamToShow]);
			//PostMessage(hWnd, WM_COMMAND, (streamToShow - IDM_STR0), 0); //test 17-6-2013
		    //prevStreamToShow = streamToShow;
		  break;
		}
	}//for
}

void ProcessWmInput(WPARAM wParam, LPARAM lParam);

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
    HWND setpar = 0; 
	char c[100];
	static sockaddr_in si_other;
	int  xHwnd, yHwnd;
	RECT rect;
	int wTimerID;
	static int topActiveStream;
	static int activeIndices[100];
	static char cb[100];
	static char cb2[250];
	static int iProcessingUDP = 0;

	switch (message)
	{
	case WM_CREATE:
		m_hWnd = hWnd;

		rectMonitors.clear();		//1.2, #5-7-2014
		EnumDisplayMonitors(NULL,NULL,MonitorEnumProc, 0);		

		//InitStrings();
		InitRandomRotation(); //26-6-2013		
	break;
	case WM_INPUT:  //8-11-2013
       ProcessWmInput(wParam, lParam);
	   //SetWindowTextA(hWnd, "WM_INPUT? Click in mHwnd");
	   return 0;
	break;
	case WM_RBUTTONDOWN:		
	break;
	case WM_MBUTTONDOWN:		
	break;
	case WM_LBUTTONDOWN:		
	break;
	case WM_TIMER:
		wTimerID = wParam;   // timer identifier 
           //tmprc = (TIMERPROC *) lParam; // address of timer callback
		if (wTimerID == timerIDImageOverlay)
		{
			if ( (enable_image_overlay) && (IsWindow(hwndActive)))
			{
			 hdc = GetDC(hwndActive); //hwImageOverlay); //hwndActive);
			 OnPaintImageOverlay(hdc);
		     ReleaseDC(hwImageOverlay, hdc); //hwndActive, hdc); //hwImageOverlay
			}
		}
		else if (wTimerID == timerIDPopup)
		{		   
		   if ( (IsWindow(hwMDIChild)) && (IsWindow(hwndActive))) 
		   {
			   CaptureScreen(hwMDIChild, hwndActive); //, hwndActive);			   
		   }		 
		   ////1.2 #5-7-2014 //No -- Start it, but keep it hidden when there are streams?		   
		} else if (wTimerID == timerID){ // Random Rotation			
			topActiveStream = CountActiveStreams(activeIndices);
			if (topActiveStream==0) break;

			if (debug_show_count_active_streams==1)
			{
			  sprintf(cb, "str = %d; ", topActiveStream); //Random rotation test
			  for (int k=0; k<topActiveStream; k++){
				sprintf(cb2,"%d, ", activeIndices[k]);
				strcat(cb, cb2);
			  }
			  SetWindowTextA(m_hWnd, cb);
			}

			RandomSelectAndShow();  	    
		    UpdateStreamMenu();
	 } //timerID		
	break;
	
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);		
		if ((wmId >= IDM_STR0) && (wmId <=IDM_STR25))  // Parse the menu selections: //SELECT stream to show				 
		if (IsWindow( hwndVideo[wmId - IDM_STR0])) ////hwndActive = hwndList[wmId - IDM_STR0];  	 //hwndActive = GetFocus();
		{
			hwndActive = hwndVideo[wmId - IDM_STR0]; //!!!!! Because of this Random Rotation Blinks? 17-6-2013
            MaximizeCurrentRestorePrevious(hwndActive);
		}
			
		switch (wmId)
		{
		case IDM_SHOWCONSOLES:
			RestoreConsoles();
		break;
		case IDM_HIDECONSOLES: //test
			HideConsoles();
		break;
		case IDM_KILLCONSOLES:
			DoConsoles(0);
		break;
		case IDM_JOIN:
           DialogBox(hInst, MAKEINTRESOURCE(IDD_JOIN), hWnd, Join);
		break;
		case IDM_LISTEN: //Start Server
			udp.bDebugMsg = debug_socket_init;
			if (!udp.bStarted){				//1.2 -- should initialize before this, for the idle video!
			 rectMonitors.clear();					
			 EnumDisplayMonitors(NULL,NULL,MonitorEnumProc, 0); //mHDC
			 //Use the second monitor
			 SetUpMDIChildWindowClass(hInst, "MDIClass");
			 hwMDIChild = CreateNewMDIChild(hwMDIClient, "MDIClass");			 
			 if (rectMonitors.size()>1){ //for the black overlay
			   monitorW = rectMonitors[1].right - rectMonitors[1].left;
			   monitorH = rectMonitors[1].bottom - rectMonitors[1].top;
			 } else {
			   monitorW = rectMonitors[0].right - rectMonitors[0].left;
			   monitorH = rectMonitors[0].bottom - rectMonitors[0].top;
			   MessageBoxA(hWnd, "Only one monitor is available!\nFullscreen display is disabled!", "Notice", MB_ICONWARNING);
			   fullscreen_disabled = 1;
			 }			 
			 
			 timerEnumWindowsID = SetTimer(hWnd, IDT_TIMER_ENUM_WINDOWS, enum_windows_period, EnumWindowsTimerProc);
		     if (timerEnumWindowsID == NULL) {
		      MessageBoxA(0, "(timerEnumWindowsID == NULL)", 0,0);
		     }
			 timerIDImageOverlay = SetTimer(hWnd, IDT_TIMER_IMAGE_OVERLAY, image_overlay_period, 0);
			 if (timerIDImageOverlay == NULL) {
		      MessageBoxA(0, "(timerIDImageOverlay == NULL)", 0,0);
		     }
             StartUDP(&con);

			 if (force_fullscreen_disabled) MessageBoxA(hWnd, "Display on external screen is disabled from the config file! Unset force_fullscreen_disabled to activate the second monitor.", "Notice", MB_ICONWARNING);
			 int extMonitor = 0;			 
			 if (rectMonitors.size()>1) extMonitor = 1; 
			 if ((!fullscreen_disabled) && (!force_fullscreen_disabled)) CreateBorderlessWindow(HWND_DESKTOP, hInst, &rectMonitors[extMonitor]); //OK //for the black overlay

		     iTransparency=1; //0;		

			 //SelectVideo(hWnd, 1); //1.2
			 //SelectVideo(hWnd, 1);
			 RestartIdleVideo(hWnd);
			}
		break;  
		case IDM_RESTART_VIDEO: //1.2, #7-7-2014
			    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RestartIdleVideo,
					(LPVOID) hWnd, THREAD_TERMINATE, &RestartIdleVideoID);			
                              //(LPVOID) hwIdleVideo, THREAD_TERMINATE, &RestartIdleVideoID);			
		break;
		case IDM_STOPLISTEN:			
			udp.CloseConnection();
			CloseAllWindows();
			CheckMenuItem((HMENU) GetMenu(hWnd), IDM_LISTEN, MF_UNCHECKED);
             //UDP(&con);
		break; 

		case IDM_REFRESHLIST:
			UpdateStreamMenu();
		break;
		case IDM_LOAD_LISTED_STREAMS:
			 CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadOpenStreams,
                              (LPVOID) 0, THREAD_TERMINATE, &ThrOpenStreamsID); 
		break;
		case IDM_ABOUT:		
		   DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);			
		   //SelectVideo(hWnd, 1);
		break;
		case IDM_EMBRACEPROCESS: //if didn't embraced it after xxx ms pause
             EmbraceStreamsSimple(); 
			 //EnumWindows(EnumWindowsProc, 0);
		break;
		case IDM_RANDOMROTATE:
			RandomRotate(); //if started - stop and vice verse
		break;
		case IDM_TILEWINDOWS:
			TileWindows();
		break;

		case IDM_UPDATE_SETTINGS:
			if (!udp.bStarted)
			{
			  MessageBoxA(hWnd, "Start the server first!", 0, MB_ICONEXCLAMATION); 
			  break;
			}		   
           UpdateSettings();
		break;
		case IDM_VIDEO:	{
			HWND hwTemp = hwIdleVideo;
			SelectVideo(hWnd);
			if (hwIdleVideo!=0) SendMessage(hwTemp, WM_CLOSE, 0, 0);
						}
		break;
		case IDM_EXIT:
			DestroyWindow(hWnd); //WM_DESTROY
			break;		
		} //switch(wmId)
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);		
		// TODO: Add any drawing code here...
		//GetDC(m_hWnd);                 
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:		
		for(int i=0; i<hwndList.size(); i++){			  
			 SendMessage(hwndList[i], WM_CLOSE, 0, 0);						
		}
		for(int i=0; i<maxStreams; i++){
			SendMessage(hwndVideo[i], WM_CLOSE, 0, 0);
		}
		
		udp.CloseConnection();
		ClearDIB();
				//UnhookWindowsHookEx(hhk);
				//UnhookWindowsHookEx(hhk_wmpaint);
				//SaveMsgLog(msgLog, cntCall);				
				//shutdown(Socket,SD_BOTH);
				//closesocket(Socket);
				//WSACleanup();
		SendMessage(hwIdleVideo, WM_CLOSE, 0, 0);

		CloseLogFile(); //#27-7-2014

		PostQuitMessage(0);
		break;

		//Future version suggestion -- create a thread when receiving this message
		case WM_SOCKET: 
		{
			switch(WSAGETSELECTEVENT(lParam))
			{
			  case FD_READ:
					//if (iProcessingUDP == 1) break; //26-6-2013
			  {
				char szIncoming[1024];
				int slen;
				static int buflen = 1024;
				ZeroMemory(szIncoming,sizeof(szIncoming));					
			    int recv_len = recvfrom(udp.s, szIncoming, buflen, 0, (struct sockaddr *) &si_other, &slen);										
			   //!!!! If SetWindowTextA is here -- no Process Rush! 26-6-2013
				//Add delay?
				if (bUpdateWindowTitle) SetWindowTextA(hWnd, szIncoming);

				//printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
				//printf("Data: %s\n" , buf);
				char ip[128];
		
				Sleep(15);
				inet_ntop(AF_INET, (struct sockaddr*)&si_other.sin_addr.s_addr, ip, sizeof(ip));

				/*LPWSAPROTOCOL_INFO
		WSAAddressToString(struct sockaddr*)&si_other.sin_addr.s_addr, 
  _In_      LPSOCKADDR lpsaAddress,
  _In_      DWORD dwAddressLength,
  _In_opt_  LPWSAPROTOCOL_INFO lpProtocolInfo,
  _Inout_   LPTSTR lpszAddressString,
  _Inout_   LPDWORD lpdwAddressStringLength
);*/

				/*
				INT WSAAPI WSAAddressToString(
  _In_      LPSOCKADDR lpsaAddress,
  _In_      DWORD dwAddressLength,
  _In_opt_  LPWSAPROTOCOL_INFO lpProtocolInfo,
  _Inout_   LPTSTR lpszAddressString,
  _Inout_   LPDWORD lpdwAddressStringLength
);*/

				if (strstr(szIncoming, ":END:") !=NULL)
				{ CloseIP(ip);}
				else  
				{ //RTSP!!! //"rtsp"
					bool bDoRead = true;
					if (enable_udp_login_filter==1)
					if (strstr(szIncoming, udp_login_filter) == NULL) bDoRead = false;
			
					if (bDoRead)
					{
						if (iProcessingUDP == 0)
						{
			   				iProcessingUDP = 1;  //EnterCriticalSection? for iProcessingUDP?
							Sleep(udp_sleep_periodA); //Arbitrary				
							OpenStream(szIncoming, topStream, ip);			
							//MessageBoxA(0, "iProcessingUDP=>1",0,0);
							Sleep(udp_sleep_periodB);
							iProcessingUDP = 0;
						}
					}
				}
								
			strcat(szIncoming, ip);
			int newlen = strlen(szIncoming);
	   
			if (sendto(udp.s, szIncoming, newlen, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
			{
				printf("sendto() failed with error code : %d" , WSAGetLastError());
				//Debug -- add flag ...
				// MessageBoxA(0, "sendto() failed with error code : " ,0,  0);
				return 0; //exit(EXIT_FAILURE);
			}	
		} //allows declaration of local vars inside the switch cases, without compier complaining
	    break; //case FD_READ

				case FD_CLOSE:
				{
					MessageBoxA(hWnd, "Client closed connection", "Connection closed!", MB_ICONINFORMATION|MB_OK);
					closesocket(udp.s);					
				}
				break;

				case FD_ACCEPT:
				{
					/*
					int size=sizeof(sockaddr);
					udp.s =accept(wParam,&sockAddrClient,&size);                
					if (Socket==INVALID_SOCKET)
					{
						int nret = WSAGetLastError();
						WSACleanup();
					}
					///SendMessage(hEditIn,
					//	WM_SETTEXT,
					//	NULL,
					//	(LPARAM)"Client connected!");
					//	
					SetWindowTextA(hWnd, "Client connected!");
					*/
				} //switch(WSAGETSELECTEVENT(lParam))
				break;
			}
		} //case WM_SOCKET

	default: return DefFrameProc(hWnd, hwMDIClient, message, wParam, lParam);
	}
	return 0;
}

// Message handler for the about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

/*
char serviceStatusStr[]  = "\"status\":{";
char serviceCodeStr[]  = "\"code\":";
char serviceErrorMessageStr[]  = "\"error_message\":";
char serviceSuccessMessageStr[]  = "\"success_message\":";
int errorCodeMaxLen = 7;
*/	
int CheckAuthenticateResponse(char* sResp, char* msg){
	char cb[300];
	int err;
	int i;
	char *find = strstr(sResp, serviceCodeStr); //"\"code\":");	
	char *findB, *findC;

	if (find!=NULL){
		strncpy(cb, &find[strlen(serviceCodeStr)], errorCodeMaxLen);
		sscanf(cb, "%d", &err);
	}
	else MessageBoxA(m_hWnd, "Can't find error code number", "CheckAuthenticateResponse", 0);

	if (err==0){ //success
		 find = strstr(sResp, serviceSuccessMessageStr);	
		if (find!=NULL){
          i = strlen(serviceSuccessMessageStr) + 1; //skip first "  //find + 
		  int j=0;
		  while(find[i]!='"'){
			  msg[j] = find[i];
			  j++;
			  i++;
		  }		  
		  msg[j] = 0;
          return 1;
		}
	}

	if ((err==404) || (err==10)){ //error
        find = strstr(sResp, serviceErrorMessageStr);	
		if (find!=NULL){
          i = strlen(serviceErrorMessageStr) + 1; //skip first " find + 
		  int j=0;
		  while(find[i]!='"'){
			  msg[j] = find[i];
			  j++;
			  i++;
		  }		  
		  msg[j] = 0;
		}        
		return 0;
	}	
	
	//Else if unknown error was found:
	strcpy(msg, sResp);

	return 0;		
}

void ReadAuthKeyFromBin(char* chPt, int* offs, int maxSymbols, char* result){

   for (int i=0; i<maxSymbols; i++)
   {
      result[i] = chPt[offs[i]];
	  if (result[i] == 0) break;			  
   }
}

//In order to debug the response, set the config param: debug_webservice_update	1
int UpdateSettings() {

    CWebService webService;
	DWORD dwResp[16000];
	int maxSize = 15999;
	char host[700], action[700];
	unsigned int dwDownloadedSize;
	
    strcpy(host, sBaseURL);	
	sprintf(action, "/api/v%s/%s", sVersion, sUpdateClubStreamingSettings); //%s/

	char authKeyLoaded[maxAuthKeySymbols];

	FILE* f = fopen(RECEIVER_REGISTER_PATH, "rb");
	char bin[5000];	
	if (f!=NULL){			
		fread(bin, 1, 4096, f);	    	   
		DWORD* dwPt = (DWORD*) bin;
		fclose(f);
	}

	ReadAuthKeyFromBin(bin, offsKey, maxAuthKeySymbols, authKeyLoaded);

	webService.bDebugMsg = debug_webservice_update==1; //debug_webservice_auth	1

    char sentIP[100]; //forced IP may be different than the real, for debug pursposes of the error "Invaid IP"

	if (use_forced_ip==1) strcpy(sentIP, forced_ip);
	else strcpy(sentIP, udp.localIP);
	
	dwDownloadedSize = webService.postUpdateSettingsAuthKey(sBaseURL, action, iClubID, authKeyLoaded, sentIP, udp.PORT, dwResp, NULL); //serial number

	char* sResp = (char*) dwResp;
	sResp[dwDownloadedSize] = 0; //maxSize
	
	char msg[1024];
	int auth = CheckAuthenticateResponse(sResp, msg); //14-5-2013

	if (auth!=0) 
	{ 
	 MessageBoxA(m_hWnd, "Settings Updated!", "Notice", MB_ICONINFORMATION); //sResp	 	 
	 return 1;
	}
	else {		
		MessageBoxA(m_hWnd, msg, "Error!", MB_ICONSTOP); //sResp		
     	return 0;
	}
}

int Authenticate(char* activationKey){ //to debug webservice, use: debug_webservice_authorize	1
	CWebService webService;
	DWORD dwResp[16000];
	int maxSize = 15999;
	char host[500], action[500];
	unsigned int dwDownloadedSize;

	strcpy(host, sBaseURL);	
	sprintf(action, "/api/v%s/%s", sVersion, sAuthenticateInstallationStr);
	if (debug_webservice_authorize==1){
	  MessageBoxA(0, action, activationKey, 0);
	}

	webService.bDebugMsg = debug_webservice_authorize == 1; 
	dwDownloadedSize = webService.postAuthorize(host, action, activationKey, iClubID, dwResp, NULL); //serial number
	webService.bDebugMsg = false;

	char* sResp = (char*) dwResp;
	sResp[dwDownloadedSize] = 0; //maxSize
		
	char msg[1024];

	int auth = CheckAuthenticateResponse(sResp, msg);

	//auth = 1; //#27-7-2014 it passes through MarkRegistered...

	MessageBoxA(m_hWnd, msg, "Web Service Response", MB_ICONINFORMATION);
	if (auth!=0)
	{ 
	 MessageBoxA(m_hWnd, "Successful Activation!", "Notice", MB_ICONINFORMATION); 
	 MarkRegistered(activationKey);
	 return 1;
	}
	else {
		MessageBoxA(m_hWnd, "Wrong Activation Key!", "Error", MB_ICONSTOP);
     	return 0;
	}
}

// Message handler for the About box.
INT_PTR CALLBACK Register(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char sn[1000];
	int allowed;
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			//Connect to server ***
			//If OK --...
			//Else ... 
			//get text from ...
			unsigned int id = GetDlgItemInt(hDlg, IDD_CLUB_ID, 0, 0); //validation? or on the server? //1.2, 7.7.2014
			//clubidRegister = id;
			iClubID = id;
			SaveClubId(id);

			GetDlgItemTextA(hDlg, IDD_SERIAL, sn, 15);
			allowed = Authenticate(sn); //Calls Mark Registered
			memset(sn, 0, 300); //clear it

			if (allowed == 0) exit(0);

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// Message handler for manual opening a stream
INT_PTR CALLBACK Join(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char s[512];

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK){
			 int hr;
			 int streamID;
		     hr = GetDlgItemTextA(hDlg, IDD_ADDRESS, s, 511);
			  streamID = GetDlgItemInt(hDlg, IDD_STREAMID, 0, 0); //511);			 			 
		/////	 NewProcessB(pathToExe, s, index);
			 index = streamID;
			 OpenStream(s, streamID, "s");
			 
		     //wideBuffer.setText(0, wbf);
		}
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

int monitorIndex = 0;

 //Call once in the beginning
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
  {
	RECT rect;
	rect.bottom = lprcMonitor->bottom;
    rect.top = lprcMonitor->top;
	rect.left = lprcMonitor->left;
	rect.right = lprcMonitor->right;
	rectMonitors.push_back(rect); //(const) &lprcMonitor);
	
	iMonitor++;
   return true;
  }
   
//8-11-2013 Right Click --> Activate Stream to Main Window
// Make sure both are in the same co-ordinates
BOOL IsInside(const RECT* rect, const POINT point) //Changed to pointer from &, from, credit: http://www.davekb.com/browse_programming_tips:win32_is_a_point_inside_a_rectangle:txt
{
	return !(point.x < rect->left || point.x > rect->right || point.y < rect->top || point.y > rect->bottom);
}

void ProcessWmInput(WPARAM wParam, LPARAM lParam){

    UINT dwSize;
	HRESULT hResult = 0;
	char szTempOutput[1024];
	static LPBYTE lpb[2048];
	POINT pt;
	RECT rect;

    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, 
                    sizeof(RAWINPUTHEADER));

    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, 
		sizeof(RAWINPUTHEADER)) != dwSize ) {
         SetWindowTextA(m_hWnd, "sizeof(RAWINPUTHEADER)) != dwSize");
			return;
	}

   RAWINPUT* raw = (RAWINPUT*)lpb;

   if (raw->header.dwType == RIM_TYPEMOUSE) 
   {
	 if (raw->data.mouse.ulButtons == RI_MOUSE_RIGHT_BUTTON_DOWN)
	 {
			//x = raw->data.mouse.lLastX;
			//y = raw->data.mouse.lLastY;
			///pt.x = raw->data.mouse.lLastX;
			///pt.y = raw->data.mouse.lLastY;				
			GetCursorPos(&pt);		
    
	  for (int i=0; i<maxStreams; i++){		
		if (IsWindow(hwndVideo[i]) && (hwndVideo[i]!=hwndActive))
		{			
			GetWindowRect(hwndVideo[i], &rect);
			if (IsInside(&rect, pt))
			{
				MaximizeCurrentRestorePrevious(hwndVideo[i]);				
			}
		}
	  }//for maxStreams	
	 }//RB DOWN
   }//RIM_TYPEMOUSE
      
    return;
}

//1.2, 5-7-2014

//string sIdleVideo;

//HWND hwIdleVideo = 0; //up
DWORD idleVideoProcessID = 0;
 BOOL CALLBACK EnumWindowsIdleVideoProc( HWND hwnd,LPARAM lParam );

void SelectVideo(HWND hw, int mode){  //mode=0 -- open file save, 1 = from config/sIdleVideo
  //sIdleVideo = 
	   //MessageBoxA(0, "SelectVideo", "D", MB_OK | MB_ICONEXCLAMATION);
	////////////////////////////
	if (mode == 0)
	{
	 sIdleVideo = DoFileOpenSave(hw);
	 ////IdleVideoPathSave(); //1.2
	// FILE* f = fopen(RECEIVER_CONFIG_IDLE_VIDEO, "wt"); //"receiverConfig.txt"	
	 //FILE* f = fopen("S:\\xxx\\initVideo.txt", "wt"); //"receiverConfig.txt"	
	//if (f!=NULL) {
	    //FILE* f = fopen(RECEIVER_CONFIG_IDLE_VIDEO, "wt"); 
	///	FILE* f = fopen("initVideo.txt", "wt"); 
	///	fprintf(f, "%s\nAAAAAAA", sIdleVideo.c_str());		
	///	fclose(f);		

		//char wdir[1024];
		char path[1024];
		//_getcwd(wdir, 1024); //#include <direct.h>  //No, get it when starting!
		//sprintf(path, "%s\\config\\idleVideo.txt", wdir);
		sprintf(path, "%s\\config\\idleVideo.txt", workDir);


		 //ofstream fs(RECEIVER_CONFIG_IDLE_VIDEO, ios::out);
		ofstream fs(path, ios::out);

		//ofstream fs("S:\\xxx\\idleVideo.txt", ios::out);
		 if (fs!=NULL){		   
			 fs << sIdleVideo.c_str();				
		 }
		   fs.close();
		//f:\Code-Work\WinTest-11-9-2010_2-REFACTOR-VideolistItem-move\UTF8-Test\Glas2B-WindowsTest\Logger.h
//	}		

	////// MessageBoxA(0, sIdleVideo.c_str(), path, MB_OK | MB_ICONEXCLAMATION);
	}
	
	char cmd[800];
	//sprintf(cmd, "%s -an -loop 0 -fs", sIdleVideo.c_str());
	//sprintf(cmd, "%s", sIdleVideo.c_str());
	//sprintf(cmd, "-i %s -an -loop 0 -fs", sIdleVideo.c_str());
	sprintf(cmd, "-i %s -an -loop 0", sIdleVideo.c_str());
	// ShowWindow(hwndPrevActive, SW_SHOW);	
	STARTUPINFOA si;   
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
	//CreateProcessA( pathToExe, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi );
	//CreateProcessA( "F:\\Lib\\ffmpeg_win32-shared\\bin\\ffplay.exe","C:\\Users\\tosh\\Documents\\1577054_ne_sa_smei_lapai_bulgaria_bulgarian_porn.mp4", NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
	//CreateProcessA( "F:\\Lib\\ffmpeg_win32-shared\\bin\\ffplay.exe", "C:\\Users\\tosh\\Documents\\1577054_ne_sa_smei_lapai_bulgaria_bulgarian_porn.mp4", NULL, NULL, FALSE, NULL, NULL, "F:\\Lib\\ffmpeg_win32-shared\\bin\\", &si, &pi );
	
	
	//CreateProcessA( "F:\\Lib\\ffmpeg_win32-shared\\bin\\ffplay.exe", "-i rtsp://127.0.0.1:8554/2", NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
	//CreateProcessA( "F:\\Lib\\ffmpeg_win32-shared\\bin\\ffplay.exe", "-i C:\\Users\\tosh\\Documents\\1577054_ne_sa_smei_lapai_bulgaria_bulgarian_porn.mp4", NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
	//CreateProcessA( "F:\\Lib\\ffmpeg_win32-shared\\bin\\ffplay.exe", cmd, NULL, NULL, FALSE,  NULL, NULL, NULL, &si, &pi );
	CreateProcessA(pathToExe, cmd, NULL, NULL, FALSE,  NULL, NULL, NULL, &si, &pi );
	idleVideoProcessID = pi.dwProcessId;

	Sleep(2500);
	EnumWindows(EnumWindowsIdleVideoProc, 0);
   

	//"F:\\Lib\\ffmpeg_win32-shared\\bin\\"
	//CREATE_NO_WINDOW
	
	//fplay.bat
	//C:\\Users\\tosh\\Documents\\1577054_ne_sa_smei_lapai_bulgaria_bulgarian_porn.mp4
	//C:\\Users\\tosh\\Documents\\1577054_ne_sa_smei_lapai_bulgaria_bulgarian_porn.mp4
//---------------------------
//F:\Lib\ffmpeg_win32-shared\bin\ffplay.exe
//---------------------------
//OK   
//--------------------------- 
/*
	MessageBoxA(0, pathToExe, cmd, MB_OK | MB_ICONEXCLAMATION);
	Sleep(30);
	CreateProcessA( pathToExe, cmd, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
	Sleep(30);
	//MessageBoxA(0, pathToExe, cmd, MB_OK | MB_ICONEXCLAMATION);
	CreateProcessA( pathToExe, cmd, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
	Sleep(200);
	//MessageBoxA(0, pathToExe, cmd, MB_OK | MB_ICONEXCLAMATION);
	CreateProcessA( pathToExe, cmd, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
	Sleep(230);
	CreateProcessA( pathToExe, cmd, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
	Sleep(300);
	//MessageBoxA(0, pathToExe, cmd, MB_OK | MB_ICONEXCLAMATION);
	CreateProcessA( pathToExe, cmd, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
	Sleep(300);
	CreateProcessA( pathToExe, cmd, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
	Sleep(300);
	//MessageBoxA(0, pathToExe, cmd, MB_OK | MB_ICONEXCLAMATION);
	CreateProcessA( pathToExe, cmd, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
	Sleep(300);
	//MessageBoxA(0, pathToExe, cmd, MB_OK | MB_ICONEXCLAMATION);
	//CREATE_NO_WINDOW
	*/
}

OPENFILENAMEA ofn;

string DoFileOpenSave(HWND hwnd) //HWND hwnd)
{
		  // MessageBoxA(0, "DoFileOpenSave", "D", MB_OK | MB_ICONEXCLAMATION);
    char szFileName[MAX_PATH];
    ZeroMemory(&ofn, sizeof(ofn));
   //  string s1("null");
	 //return s1; //NULL;

    szFileName[0] = 0;

   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFilter = "Video File (avi, mp4, mov, mkv, mpg, ...)\0*.avi;*.mp4;*.mov;*.mkv;*.mpg\0All (*.*)\0*.*\0\0";
   ofn.nFilterIndex = 0; //FilterIndex;
   ofn.lpstrFile = szFileName;

   if (!ofn.lpstrDefExt) ofn.lpstrDefExt = "avi"; //|mp4|mkv|mov"; //!test
   ofn.nMaxFile = MAX_PATH;    
  
      ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST; // | OFN_HIDEREADONLY;
	  string sFile("null"); //NULL; //null;

	  if(GetOpenFileNameA(&ofn)) {
		  sFile.assign(ofn.lpstrFile);
		  return sFile; //ofn.lpstrFile;
	  }
	  else 
      {
		   MessageBoxA(0, "Can't open that file!", "Error!", MB_OK | MB_ICONEXCLAMATION);
            return sFile;
	  }           
}

int iEnumeratingIdle = 0; //to 2 - 1 for the video window,  2 - console

 BOOL CALLBACK EnumWindowsIdleVideoProc( HWND hwnd,LPARAM lParam )	// handle to parent window	// application-defined value)
{
	DWORD prevProc = NULL;
	char s[255];
	bool isListed= false;	

	iEnumerating = 1;	

	GetWindowThreadProcessId(hwnd, &dwProcessID);  //DWORD ThrID = 
   
    //isListed = isContained(hwnd, hwndVideo, maxStreams);  //if (dwProcessID == procSleepessID){ //if ((dwProcessID == processID) && (!isContained(hwnd, hwndVideo, maxStreams))){
   
	if ((dwProcessID == idleVideoProcessID)) 
	{ // && (!isListed)){ //up to 29-5 *******
     ///NO SetParent(hwnd, m_hWnd); //let's see... will it work...
	  ///// hwndList.push_back(hwnd);

	   GetWindowTextA(hwnd, s, 255);	 
	     const char* stats;

	  stats = strstr ((const char*) s, (const char*) exeTemplate);
		       if (stats!=0){  
				   //if (bHideConsoles){
                    ShowWindow(hwnd, SW_HIDE);
					iEnumeratingIdle++;
					if (iEnumeratingIdle==2) { iEnumeratingIdle = 0; return false; 	}
					else return true;
				 }
		 
			 stats = strstr ((const char*) s, (const char*) sIdleVideo.c_str());
			 //Match file name
		       if (stats!=0){  
				   //if (bHideConsoles){
                    //ShowWindow(hwnd, SW_HIDE);
				   hwIdleVideo = hwnd;
				 //  SetParent(hwIdleVideo, m_hWnd); //hide from taskbar? or no NO -- the window disappears?
					iEnumeratingIdle++; 


			if ( (rectMonitors.size()<2) || (force_fullscreen_disabled) ){			
			  SetWindowPos(hwnd, HWND_DESKTOP, rectMonitors[0].left - offset_main_window_x_left, - offset_main_window_y, rectMonitors[0].right -  rectMonitors[0].left + offset_main_window_x,  rectMonitors[0].bottom -  rectMonitors[0].top + offset_main_window_y*2,SWP_NOACTIVATE); // false); //true); //false); //don't repaint, then MAXIMIZE			  
		} else { //second monitor		  
			  SetWindowPos(hwnd,HWND_DESKTOP, rectMonitors[1].left - offset_main_window_x_left, - offset_main_window_y, rectMonitors[1].right -  rectMonitors[1].left + offset_main_window_x,  rectMonitors[1].bottom -  rectMonitors[1].top + offset_main_window_y*2,SWP_NOACTIVATE); // false); //true); //false); //don't repaint, then MAXIMIZE			  
			}	

			if (iEnumeratingIdle==2) { iEnumeratingIdle = 0; return false; 					}
			   
				 //  else
				   //ShowWindow(hwnd, SW_MINIMIZE);				 
			   }
			   
			  		    		 	
		return true;
   	
	} //process
	   
  //next window
   return true;	   
 }



#define RECEIVER_CONFIG_IDLE_VIDEO  "config\\idleVideo.txt"
 
void IdleVideoPath(){		
	char sA[1024];	

	FILE* f = fopen(RECEIVER_CONFIG_IDLE_VIDEO, "rt"); //"receiverConfig.txt"	
	if (f!=NULL){		
		fgets(sA, 999, f);
		fclose(f);		
		sIdleVideo.assign(sA);
		//MessageBoxA(0, sA, "VideoPath", 0);
		//MessageBoxA(0, sA, sIdleVideo.c_str(), 0);
		
	}
}

void IdleVideoPathSave(){		
	FILE* f = fopen(RECEIVER_CONFIG_IDLE_VIDEO, "wt"); //"receiverConfig.txt"	
	if (f!=NULL) {
		fprintf(f, "%s", sIdleVideo.c_str());
		fclose(f);		
	}		
}

void SaveClubId(unsigned int id){
	iClubID = id;
	FILE* f = fopen(RECEIVER_CONFIG_ID, "wt"); //"receiverConfig.txt"	
	if (f!=NULL) { 
		fprintf(f, "%u", id); 
		fclose(f);	
	}	
}
unsigned int LoadClubId(){	
	unsigned int id = 0;
	FILE* f = fopen(RECEIVER_CONFIG_ID, "rt"); //"receiverConfig.txt"	
	if (f!=NULL) { fscanf(f, "%u", &id); fclose(f);	}		
	iClubID = id;
	return id;
}

DWORD RestartIdleVideo(LPVOID dwParam){
	if (hwIdleVideo!=0) SendMessage(hwIdleVideo, WM_CLOSE, 0, 0);
	SelectVideo((HWND) dwParam, 1);
	return 0;
}

void OpenLogFile(){
	flog = fopen("config\\log.txt", "wt");
}

void CloseLogFile(){
	fclose(flog);
}