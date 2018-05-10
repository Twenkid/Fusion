// WebService.h 
// WinInet classes for http POST transactions
// (C) Todor "Twenkid" Arnaudov 2013
// Licensed to Incept Development for usage in Fusen
// MIT License from May 2018

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include "stdafx.h"
#include "windows.h"
#include "wininet.h"  //OK
#include <string.h>

#pragma comment(lib, "Wininet")


/*
struct LoginData{
	char user[16];
	char pass[16];
	LoginData(char* u, char* p){
		strcpy(user, u);
		strcpy(pass, p);
	}
	LoginData(){ clear();}
	void clear(){
		strcpy(user, " ");
		strcpy(pass, " ");
	}
};
*/

struct LastError{
	bool bError;
	DWORD lastError;
	LastError(){
		bError = false;
	    lastError = 0;
	}
};

class CWebService {
public: //All is public for simpler development, further implementation should use accessor methods
	LastError error;	
	bool bDebugMsg;

	void SetLastError(){
		error.lastError = GetLastError();
	    HRESULT hr = HRESULT_FROM_WIN32(error.lastError);	   
		error.bError = true;
	}

	CWebService(){
		bDebugMsg = false; //true; //false; // false; //		
	}


 unsigned int postAuthorize(char* host, char* action, char* authNumber, int clubID, DWORD* lpOutBuffer, LastError* er) { //;dwReceived, int size){    
	
	char hdrs[200], frmdata[500];	
	char cb[1000]; //buffer for printing
	//sprintf(hdrs, "Content-Type: multipart/form-data; boundary=----Boundary"); //application/x-www-form-urlencoded"); // application/json");
	sprintf(hdrs, "Content-Type: application/x-www-form-urlencoded"); //application/x-www-form-urlencoded"); // application/json");
	//sprintf(hdrs, " ");

	sprintf(frmdata, "club_id=%d&auth_number=%s", clubID, authNumber); //"6.1"); //OsVersion); //19-3-2013  
	////sprintf(frmdata, "----Boundary\nContent-Disposition: form-data; name=\"club_id\"\n13\n----Boundary\nContent-Disposition: form-data; name=\"auth_number\"\nA096-057C\n----Boundary");
    if (bDebugMsg)  MessageBoxA(0, frmdata, hdrs, 0);

    static LPCSTR accept[2]={"*/*", NULL};	
	sprintf(cb, "host: %s, action: %s, clubID: %d, auth_number: %s\n frmdata = %s", host, action, clubID, authNumber, frmdata);
	if (bDebugMsg)  MessageBoxA(0, cb, 0, 0);
        
    HINTERNET hSession = InternetOpenA("MyAgent", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
   
   if (hSession == 0) {
	SetLastError();
    return 0;
   }

   if (bDebugMsg)  MessageBoxA(0, "After if (hSession == 0) {", "Debug", 0);

   HINTERNET hConnect = InternetConnectA(hSession, host, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);

   if (hConnect == 0)
   {
	SetLastError();
    return 0;
   }

   if (bDebugMsg)  MessageBoxA(0, "After if (hConnect == 0) {", "Debug", 0);

   //INTERNET_FLAG_NO_UI   
   HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", action, NULL, NULL, accept, 0, 1); //INTERNET_FLAG_NO_UI, 1)

   if (hRequest==0) {
	 SetLastError();
     return 0;
   }

   if (bDebugMsg)  MessageBoxA(0, "After if (hRequest == 0) {", "Debug", 0);

   if (bDebugMsg)  MessageBoxA(0,frmdata, "FrmData before HttpSendRequestA", 0);
   bool bh = HttpSendRequestA(hRequest, hdrs, strlen(hdrs), frmdata, strlen(frmdata));

   DWORD lastError=0;
   if (!bh){
	   lastError = GetLastError();
	   HRESULT hr = HRESULT_FROM_WIN32(lastError);
	   //printf("\nError: %d, HR = %d", lastError, hr);
	   SetLastError();
	   return 0;
   }

   if (bDebugMsg)  MessageBoxA(0, "After if (!bh){ {", "Debug", 0);
   
   char lpszData[10000]; //Response Data Buffer   
   DWORD dwSize, dwDownloaded; 
   bool ba;

   //bool ba = HttpQueryInfoA(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF,(LPVOID)lpOutBuffer,&dwSize,NULL);
   //Don't use the above, this Query doesn't work in WinInet
   ba = true;

   bool bb,bc;

   if (ba){             
	       if (bDebugMsg)  MessageBoxA(0, "   if (ba){    ", "Debug", 0);
	       bb = InternetQueryDataAvailable(hRequest,&dwSize,0,0);

		 if (bb)
		 {
			 if (bDebugMsg)  MessageBoxA(0, "   if (bb) ", "Debug", 0);         
			   bc = InternetReadFile(hRequest,(LPVOID)lpszData,dwSize,&dwDownloaded);
			   if (bc){
				   if (bDebugMsg)  MessageBoxA(0, "   if (bc) ", "Debug", 0);
				   lpszData[dwDownloaded+1] = 0;
				   lpszData[dwDownloaded+2] = 0;
				  // printf("%s %d", (char*)lpszData, dwDownloaded);
				   memcpy(lpOutBuffer, lpszData, dwDownloaded);//+1); not +1
			   }
		 }
   } 

   if (!ba || !bb || !bc) {
	         if (bDebugMsg)  MessageBoxA(0, "ERROR if (!ba || !bb || !bc) { {", "Error", 0);			 
			 if (bDebugMsg)  MessageBoxA(0, (const char*) lpszData, "Data!!!!!", 0); //
	         SetLastError();

	   char cb2[200];
 	   sprintf(cb2, "Error: %d", error.lastError);
	   if (bDebugMsg)  MessageBoxA(0, cb2, 0, 0); //

	   return 0;  
   } //postWI
  
   if (bDebugMsg)  MessageBoxA(0, (const char*) lpszData, "Data", 0);
   return dwDownloaded;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  int postUpdateSettingsAuthKey(char* host,char* action,int iClubID, char* authNumber, char* localIP, int port, DWORD* lpOutBuffer, LastError* er=NULL)
  {
	char hdrs[200], frmdata[1000];	
   //sprintf(hdrs, "Content-Type: multipart/form-data"); //application/x-www-form-urlencoded"); // application/json");
	sprintf(hdrs, "Content-Type: application/x-www-form-urlencoded"); //applic
	
	sprintf(frmdata, "club_id=%d&auth_number=%s&ip_address=%s&port=%d", iClubID, authNumber, localIP, port); //"6.1"); //OsVersion); //19-3-2013  	
	
    static LPCSTR accept[2]={"*/*", NULL};

	char cb[1000];
	sprintf(cb, "hdrs: %s\nfrmdata: %s\n\n\nhost: %s, action: %s, clubID: %d, authNumber: %s, localIP: %s, port: %d", hdrs, frmdata, host, action, iClubID, authNumber, localIP, port);
	if (bDebugMsg) MessageBoxA(0, cb, "Debug", 0);
    
    HINTERNET hSession = InternetOpenA("MyAgent", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
   
   if (hSession == 0) {
	SetLastError();
    return 0;
   }

   if (bDebugMsg)  MessageBoxA(0, "After if (hSession == 0) {", "Debug", 0);

   HINTERNET hConnect = InternetConnectA(hSession, host, // host.c_str(), //localhost",
      INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);

   if (hConnect == 0) {
	SetLastError();
    return 0;
   }
   if (bDebugMsg)  MessageBoxA(0, "After if (hConnect == 0) {", "Debug", 0);

//   HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST",
//	    action, NULL, NULL, accept, 0, 1);
   HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST",
	    action, "HTTP/1.1", NULL, accept, INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE, 1);

   if (hRequest ==0) {
	 SetLastError();
    return 0;
   }
      if (bDebugMsg)  MessageBoxA(0, "After if (hRequest == 0) {", "Debug", 0);

   bool bh = HttpSendRequestA(hRequest, hdrs, strlen(hdrs), frmdata, strlen(frmdata));
 
   DWORD lastError=0;
   if (!bh){
	   lastError = GetLastError();
	   HRESULT hr = HRESULT_FROM_WIN32(lastError);
	   printf("\nError: %d, HR = %d", lastError, hr);
	   SetLastError();
	   return 0;
   }

   if (bDebugMsg)  MessageBoxA(0, "After if (!bh){ {", "Debug", 0);
   
   char lpszData[10000]; // = new char[10000];   
   DWORD dwSize, dwDownloaded; 
  
   bool ba; 

   //HttpQueryInfoA - doesn't work correctly with WinInet, don't use
   //bool ba = HttpQueryInfoA(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF,(LPVOID)lpOutBuffer,&dwSize,NULL);
   //SetLastError();

   ba = true;

   bool bb,bc;

   if (ba)
   {    
     if (bDebugMsg)  MessageBoxA(0, "   if (ba){    ", 0, 0);
     bb = InternetQueryDataAvailable(hRequest,&dwSize,0,0);
	 if (bb)
	 {
      if (bDebugMsg)  MessageBoxA(0, "   if (bb) ", "Debug", 0);
      bc = InternetReadFile(hRequest,(LPVOID)lpszData,dwSize,&dwDownloaded);
	    if (bc)
	    {
		  if (bDebugMsg)  MessageBoxA(0, "   if (bc) ", "Debug", 0);
		  lpszData[dwDownloaded+1] = 0;
		  lpszData[dwDownloaded+2] = 0;
		  //printf("%s %d", (char*)lpszData, dwDownloaded);
	       memcpy(lpOutBuffer, lpszData, dwDownloaded);//+1); not +1
		 }
	 }
   }

   if (!ba || !bb || !bc) {
	         if (bDebugMsg)  MessageBoxA(0, "ERROR if (!ba || !bb || !bc) { {", "Error", 0);			 
			 if (bDebugMsg)  MessageBoxA(0, (const char*) lpszData, "Data", 0); //
	         SetLastError();

	   char cb2[200];
 	   sprintf(cb2, "Error: %d", error.lastError);
	   if (bDebugMsg)  MessageBoxA(0, cb2, "Error", 0); //

	   return 0;  
   }

     if (bDebugMsg)  MessageBoxA(0, (const char*) lpszData, "Data", 0);
     return dwDownloaded;
  } //postUpdateSettingsAuthKey


};