
#include "inc/DirectXLayer.h"
#include "resource.h"
#include "mmsystem.h"
#pragma comment(lib, "WinMM.lib")
#include <string>

#include <fstream>
#include <map>
using namespace std;

#pragma comment(lib, "lib/DirectXLayer_2013.lib")

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

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

struct Mesh {
	string m_meshName;
	vector<VertexPositionNormalTexture> m_vertex;
	vector<unsigned int> m_index;
	vector<string> m_texture;
	Mesh& operator= (const Mesh& other) {
		if (this != &other) {
			m_meshName = other.m_meshName;
			m_vertex = other.m_vertex;
			m_index = other.m_index;
			m_texture = other.m_texture;
		}
		return *this;
	}
};

struct Scene {
	vector<string> m_meshNames;
	map<string, Mesh> m_meshes;
	vector<XMMATRIX> m_worldMatrix;
	vector<BufferInfo> m_buffInfos;
	void clear() {
		m_meshNames.clear();
		m_meshes.clear();
		m_worldMatrix.clear();
		m_buffInfos.clear();
	}
	void FillInTheBuffer() {
		if (m_meshes.empty()) return;

		m_buffInfos.resize(m_meshNames.size());
		for (unsigned int i = 0; i < m_meshNames.size(); i++) {
			DXLayer.LoadVertexAndIndexData_PosNormalTexture(
				&m_meshes[m_meshNames[i]].m_vertex[0],
				m_meshes[m_meshNames[i]].m_vertex.size(),
				&m_meshes[m_meshNames[i]].m_index[0],
				m_meshes[m_meshNames[i]].m_index.size(),
				&m_buffInfos[i]
			);
		}
	}
};

Scene aScene;

bool loaded = false;
string mainApplicationDirectory;


INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeComponents(void);
void ResizeDisplayPanel(void);
void ResizeDevice(void);
void Update(void);
void Render(void);
Mesh LoadMeshFromFile(const char * path);
void LoadSceneFromFile(const char *path);

string GetFilePath(File_Extension ext = FILE_EXT_MESH);


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

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

void LoadSceneFromFile(const char *path) {
	if (strlen(path) < 1) return;
	ifstream fin;
	streampos file_size;

	fin.open(path, ios_base::binary | ios_base::in);
	if (fin.is_open()) {
		file_size = fin.tellg();
		unsigned int numberOfObjects;
		fin.read((char*)&numberOfObjects, sizeof(numberOfObjects));

		//aScene.m_meshNum = numberOfObjects;
		//aScene.m_meshNames.resize(numberOfObjects);

		for (unsigned int i = 0; i < numberOfObjects; i++) {
			unsigned int fileNameLength;
			fin.read((char*)&fileNameLength, sizeof(fileNameLength));
			char* fn = new char[fileNameLength];
			fin.read(fn, fileNameLength*sizeof(char));
			string fileName = fn;	// fileName = meshName
			delete[] fn;
			XMFLOAT4X4 matrix4x4;
			fin.read((char*)&matrix4x4, sizeof(matrix4x4));
			XMMATRIX worldMatrix;		
			worldMatrix = XMLoadFloat4x4(&matrix4x4);	// mesh poistion

			Mesh tmpMesh;
			string meshDirectory = mainApplicationDirectory;
			meshDirectory += "\\mesh";
			SetCurrentDirectory(meshDirectory.c_str());
			tmpMesh = LoadMeshFromFile(fileName.c_str());			// loading a mesh!!
			//aScene.m_meshNames[i] = tmpMesh.m_meshName;
			aScene.m_meshNames.push_back(tmpMesh.m_meshName);
			map<string, Mesh>::iterator meshIt = aScene.m_meshes.find(tmpMesh.m_meshName);
			if (meshIt == aScene.m_meshes.end()) {
				aScene.m_meshes.insert(pair<string, Mesh>(tmpMesh.m_meshName, tmpMesh));
			}
			aScene.m_worldMatrix.push_back(worldMatrix);
		}
		aScene.FillInTheBuffer();
		fin.close();
	}
}


Mesh LoadMeshFromFile(const char *path)
{
	Mesh aMesh;
	// TODO: Load mesh data and send it to the graphics card.
	if (strlen(path) < 1) return aMesh;


	ifstream fin;
	streampos file_size;

	fin.open(path, ios_base::binary | ios_base::in);

	if (fin.is_open()) {
		file_size = fin.tellg();

		unsigned int meshNameLength;
		fin.read((char*)&meshNameLength, sizeof(meshNameLength));
		//fin.read(meshName, meshNameLength*sizeof(char));
		char* meshName = new char[meshNameLength];
		fin.read(meshName, meshNameLength*sizeof(char));

		aMesh.m_meshName = meshName;
		delete[] meshName;
		unsigned int textureCounts;
		fin.read((char*)&textureCounts, sizeof(textureCounts));

		for (unsigned int i = 0; i < textureCounts; i++) {
			unsigned int textureNameLength;
			fin.read((char*)&textureNameLength, sizeof(textureNameLength));
			char* texName = new char[textureNameLength];
			fin.read(texName, textureNameLength*sizeof(char));
			string pendingName = texName;
			delete[] texName;
			std::size_t found = pendingName.find_last_of("/\\");

			aMesh.m_texture.push_back(pendingName.substr(found + 1));
			DXLayer.LoadTexture(pendingName.substr(found + 1).c_str());
		}

		unsigned int uniqueVertCounts;
		fin.read((char*)&uniqueVertCounts, sizeof(uniqueVertCounts));

		for (unsigned int i = 0; i < uniqueVertCounts; i++) {
			VertexPositionNormalTexture vpnt;
			// position
			fin.read((char*)&vpnt.position.x, sizeof(float));
			fin.read((char*)&vpnt.position.y, sizeof(float));
			fin.read((char*)&vpnt.position.z, sizeof(float));
			// normals
			fin.read((char*)&vpnt.normal.x, sizeof(float));
			fin.read((char*)&vpnt.normal.y, sizeof(float));
			fin.read((char*)&vpnt.normal.z, sizeof(float));
			// uvs
			fin.read((char*)&vpnt.textureCoordinate.x, sizeof(float));
			fin.read((char*)&vpnt.textureCoordinate.y, sizeof(float));
			aMesh.m_vertex.push_back(vpnt);
		}

		unsigned int triangleCounts;
		fin.read((char*)&triangleCounts, sizeof(triangleCounts));

		for (unsigned int i = 0; i < triangleCounts; i++) {
			//indices in a triangle
			unsigned int a, b, c;
			fin.read((char*)&a, sizeof(unsigned int));
			fin.read((char*)&b, sizeof(unsigned int));
			fin.read((char*)&c, sizeof(unsigned int));

			aMesh.m_index.push_back(a);
			aMesh.m_index.push_back(b);
			aMesh.m_index.push_back(c);
		}

		fin.close();
	}
	return aMesh;

	//DXLayer.CreateCameraMatrix(XMVectorSet(5.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f));
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
			//MessageBox(NULL, "Load a Scene file and render everything!", "Task!", MB_OK);
			aScene.clear();
			LoadSceneFromFile(GetFilePath(FILE_EXT_SCN).c_str());
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
			g_cameraHeight += 0.05f;
	if (GetAsyncKeyState(VK_DOWN))
			g_cameraHeight -= 0.05f;
	if (GetAsyncKeyState(VK_RIGHT))
			g_cameraZ += 0.05f;
	if (GetAsyncKeyState(VK_LEFT))
			g_cameraZ -= 0.05f;
	DXLayer.CreateCameraMatrix(XMVectorSet(0.0f, g_cameraHeight, g_cameraZ, 0.0f), XMVectorSet(0.0f, 5.0f, 0.0f, 0.0f));
}

void Render()
{
	// TODO: Draw stuff here.
	const float clearColor[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
	DXLayer.Clear(clearColor);

	for (size_t totalMeshes = 0; totalMeshes < aScene.m_meshNames.size(); totalMeshes++) {
		DXLayer.SetWorldMatrix(&aScene.m_worldMatrix[totalMeshes]);
		for (size_t totalTextures = 0; totalTextures < aScene.m_meshes[aScene.m_meshNames[totalMeshes]].m_texture.size(); totalTextures++) {
			string texture = aScene.m_meshes[aScene.m_meshNames[totalMeshes]].m_texture[totalTextures];
			DXLayer.SetTexture(texture.c_str());
		}
		DXLayer.DrawMesh(&aScene.m_buffInfos[totalMeshes]);
	}

	DXLayer.Present();

}