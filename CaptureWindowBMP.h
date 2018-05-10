// (C) Todor "Twenkid" Arnaudov 2013
// All rights reserved
// http://twenkid.com
// http://research.twenkid.com
// Licensed to Incept Development
// Non Disclosure Agreement
// The source is not to be shared with third parties or resold

#include "stdafx.h"
#include <windows.h>
#include <windowsx.h> //SelectBitmap
#include <stdio.h>

HBITMAP hBitmap = NULL;
BYTE*  pData = NULL;
HBITMAP hDIB = 0;
HWND hwPopupMain = NULL; //Borderless
HWND hwPopupPrev = NULL; //Borderless
HWND hwBlackBar = NULL;
HWND hwMDIPanel = NULL;
HWND hwMDIChild = NULL;
HWND hwImageOverlay = NULL;
extern  int image_overlay_x, image_overlay_y, image_overlay_width, image_overlay_height;

int iTitleBarHeight = 30;
int iStoppedPopup = 0;
int iPopupX = 8;
int iPopupY = 8;

int monitorW, monitorH; //set to rect...

LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
HBRUSH hBrush = NULL;

extern VOID OnPaintImageOverlay(HDC hdc);

//Create hBrush before creating this window, hwPopup ...
LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
    int nPos;	

	int maxItems=100; //max Selected
	int selItems[100]; //better solution!!!
    int numSel;
	//static HMENU hMenu;
    POINT point;
	char cb[500];
	//static int btnId[] = {IDM_SETTINGS, IDM_OPEN_AVI, IDM_LOAD_PROJECT, IDM_SAVE_SELECTED_A, IDM_SAVE_PROJECT, IDM_RESET_D3D, IDM_TOOLBOX_BASIC_TEST, IDM_SCRIPTS_CALL};
	static int iPaint = 0;
	

	switch (message)
	{		
	   case WM_PAINT:
		  hdc = BeginPaint(hWnd, &ps);		
		  SelectObject(hdc, hBrush);
		  Rectangle(hdc, 0, 0, monitorW, monitorH); //Glas.IntVars[32]); //Glas.IntVars[30]*2);				
		  EndPaint(hWnd, &ps);
	   break;

	   case WM_DESTROY:
	  	  if (hBrush!=NULL) DeleteObject(hBrush);
		  hBrush = NULL;		
	    break;
	}
		
	return DefWindowProc(hWnd, message, wParam, lParam);(hWnd, message, wParam, lParam);
}

/*
WS_EX_LAYERED
0x00080000	
 The window is a layered window. This style cannot be used if the window has a class style of either CS_OWNDC or CS_CLASSDC.
Windows 8:  The WS_EX_LAYERED style is supported for top-level windows and child windows. Previous Windows versions support WS_EX_LAYERED only for top-level windows.
*/

//in Win7 not supported for child windows! WS_CHILD  //WS_CHILD
void CreateBorderlessWindow(HWND hParent, HINSTANCE hInstance, LPRECT lpRect){ //10-5-2013 OK
	if (!IsWindow(hwPopupMain)){
    hwPopupMain = CreateWindowExA(0, "STATIC", " ", 
      SS_NOTIFY | WS_CHILD | WS_VISIBLE | SS_LEFT | WS_POPUP | WS_EX_LAYERED, //no border |WS_BORDER, //WS_BORDER  
      lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, //nButtonWidth * 4, nButtonHeight*4,
      hParent, (HMENU) 0 , hInstance, NULL); //HMENU - ID, to change labels on the fly
      if (hwPopupMain == NULL) MessageBoxA(0, "hwPopupMain == NULL) ", 0, 0);
	  else {
		  	    hBrush = CreateSolidBrush(RGB(0, 0, 0));
		SetWindowLong(hwPopupMain, GWL_WNDPROC, (LONG)OverlayWndProc);  
	  }
	}	
}



void OnPaint(HWND hwSrc, HWND hwnd)
{
        HDC hBitmapdc, hWindowdc;
        HBITMAP hOld;
        PAINTSTRUCT ps;
        RECT rc, rect, rectSrc;
        int nWid, nHt;		

		if (iStoppedPopup==1) return;

        // If the bitmap is present, draw it
        if (hBitmap)
        {
                // Get window dc and start paint
                hWindowdc = BeginPaint(hwnd, &ps);
                hWindowdc = GetDC(hwnd); //uncommented! 26-6-2013 NO?!
                // Create compatible dc
                hBitmapdc = CreateCompatibleDC(hWindowdc);
                // Select the bitmap into the dc
                hOld = SelectBitmap(hBitmapdc,hBitmap);
                // Get width and height of this window
                GetClientRect(hwnd,&rc);
                // Get the dimensions of the bitmap
                nWid = GetSystemMetrics(SM_CXSCREEN);
                nHt = GetSystemMetrics(SM_CYSCREEN);
  				
				//NO! -- the above is only for full screen

				GetWindowRect(hwSrc, &rectSrc); //Main Window, external
				nWid = rectSrc.right - rectSrc.left;
				nHt = rectSrc.bottom - rectSrc.top;

				GetWindowRect(hwnd, &rect);
			    //int xHwnd = rect.left;
			    //int yHwnd = rect.top;
				//nWid = rect.right - rect.left;
				//nHt = rect.bottom - rect.top;

                // Draw the bitmap to the window leaving a border of 20 pixels
                //StretchBlt(hWindowdc,0,0,rc.right,rc.bottom,
                //hBitmapdc,0,0,nWid,nHt,SRCCOPY);

			//	StretchBlt(hWindowdc,0,0,rc.right,rc.bottom, //All including titlebar
             //   hBitmapdc,0,0,nWid,nHt,SRCCOPY);

				StretchBlt(hWindowdc,iPopupX,iPopupY ,rc.right,rc.bottom,
					hBitmapdc,0,iTitleBarHeight,nWid,nHt-iTitleBarHeight,SRCCOPY);
				
				
                // Delete bitmap dc
                DeleteDC(hBitmapdc);
                // Tell windows that you've updated the window
                EndPaint(hwnd,&ps);
				ReleaseDC(hwnd, hWindowdc); //uncomment 26-6-2013 //EndPaint == ReleaseDC? 17-6-2013
        }
		//else
		//	MessageBoxA(0, "hBitmap == NULL", 0, 0);
}

//REFACTOR, hwSrc == hwWindow
void CaptureScreen(HWND hParent, HWND hwSrc) //, HWND hwWindow) // HBITMAP hBitmap)
{
        HDC hSrcdc, hWindowdc, hBitmapdc; 
        int nWid, nHt; // Stores the height and width of the screen
        HBITMAP hOriginal;
		BITMAPINFO bmpInfo;
		RECT lpRect;

	   if (iStoppedPopup==1) return; //12-5-2013		

		if (hBitmap!=NULL) {
			DeleteObject(hBitmap); //14-3	
			hBitmap = NULL;
		}
		
        // Get a handle to the screen device context

        hSrcdc = GetWindowDC(hwSrc); //HWND_DESKTOP); //!!!

		//////hWindowdc = GetWindowDC(hwWindow); //Dest  No
        //if (hDesktopdc)
		if (hSrcdc)
        {
                // Get width and height of screen
                nWid = GetSystemMetrics(SM_CXSCREEN); //DESKTOP
                nHt = GetSystemMetrics(SM_CYSCREEN); //DESKTOP

				nWid = GetSystemMetrics(SM_CXSCREEN);
                nHt = GetSystemMetrics(SM_CYSCREEN);

				GetWindowRect(hwSrc, &lpRect); 	// address of structure for window coordinates
                int dx = abs(lpRect.left - lpRect.right);
	            int dy = abs(lpRect.bottom - lpRect.top);

				nWid = dx;
				nHt = dy;

				//nWid = 640;
				//nHt = 480;
				//Getting from the Main Window, which is whole screen
				//Get windopw dimensions
				//nWid = //GetSystemMetrics(SM_CXSCREEN);
                //nHt = GetSystemMetrics(SM_CYSCREEN);
				
		        bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		        bmpInfo.bmiHeader.biWidth =  nWid;
				bmpInfo.bmiHeader.biHeight =  nHt;
				bmpInfo.bmiHeader.biBitCount = 24;//32; //or 24?
				bmpInfo.bmiHeader.biPlanes = 1;
				bmpInfo.bmiHeader.biCompression =  BI_RGB;
				bmpInfo.bmiHeader.biSizeImage =  0;
				bmpInfo.bmiHeader.biXPelsPerMeter =  0;
				bmpInfo.bmiHeader.biYPelsPerMeter =  0;
				bmpInfo.bmiHeader.biClrUsed =  0;
				bmpInfo.bmiHeader.biClrImportant =  0;

				char cb[300];
				//sprintf(cb, "Width/Height = %d, %d", nWid, nHt);
			    //MessageBoxA(0, cb, 0, 0);
                // Create a compatible bitmap

                hBitmap = CreateCompatibleBitmap(hSrcdc,nWid,nHt);

				DWORD lastError = 0;
				if (hBitmap == NULL)
				{
					lastError = GetLastError();
				    sprintf(cb, "LastError = %d", lastError);
				    MessageBoxA(0, cb, 0, 0);					
				}
				else {
					sprintf(cb, "hBitmap = %x", hBitmap);
				    //MessageBoxA(0, cb, 0, 0);					
				}

		if (pData == NULL) {
			pData = new BYTE[bmpInfo.bmiHeader.biBitCount/8 * nWid * nHt];			
		   ///	MessageBoxA(0, "Data = new BYTE[bmpInfo.bmiHeader.biBitCount/8 * nWid * nHt];", 0, 0);
		}

				//hDIB = CreateDIBSection(hDesktopdc, &bmpInfo, DIB_RGB_COLORS, (void**) pData, 0, 0);
		 if (hDIB == NULL) hDIB = CreateDIBSection(hSrcdc, &bmpInfo, DIB_RGB_COLORS, 0, 0, 0); 						
                				
                hBitmapdc = CreateCompatibleDC(hSrcdc);
                // Select Bitmap
                hOriginal = (HBITMAP)SelectBitmap(hBitmapdc, hBitmap);
                // Copy pixels from screen to the BITMAP
                BitBlt(hBitmapdc,0,0,nWid,nHt,hSrcdc,0,0,SRCCOPY);
				
				///////////////////////////
				HDC iDC = CreateCompatibleDC(0); // create dc to store the image
                SelectObject(iDC, hDIB); // assign the dib section to the dc

				BitBlt(iDC, 0, 0, bmpInfo.bmiHeader.biWidth, bmpInfo.bmiHeader.biHeight, hSrcdc, 0, 0, SRCCOPY); // copy hdc to our hdc
                GetDIBits(iDC, hDIB, 0, bmpInfo.bmiHeader.biHeight, &pData[0], &bmpInfo, DIB_RGB_COLORS);				
				//pData
				if (hDIB == NULL)
				{
				  //printf( "hDIB = 0 (%d).\n", GetLastError() );
		          DWORD err = GetLastError();
		          char c[100];
		          sprintf(c, "hDIB == NULL %d", err);
		          MessageBoxA(0, c, 0, 0);
				}			
			///////////////////////////
			///////////////////////////

                // Delete the hBitmapdc as you no longer need it
                DeleteDC(hBitmapdc);
				DeleteDC(iDC);
                // Release the desktop device context handle                
				//  ReleaseDC(HWND_DESKTOP, hDesktopdc);
				ReleaseDC(hwSrc, hSrcdc);

                // Invalidate the window
                //UpdateWindow(hParent);			 -- Doesn't update
				OnPaint(hwSrc, hParent); //7-1-2013
		}
		else
		{
			//MessageBoxA(0, "hSrcdc == NULL", 0, 0);
			if (hBitmap!=NULL) {
				DeleteObject(hBitmap); //14-3	
				hBitmap = NULL;
			}
			return;
		}
			//If error - stop timers?
		
}

void ClearDIB(){
	if (hDIB!=NULL) DeleteObject(hDIB);
}