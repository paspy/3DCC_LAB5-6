
#include "inc/DirectXLayer.h"
#include "resource.h"
#include "mmsystem.h"
#pragma comment(lib, "WinMM.lib")
#include <string>
using namespace std;

#pragma comment(lib, "lib/DirectXLayer_2013.lib")

// This object is your interface to the DirectX abstraction layer.
DirectXLayer DXLayer;

HINSTANCE g_hInst;
HWND g_hWnd;
HWND g_Dlg;
HWND g_FPS;
HMENU g_dlgMenu;
RECT g_displayPanel, g_oldDisplaySize;
const unsigned int g_displayPanelLeft = 275;
const unsigned int g_displayPanelTop = 0;
HACCEL hacc;
int g_width = 1280;
int g_height = 720;
float proj = 70.0f;
float g_cameraHeight = 30.0f;
float g_cameraZ = 15.0f;
enum File_Extension { FILE_EXT_MESH, FILE_EXT_SCN };

bool loaded = false;
string mainApplicationDirectory;


INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeComponents(void);
void ResizeDisplayPanel(void);
void ResizeDevice(void);
void Update(void);
void Render(void);
void LoadMeshFromFile(const char * path);
string GetFilePath(File_Extension ext = FILE_EXT_MESH);

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	UNREFERENCED_PARAMETER( hPrevInstance );
	UNREFERENCED_PARAMETER( lpCmdLine );
	g_hInst = hInstance;
	/*========= Application Initialization =========*/
	g_dlgMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
	g_Dlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, DialogProc, 0);
	g_FPS = GetDlgItem(g_Dlg, IDC_STATIC_FPS);
	
	InitializeComponents();
	hacc = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	ShowWindow(g_Dlg, nCmdShow);
	
	// Set program path.
	HMODULE hModule = GetModuleHandle(NULL);
	char path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	mainApplicationDirectory = path;
	unsigned int pos = mainApplicationDirectory.find_last_of('\\');
	mainApplicationDirectory.erase(pos);
	SetCurrentDirectory(mainApplicationDirectory.c_str());
	
	HWND renderwindow = GetDlgItem(g_Dlg, IDC_RENDER_WINDOW);
	
	if (!DXLayer.Initialize(renderwindow, g_width, g_height))
	{
		DXLayer.Cleanup();
		return 0;
	}

	// Main message loop
	MSG msg = {0};
	while( WM_QUIT != msg.message )
	{
		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if (!TranslateAccelerator(g_Dlg, hacc, &msg) && !IsDialogMessage(g_Dlg, &msg))
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		else
		{
			Update();
			Render();
		}
	}

	DXLayer.Cleanup();
	return ( int )msg.wParam;
}



void LoadMeshFromFile(const char *path)
{
	// TODO: Load mesh data and send it to the graphics card.
}

// Open a file dialog box and return the selected path.
string GetFilePath(File_Extension ext)
{
	string extension;
	char buffer[256] = "\0";
	OPENFILENAME opFile;
	if (ext == FILE_EXT_MESH)
		extension = "Mesh Files (*.mesh)#*.mesh#";
	else if (ext == FILE_EXT_SCN)
		extension = "Scene Files (*.scn)#*.scn#";
	char szFilters[64];		// buffer of file filters
	memset( szFilters, 0, sizeof( szFilters ) );
	memcpy( szFilters, extension.c_str(), extension.length() );
	for ( int i = 0; i < sizeof(szFilters); i++ ) szFilters[ i ] = ( ( szFilters[ i ] == '#' ) ? '\0' : szFilters[ i ] );

	memcpy(buffer, mainApplicationDirectory.c_str(), mainApplicationDirectory.length());
	ZeroMemory( &opFile, sizeof(opFile) );
	opFile.lStructSize		= sizeof(opFile);
	opFile.hwndOwner		= g_Dlg;
	opFile.lpstrFile		= buffer;
	opFile.lpstrFile[0]	= '\0'; // because szFile is crap.
	opFile.nMaxFile		= sizeof(buffer);
	opFile.lpstrFilter		= szFilters;
	opFile.nFilterIndex	= 0;
	opFile.lpstrFileTitle	= NULL;
	opFile.nMaxFileTitle	= 0;
	opFile.lpstrInitialDir	= mainApplicationDirectory.c_str();
	opFile.Flags			= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;

	SetCurrentDirectory(mainApplicationDirectory.c_str());
	string path;
	if (GetOpenFileName(&opFile) == TRUE)
		path = opFile.lpstrFile;
	else
		path = "";

	return path;
}

/*=================== Message Handler ========================*/
INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId = 0;
	
	string path;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			HICON bigIcon = NULL;
			bigIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)bigIcon);
			loaded = true;
			break;
		}
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch(wmId)
		{
		case ID_ACCELERATOR_OPEN:
		case ID_FILE_OPEN_MENU:
			// TODO: Open up a .scn file and load meshes from it.
			MessageBox(NULL, "Load a Scene file and render everything!", "Task!", MB_OK);
			break;
		case ID_FILE_EXIT:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;
		case IDC_BUTTON1:
			{
				MessageBox(NULL, "Load the bench and draw it on the left", "Task!", MB_OK);
				
				break;
			}
		case IDC_BUTTON2:
			{
				MessageBox(NULL, "Load Major N and draw him in the middle", "Task!", MB_OK);
				
				break;
			}
		case IDC_BUTTON3:
			{
				MessageBox(NULL, "Load the model with the hierarchy, and draw it on the right", "Task!", MB_OK);
				break;
			}
		}

	case WM_SIZE:
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			// Only resize if necessary... mostly to keep from breaking DirectX debugging.
			if (width != 0 && height != 0)
				ResizeDisplayPanel();

			break;
		}
	case WM_CLOSE:
			DestroyWindow(hDlg);
		
		return TRUE;
	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpInfo = (LPMINMAXINFO)lParam;
			if(lpInfo)
				lpInfo->ptMinTrackSize.x = 1024, lpInfo->ptMinTrackSize.y = 768;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;
	}

	return FALSE;
}

void InitializeComponents(void)
{
	g_oldDisplaySize.bottom = 0;
	g_oldDisplaySize.top = 0;
	g_oldDisplaySize.left = 0;
	g_oldDisplaySize.right = 0;
	ResizeDisplayPanel();
}

void ResizeDisplayPanel(void)
{
	RECT mainwindow;
	GetWindowRect(g_Dlg, &mainwindow);
	g_displayPanel.top = mainwindow.top;
	g_displayPanel.left = mainwindow.left + 275;
	g_displayPanel.bottom = mainwindow.bottom;// - 75;
	g_displayPanel.right = mainwindow.right;// - 25;

	g_width = g_displayPanel.right - g_displayPanel.left - 25;
	g_height = g_displayPanel.bottom - g_displayPanel.top - 75;

	HWND pict = GetDlgItem(g_Dlg, IDC_RENDER_WINDOW);
	SetWindowPos(pict, HWND_TOP, g_displayPanelLeft, g_displayPanelTop, g_width, g_height, SWP_SHOWWINDOW);
	InvalidateRect(g_Dlg, NULL, true);

	DXLayer.ResizeDevice(g_width, g_height);

	HWND div = GetDlgItem(g_Dlg, IDC_TOOL_DIVIDER);
	RECT rdiv;
	GetWindowRect(div, &rdiv);
	SetWindowPos(div, HWND_TOP, 265, 0, 10, mainwindow.bottom - mainwindow.top, SWP_SHOWWINDOW);
	DXLayer.CreateProjectionMatrix(proj);
}

void Update(void)
{

	// BASIC camera controls.
	if (GetAsyncKeyState(VK_UP))
			g_cameraHeight += 0.01f;
	if (GetAsyncKeyState(VK_DOWN))
			g_cameraHeight -= 0.01f;
	if (GetAsyncKeyState(VK_RIGHT))
			g_cameraZ += 0.01f;
	if (GetAsyncKeyState(VK_LEFT))
			g_cameraZ -= 0.01f;
	DXLayer.CreateCameraMatrix(XMVectorSet(0.0f, g_cameraHeight, g_cameraZ, 0.0f), XMVectorSet(0.0f, 5.0f, 0.0f, 0.0f));
}

void Render()
{
	// TODO: Draw stuff here.
}