// MDIChild.h
// Author/Contributor: Todor Arnaudov
// Derived/modified from the following example:
// http://www.winprog.org/tutorial/app_four.html
//

#include <windows.h>


LRESULT CALLBACK MDIChildWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern HWND m_hWnd;

BOOL SetUpMDIChildWindowClass(HINSTANCE hInstance, LPCSTR childClassName )
{
    WNDCLASSEXW wc;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MDIChildWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL; //LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = _T("mdichild"); //childClassName; //g_szChildClassName;
    wc.hIconSm       = NULL; //LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassExW(&wc))
    {
       // MessageBoxA(0, "Could Not Register Child Window", "Oh Oh...",
       //     MB_ICONEXCLAMATION | MB_OK);
		
		DWORD err = GetLastError();
		char c[200];
		//sprintf(c, "%d", err);
		sprintf(c, "Could Not Register Child Window (%d)", err); //!!
		MessageBoxA(0, c, 0, 0);

        return FALSE;
    }
    else
        return TRUE;
}

HWND CreateNewMDIChild(HWND hMDIClient, LPCSTR childClassName) //OK!
{ 
    MDICREATESTRUCTW mcs;
	HWND hChild;
   

    mcs.szTitle = _T("Main"); //[Untitled]";
    mcs.szClass = _T("mdichild"); //childClassName; //g_szChildClassName;
    mcs.hOwner  = GetModuleHandleA(NULL);
    mcs.x = mcs.cx = 0; //CW_USEDEFAULT;
    mcs.y = mcs.cy = 0; //CW_USEDEFAULT;
	mcs.cx = 320;
	mcs.cy = 240;

    mcs.style = MDIS_ALLCHILDSTYLES;

    hChild = (HWND)SendMessageW(hMDIClient, WM_MDICREATE, 0, (LONG)&mcs);
    if(!hChild)
    {
      //  MessageBoxA(hMDIClient, "MDI Child creation failed.", "Oh Oh...",
       //     MB_ICONEXCLAMATION | MB_OK);
		DWORD err = GetLastError();
		char c[200];
		//sprintf(c, "%d", err);
		sprintf(c, "MDI Child creation failed, err = %d, MDIClass = %s,  mcs.szClass = %s", err, childClassName,  mcs.szClass ); //!!
		MessageBoxA(0, c, 0, 0);
    }

	HMENU	hMenu;		
	if ((hMenu = GetSystemMenu(hChild, FALSE)) != NULL)
	{		
	 EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	 EnableMenuItem(hMenu, SC_MAXIMIZE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	 EnableMenuItem(hMenu, SC_MINIMIZE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	} else MessageBoxA(0, "NUll! (hMenu = GetSystemMenu(hChild, FALSE)) != NULL)",0,0);
    return hChild;
}

LRESULT CALLBACK MDIChildWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:        
        break;

		case WM_SYSCOMMAND:
			switch(wParam){
				case SC_CLOSE:
				case SC_MAXIMIZE:
				case SC_MINIMIZE: //MessageBoxA(0, "SC_CLOSE, ... ?", 0,0);
				return 1; //Ignore the system messages				
			}	
		break;

        case WM_MDIACTIVATE:
        {
            DrawMenuBar(m_hWnd); //g_hMainWindow);
        }
        break;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {				
            }
        break;
		
        case WM_SIZE:
        {			
        }        
		break;

        default:
            return DefMDIChildProc(hwnd, msg, wParam, lParam);
    
    }
    return 0;
}
