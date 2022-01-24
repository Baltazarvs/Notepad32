#include <Windows.h>
#include <CommCtrl.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <algorithm>
#include "resource.h"

#pragma warning (disable : 4996)
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define LP_CLASS_NAME		L"Ilijatech - Notepad32"

typedef std::tuple<bool, bool, bool, std::wstring, int> SETTINGS_TUPLE;

// ======================== FUNCTIONS ==================================

void InitUI(HWND, HINSTANCE);
std::wstring OpenFileWithDialog(const wchar_t*, HWND);
bool WriteReadFileToTextArea(const wchar_t*);
SETTINGS_TUPLE LoadNotepad32Settings();
template<int i, typename T> T CheckSettingsValueAndSet(std::wstring value);
bool ApplyNotepad32Settings(SETTINGS_TUPLE& settings_tuple);

LRESULT __stdcall DlgProc_Settings(HWND, UINT, WPARAM, LPARAM);
// ======================== VARIABLES ==================================

static SETTINGS_TUPLE CurrentSettingsTuple;
static wchar_t Runtime_CurrentPath[MAX_PATH];

// ========================= HANDLES ===================================

static HWND w_TextArea = nullptr;
static HWND w_StatusBar = nullptr;

// =====================================================================

LRESULT __stdcall WndProc(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_CREATE:
		{
			InitUI(w_Handle, GetModuleHandle(nullptr));
			break;
		}
		case WM_COMMAND:
		{

			switch (wParam)
			{
				case ID_FILE_OPEN:
				{
					std::wstring path(OpenFileWithDialog(L"Text File\0*.txt\0", w_Handle));
					WriteReadFileToTextArea(path.c_str());
					break;
				}
				case ID_EDIT_SETTINGS:
				{
					DialogBox(
						GetModuleHandle(nullptr),
						MAKEINTRESOURCE(IDD_SETTINGS),
						w_Handle,
						reinterpret_cast<DLGPROC>(&DlgProc_Settings)
					);
					break;
				}
			}

			break;
		}
		case WM_SIZE:
		{
			RECT wRect = { };
			GetClientRect(w_Handle, &wRect);

			RECT wTextAreaRect = { };
			int textAreaHeight = 0;
			GetWindowRect(w_TextArea, &wTextAreaRect);

			RECT wStatusRect = { };
			int statusHeight = 0;
			GetWindowRect(w_StatusBar, &wStatusRect);
			SendMessageA(w_StatusBar, WM_SIZE, 0u, 0u);

			statusHeight = wStatusRect.bottom - wStatusRect.top;
			textAreaHeight = wTextAreaRect.bottom - statusHeight;
			MoveWindow(w_TextArea, 0, 0, wRect.right, wRect.bottom - statusHeight, TRUE);
			break;
		}
		case WM_CLOSE:
			DestroyWindow(w_Handle);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(w_Handle, Msg, wParam, lParam);
	}
	return 0;
}

LRESULT __stdcall DlgProc_Settings(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND w_CheckUnderline = GetDlgItem(w_Dlg, IDC_SETTINGS_CHECK_UNDERLINE);
	HWND w_CheckItalic = GetDlgItem(w_Dlg, IDC_SETTINGS_CHECK_ITALIC);
	HWND w_CheckBold = GetDlgItem(w_Dlg, IDC_SETTINGS_CHECK_BOLD);
	HWND w_Font = GetDlgItem(w_Dlg, IDC_SETTINGS_EDIT_FONT);
	HWND w_DefaultFonts = GetDlgItem(w_Dlg, IDC_SETTINGS_BUTTON_PDFONTS);
	HWND w_FontSize = GetDlgItem(w_Dlg, IDC_SETTINGS_EDIT_FONTSIZE);

	SETTINGS_TUPLE currentSetsTuple = LoadNotepad32Settings();
	bool bItalic, bUnderline, bBold;
	wchar_t font[255], font_size[255];
	int fontSize = 0;

	HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_MENU));

	switch (Msg)
	{
		case WM_INITDIALOG:
		{
			bUnderline = std::get<0>(currentSetsTuple);
			bBold = std::get<1>(currentSetsTuple);
			bItalic = std::get<2>(currentSetsTuple);
			wcscpy(font, std::get<3>(currentSetsTuple).c_str());
			fontSize = std::get<4>(currentSetsTuple);

			if(bUnderline)
				SendMessageA(w_CheckUnderline, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0u);
			else
				SendMessageA(w_CheckUnderline, BM_SETCHECK, static_cast<WPARAM>(BST_UNCHECKED), 0u);

			if(bBold)
				SendMessageA(w_CheckBold, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0u);
			else
				SendMessageA(w_CheckBold, BM_SETCHECK, static_cast<WPARAM>(BST_UNCHECKED), 0u);

			if (bItalic)
				SendMessageA(w_CheckItalic, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0u);
			else
				SendMessageA(w_CheckItalic, BM_SETCHECK, static_cast<WPARAM>(BST_UNCHECKED), 0u);
			SendMessage(w_Font, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(std::get<3>(currentSetsTuple).c_str()));

			std::wostringstream ss;
			ss << std::get<4>(currentSetsTuple);
			SendMessage(w_FontSize, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(ss.str().c_str()));
			break;
		}
		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDCANCEL:
					EndDialog(w_Dlg, IDCANCEL);
					break;
				case ID_SETTINGS_BUTTON_APPLY:
				{
					// Get font text and font size text.
					GetWindowText(w_Font, font, 255);
					GetWindowText(w_FontSize, font_size, 255);

					// Get check states of editing style's check buttons
					bUnderline = IsDlgButtonChecked(w_CheckUnderline, BST_CHECKED) ? true : false;
					bBold = IsDlgButtonChecked(w_CheckBold, BST_CHECKED) ? true : false;
					bItalic = IsDlgButtonChecked(w_CheckItalic, BST_CHECKED) ? true : false;

					// Convert font size text string into integer
					std::wstringstream ss_sz(font_size);
					ss_sz >> fontSize;

					// Make new SETTINGS_TUPLE with these new informations.
					currentSetsTuple = std::make_tuple(
						bUnderline, 
						bBold, 
						bItalic, 
						font, 
						fontSize
					);

					// Apply new settings.
					ApplyNotepad32Settings(currentSetsTuple);
					break;
				}
				case IDC_SETTINGS_BUTTON_PDFONTS:
					// In Progress...
					break;
			}
			break;
		}
		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			if ((HWND)lParam == GetDlgItem(w_Dlg, IDC_STATIC_CHANGES_TAKES_EFFECT))
			{
				SetBkColor(hdc, RGB(0xFF, 0xFF, 0xFF));
				SetTextColor(hdc, RGB(0xFF, 0x00, 0x00));
				return (LONG_PTR)hbr;
			}
			break;
		}
		case WM_CLOSE:
			EndDialog(w_Dlg, 0);
			break;
	}
	return 0;
}

int __stdcall WinMain(HINSTANCE w_Inst, HINSTANCE w_PrevInst, char* lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wcex;

	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = 0;
	wcex.lpfnWndProc = &WndProc;
	wcex.lpszClassName = LP_CLASS_NAME;
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENUBAR);
	wcex.hInstance = GetModuleHandle(nullptr);
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
	
	if (!RegisterClassEx(&wcex))
	{
		MessageBoxA(nullptr, "Cannot register class!", "Error!", MB_OK | MB_ICONERROR);
		return -1;
	}

	HWND w_Handle = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		LP_CLASS_NAME,
		L"Notepad32 v1.0",
		WS_OVERLAPPEDWINDOW,
		100, 100, 650, 500,
		nullptr, nullptr, nullptr, nullptr
	);

	if (w_Handle == nullptr)
	{
		MessageBoxA(nullptr, "Cannot create a window!", "Error!", MB_OK | MB_ICONERROR);
		return -1;
	}

	ShowWindow(w_Handle, SW_SHOWDEFAULT);
	UpdateWindow(w_Handle);

	MSG Msg = { };
	while (GetMessage(&Msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return 0;
}

void InitUI(HWND w_Handle, HINSTANCE w_Inst)
{
	DWORD defStyle = (WS_VISIBLE | WS_CHILD);
	::CurrentSettingsTuple = LoadNotepad32Settings();

	w_TextArea = CreateWindow(
		WC_EDIT, nullptr,
		defStyle | WS_BORDER | WS_HSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
		0, 0, 300, 300,
		w_Handle, nullptr, w_Inst, nullptr
	);


	int sbWidths[] = { 250, 250, 100 };
	SendMessageA(w_StatusBar, SB_SETPARTS, (sizeof(sbWidths) / sizeof(int)), reinterpret_cast<LPARAM>(sbWidths));
	w_StatusBar = CreateWindow(
		STATUSCLASSNAME, nullptr,
		defStyle | SBS_SIZEGRIP,
		0, 0, 0, 0,
		w_Handle, nullptr, w_Inst, nullptr
	);

	SendMessage(w_StatusBar, SB_SETTEXTW, 0u, reinterpret_cast<LPARAM>(L"No file loaded."));

	const std::size_t control_num = 1;
	HWND w_Controls[control_num] = { w_TextArea };

	for (std::size_t i = 0; i < control_num; ++i)
	{
		if (w_Controls[i] == w_TextArea)
		{
			HFONT hFont = CreateFont(
				std::get<4>(::CurrentSettingsTuple), 
				0, 0, 0, 0, 
				(DWORD)std::get<2>(::CurrentSettingsTuple),
				(DWORD)std::get<0>(::CurrentSettingsTuple), 
				0, 0, 0, 0, 0, 0, (LPCWSTR)std::get<3>(::CurrentSettingsTuple).c_str()
			);

			SendMessage(w_Controls[i], WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 1u);
			continue;
		}
		SendMessage(w_Controls[i], WM_SETFONT, reinterpret_cast<WPARAM>((HFONT)GetStockObject(DEFAULT_GUI_FONT)), 1u);
	}
}

std::wstring OpenFileWithDialog(const wchar_t* Filters, HWND w_Handle)
{
	OPENFILENAME ofn = { };
	wchar_t* _Path = new wchar_t[MAX_PATH];

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = w_Handle;
	ofn.lpstrFilter = Filters;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = _Path;
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrTitle = L"Open Project";
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = (OFN_EXPLORER | OFN_PATHMUSTEXIST);

	GetOpenFileName(&ofn);
	std::wstring __Path(_Path);
	delete[] _Path;
	return __Path;
}

bool WriteReadFileToTextArea(const wchar_t* Path)
{
	SendMessage(w_StatusBar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(Path));
	wcscpy(Runtime_CurrentPath, Path);

	if (Path != nullptr)
	{
		std::wfstream file;
		file.open(Path, std::ios::in | std::ios::out);
		if (file.is_open())
		{
			std::wstring textBuffer;
			std::wstring line;
			while (std::getline(file, line))
			{
				// Needs fix for last new line append. After last line, there shouldn't be new line if that's not in file.
				textBuffer.append(line.c_str());
				textBuffer.append(L"\r\n");
			}
			SendMessage(w_TextArea, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(textBuffer.c_str()));
			file.close();
		}
		else
			return false;
	}
	return true;
}

SETTINGS_TUPLE LoadNotepad32Settings()
{
	SETTINGS_TUPLE tTemplate;
	bool bItalic, bUnderline, bBold;
	std::wstring font;
	int fontSize = 0;

	std::wfstream file;
	file.open(L"Source\\Settings\\Main.txt", std::ios::in | std::ios::out);
	if (file.is_open())
	{
		std::wstring line;
		std::wstring settingsListStr;
		std::size_t index = 0u;

		while (std::getline(file, line))
		{
			switch (index)
			{
				case 0:
					bUnderline = CheckSettingsValueAndSet<0, bool>(line);
					break;
				case 1:
					bBold = CheckSettingsValueAndSet<0, bool>(line);
					break;
				case 2:
					bItalic = CheckSettingsValueAndSet<0, bool>(line);
					break;
				case 3:
					font = line;
					break;
				case 4:
				{
					// Used for checking if there is wrong font size (has character, ...)
					bool bErr = false;
					// Iterate through current line (font size setting) and check if there are only digits.
					for (std::wstring::iterator itr = line.begin(); itr != line.end(); ++itr)
					{
						// Check if there are just digits.
						if (!std::isdigit(*itr))
						{
							bErr = true;
							break;
						}
						else
							continue;
					}
					if (bErr)
					{
						// If there was an error, set default font size.
						fontSize = 0;
						break;
					}
					else
					{
						// Used for conversion from string to int (loading from file is string product)
						int size_temp = 0;
						std::wstringstream ss(line);
						ss >> size_temp;
						// Set converted size to font size variable.
						fontSize = size_temp;
					}
					break;
				}
			}
			++index;
		}
	}
	else
		return std::make_tuple(false, false, false, L"Arial", 0);
	return std::make_tuple(bUnderline, bBold, bItalic, font.c_str(), fontSize);
}

bool ApplyNotepad32Settings(SETTINGS_TUPLE& settings_tuple)
{
	std::wostringstream ss_temp;
	std::wfstream file;
	file.open(L"Source\\Settings\\Main.txt", std::ios::in | std::ios::out | std::ios::trunc);
	if (file.is_open())
	{
		ss_temp << std::get<0>(settings_tuple) << std::endl;
		ss_temp << std::get<1>(settings_tuple) << std::endl;
		ss_temp << std::get<2>(settings_tuple) << std::endl;
		ss_temp << std::get<3>(settings_tuple).c_str() << std::endl;
		ss_temp << std::get<4>(settings_tuple); // << std::endl; // Last line must be excepted if there is no more settings lines!
		file << ss_temp.str().c_str();
		file.close();
	}
	else
		return false;
	return true;
}

template<int i, typename T>
T CheckSettingsValueAndSet(std::wstring value)
{
	/*
	For i:
	
	0 - bool check
	1 - string check
	*/
	switch (i)
	{
		case 0:
		{
			if (value == L"true")
				return true;
			else
				return false;
		}
	}
}
