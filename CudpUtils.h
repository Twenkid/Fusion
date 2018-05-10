// CudpUtils.h
// (C) Todor "Twenkid" Arnaudov 2013
// Licensed to Incept Development for usage in Fusen
// MIT License from May 2018
/*
 23-3-2013, 3:31 Todor Arnaudov 
 The server in this terminology will receive data and display it.
 --> Make win app
	 
 Thanks to  Silver Moon ( m00n.silv3r@gmail.com ) for his sample Simple UDP Server

 Compatibility:
 inet_ntoa and similar functions from WinSock 2 might be not compatible with Windows XP.
 Vista or 7 required. There are alternatives in the MSDN.
*/

	#include <stdio.h>
	#include <winsock2.h>
	#include <windows.h>

	#include <Ws2tcpip.h> //multicast, 4-4-2013
	#include <mswsock.h>

	#pragma comment(lib,"ws2_32.lib") //Winsock Library

	#ifndef WM_SOCKET
	#define WM_SOCKET 104
	#endif

	/*
	//#define SERVER "127.0.0.1"	//ip address of udp server
	#define SERVER "127.0.0.1"	//ip address of udp server
	#define BUFLEN 1024 //512	//Max length of buffer
	#define PORT 8888	//The port on which to listen for incoming data
	#define MULTICAST "224.0.0.1"//  "127.0.0.1" //
	*/
	//int SERVER "127.0.0.1"	//ip address of udp server

	#define BUFLEN 2048 //512	//Max length of buffer
	#define PORT_DEFAULT 8888	//The port on which to listen for incoming data
	#define MULTICAST "224.0.0.1"//  "127.0.0.1" //


	#define DEBUG_JOINING 0

	#define u_int32 UINT32  // Unix uses u_int32

	/*
		Procedure
		==========
		StartWS();
		CreateSocket();
		SetupAddress();
		ListenSocket();
		CloseConnection();
	*/
	class UDPConnectionClient{
	public:
		bool bDoFinish; //Set in PostFinish when initiating close -- for graceful exit

		struct sockaddr_in si_other;
		int s, slen; //=sizeof(si_other);
		char buf[BUFLEN];
		char message[BUFLEN];
		WSADATA wsa;	
		bool bDebugMsg;
		HWND hWnd;

		int PORT;
		//int buflen;
		void SetPort(int portIn=PORT_DEFAULT){
			PORT = portIn;
		}			
		
		UDPConnectionClient(){
			slen=sizeof(si_other);
			bDoFinish = false;
			bDebugMsg = false;
			PORT = PORT_DEFAULT;
		}
		void SetHWND(HWND hw){ //Set the window where to paint
		   hWnd = hw;
		}

		void PostFinish(){
		   bDoFinish = true;
		}
		
		int StartWS(){
			//Initialise winsock
		if (bDebugMsg)  printf("\nInitialising Winsock...");
		if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
		{
			if (bDebugMsg) printf("Failed. Error Code : %d",WSAGetLastError());
			return -1; //exit(EXIT_FAILURE);
		}
		  if (bDebugMsg)  printf("Initialised.\n");
		  return 0;
		}

		int CreateSocket(){
			//create socket
		  if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
		  {
			if (bDebugMsg)  printf("socket() failed with error code : %d" , WSAGetLastError());
			return -1; //exit(EXIT_FAILURE);
		  }
		  return 0;
		}
		
		void SetupAddress(){
		  //setup address structure
		  memset((char *) &si_other, 0, sizeof(si_other));
		  si_other.sin_family = AF_INET;
		  si_other.sin_port = htons(PORT);
		  si_other.sin_addr.S_un.S_addr = inet_addr(MULTICAST); //SERVER); //ERROR?
		}

		void ListenSocket(){ //Call from a thread
			//start communication
		  while(1)
		  {
			if (bDebugMsg)  printf("Enter message : ");
			gets(message);
			
			//send the message
			if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
			{
				if (bDebugMsg) printf("sendto() failed with error code : %d" , WSAGetLastError());
				return; //exit(EXIT_FAILURE);
			}
			
			//receive a reply and print it
			//clear the buffer by filling null, it might have previously received data
			memset(buf,'\0', BUFLEN);
			//try to receive some data, this is a blocking call
			if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == SOCKET_ERROR)
			{
				if (bDebugMsg) printf("recvfrom() failed with error code : %d" , WSAGetLastError());
				return; //exit(EXIT_FAILURE);
			}
			
			puts(buf);
			SetWindowTextA(hWnd, buf); //bDoFinish
		  }
		}

		void CloseConnection(){
			shutdown(s,SD_BOTH);
			closesocket(s);
			//leave_source_group(PORT, inet_addr(MULTICAST), inet_addr(localIP), INADDR_ANY);
			WSACleanup();
			//shutdown(Socket,SD_BOTH);
			//	closesocket(Socket);
			//	WSACleanup();
		}
		  
	};

	/////////////
	//* SERVER *//
	/////////////
	/*
		Procedure:
		==========
		StartWS();
		CreateSocket();
		SetupAddress();
		ListenSocket();
		CloseConnection();
	*/


	class UDPConnectionServer{
	public:
		bool bDoFinish; //Set in PostFinish when initiating close -- for graceful exit
		bool bDebugMsg;
		 /* Client
		struct sockaddr_in si_other;
		int s, slen, recv_len; //=sizeof(si_other);
		/char buf[BUFLEN];
		char message[BUFLEN];
		WSADATA wsa;	
		*/

		SOCKET s;
		struct sockaddr_in server, si_other; //don't forget to set si_other
		int slen , recv_len;
		char buf[BUFLEN];
		WSADATA wsa;
		char* localIP;
		bool bStarted;		
		HWND hWnd;
		int PORT;
		
	void SetPortUDP(int portIn=PORT_DEFAULT){
		PORT = portIn;
	}
		
	//http://msdn.microsoft.com/en-us/library/windows/desktop/ms739173(v=vs.85).aspx
	/* OUT: whatever setsockopt() returns */
	int join_source_group(int sd, u_int32 grpaddr, u_int32 srcaddr, u_int32 iaddr)
	 {
	   struct ip_mreq_source imr; 
	   
	   imr.imr_multiaddr.s_addr  = grpaddr;
	   imr.imr_sourceaddr.s_addr = srcaddr;
	   imr.imr_interface.s_addr  = iaddr;	   
	   return setsockopt(sd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (char *) &imr, sizeof(imr));  
	   //return setsockopt(sd, IPPROTO_UDP, IP_ADD_SOURCE_MEMBERSHIP, (char *) &imr, sizeof(imr));  
	  }

	int leave_source_group(int sd, u_int32 grpaddr, u_int32 srcaddr, u_int32 iaddr)
	{
	   struct ip_mreq_source imr;

	   imr.imr_multiaddr.s_addr  = grpaddr;
	   imr.imr_sourceaddr.s_addr = srcaddr;
	   imr.imr_interface.s_addr  = iaddr;

	   return setsockopt(sd, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, (char *) &imr, sizeof(imr));	   
	 }

	UDPConnectionServer(){
			slen=sizeof(si_other);
			bDoFinish = false;
			bStarted = false;
			bDebugMsg = false;
			PORT = PORT_DEFAULT;
	}

		void SetHWND(HWND hw){ //Set the window where to paint
		   hWnd = hw;
		}

		void PostFinish(){
		   bDoFinish = true;
		}
		
		int StartWS(){

		  if (bStarted) return 0; //Already started

			//Initialise winsock
		   if (bDebugMsg) printf("\nInitialising Winsock...");

		if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
		{
			if (bDebugMsg) {
				printf("Failed. Error Code : %d",WSAGetLastError());
				MessageBoxA(0, "Failed. Error Code :" , 0, 0);
			}
			return -1;  
			//exit(EXIT_FAILURE); //would just crash
		}
		  if (bDebugMsg) printf("Initialised.\n");
		  return 0;
		}

		int CreateSocket(){			
		 //Create a socket
		 //if((s = socket(AF_INET , SOCK_DGRAM, 0 )) == INVALID_SOCKET)
		 if((s = socket(AF_INET , SOCK_DGRAM, IPPROTO_UDP )) == INVALID_SOCKET)
			  //IPPROTO_UDP
		  {
			  if (bDebugMsg) {
				  printf("Could not create socket : " , WSAGetLastError());
				  MessageBoxA(0, "Could not create socket  : " ,0, 0);
			  }
			return -1;
		   }
		   if (bDebugMsg) printf("Socket created.\n");
		   return 0;
		}

		//E0 00 00 01 == 224.0.0.1
		void SetupAddress()
		{

			 char ac[80];			
		if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
		   // cerr << "Error " << WSAGetLastError() <<
		  //          " when getting local host name." << endl;
			if (bDebugMsg)  MessageBoxA(0, "ERROR getting local host name.", 0,0);
			return; // 1;
		}
		//cout << "Host name is " << ac << "." << endl;
		if (bDebugMsg) 
		if (DEBUG_JOINING)	MessageBoxA(0, "Host name is: ", ac,0);

		 hostent *phe = gethostbyname(ac);
		if (phe == 0) {
			//cerr << "Yow! Bad host lookup." << endl;
			if (bDebugMsg)  MessageBoxA(0, "Bad host lookup", 0,0);
			return; // 1;
		}
		int i;
		struct in_addr addr;
		for ( i = 0; phe->h_addr_list[i] != 0; ++i) {       
			memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		   // cout << "Address " << i << ": " << inet_ntoa(addr) << endl;
			
				if (bDebugMsg) 
				if (DEBUG_JOINING) MessageBoxA(0, inet_ntoa(addr), 0,0);
		}

		 hostent* localHost;

		// Get the local host information
		localHost = gethostbyname("");
		 
		//Create a socket
		localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);// ---  8-5-2013

		//strcpy(localIP, inet_ntoa (*(struct in_addr *)*localHost->h_addr_list));
		if (bDebugMsg) MessageBoxA(0, localIP, "Local IP?", 0);

		 //Prepare the sockaddr_in structure
		  server.sin_family = AF_INET;
		  server.sin_addr.s_addr = INADDR_ANY; //inet_addr(localIP); //INADDR_ANY; //BROADCAST; //INADDR_ANY; //See http://stackoverflow.com/questions/1524946/udp-multicast-using-winsock-api-differences-between-xp-and-vista
		  server.sin_port = htons( PORT );
		  
		//MSDN
		//localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);
		 //MULTICAST! Join Group

		 //inet_addr(inet_ntoa(addr))
		 join_source_group(PORT, inet_addr(MULTICAST), inet_addr(localIP), INADDR_ANY);  //http://www.rsdn.ru/forum/cpp.applied/4353345.flat	

		 char cb[200];
		 if( bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
		 {
			 if (bDebugMsg){
				 sprintf(cb, "Bad Bind failed with error, error code %d ",  WSAGetLastError());
				 MessageBoxA(0, cb, "Error",0);
				 //MessageBoxA(0, "Bad Bind failed with error code lookup", cb,0);
			     //printf("Bind failed with error code : %d" , WSAGetLastError());
			 }
			//exit(EXIT_FAILURE); //Don't exit
		  }
		  if (bDebugMsg) puts("Bind done");	

		  memset((char *) &si_other, 0, sizeof(si_other));
		  si_other.sin_family = AF_INET;
		  si_other.sin_port = htons(PORT);
		  si_other.sin_addr.S_un.S_addr = inet_addr(MULTICAST); //"127.0.0.1"); //SERVER); //ERROR?
	}

		void ListenSocket(){ //Call from a thread
		 //start communication
		 //keep listening for data
			 char cb[200];
	
			if (bDebugMsg) 
			if (DEBUG_JOINING) MessageBoxA(0, "ListenSocket WSAAsyncSelect?" ,0,  0);

			char ip[128];

				int nResult=WSAAsyncSelect(s,
						hWnd,
						WM_SOCKET,
						(FD_CLOSE|FD_ACCEPT|FD_READ));
			if(nResult)
				{
		if (bDebugMsg) MessageBoxA(hWnd,
						"WSAAsyncSelect failed",
						"Critical Error",
						MB_ICONERROR);
					//SendMessageA(hWnd,WM_DESTROY,NULL,NULL);
					//break;
					bStarted = false;
				}
			else bStarted = true;			
		}

		void CloseConnection(){
			//closesocket(s);
			//WSACleanup();
			leave_source_group(PORT, inet_addr(MULTICAST), inet_addr(localIP), INADDR_ANY);
			shutdown(s,SD_BOTH);
			closesocket(s);		
			WSACleanup();
			bStarted = false;

		}
		  
	};