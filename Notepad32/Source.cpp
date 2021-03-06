// 2021 - 2022 Baltazarus

#define _WIN32_WINNT 0x501
#define _WIN32_IE 0x0300

#include <Windows.h>
#include <CommCtrl.h>
#include <windowsx.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <algorithm>
#include "resource.h"

#ifdef _MSC_VER
	#pragma warning (disable : 4996)
	#pragma comment(lib, "Comctl32.lib")
	#pragma comment(linker,"\"/manifestdependency:type='win32' \
	name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
	processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define LP_CLASS_NAME		L"Baltazarus - Notepad32"
#define PATH_MAIN_SETTINGS  L"Source\\Settings\\Main.settings32"
#define PATH_FONT_SETTINGS  L"Source\\Settings\\Font.settings32"
#define PATH_RECENT_FILES	L"Source\\Settings\\Recent.settings32"
#define PATH_PATCH_NOTES	L"Source\\Info\\Patch_Notes.txt"
#define PATH_HOW_TO_USE		L"Source\\Info\\How_To_Use.txt"

#define IDC_EDIT_TEXTAREA	30001

#define CRITERIA_FILE_OPEN		1
#define CRITERIA_FILE_SAVE		2
#define CRITERIA_FILE_SAVE_AS	3

typedef std::tuple<
	bool, bool, bool, 
	std::wstring, int, 
	int, int, int,
	int, int, int,
	bool
> SETTINGS_TUPLE;
				// und,   bold, it,     font,      sz,   R,   G,   B, dark_theme
// ======================== FUNCTIONS ==================================

void InitUI(HWND, HINSTANCE);
std::wstring OpenFileWithDialog(const wchar_t*, HWND, int);
bool WriteReadFileToTextArea(const wchar_t*);
bool SaveTextProcedure(HWND, int);
bool SaveTextToFile(const wchar_t*);
SETTINGS_TUPLE LoadNotepad32Settings();
bool CheckSettingsValueAndSet(std::wstring);
std::wstring CheckSettingsValueAndApply(bool);
bool ApplyNotepad32Settings(SETTINGS_TUPLE& settings_tuple);
void UpdateStatusForLengthLine(int, int);
int KeydownCombo(int key);
void CopyTextToClipboard();
std::wstring GetTextFromClipboard();
std::wstring GetSelectedText(std::wstring);
bool CheckColorSetting(std::wstring line);
std::wstring SubstringSelectedText(const wchar_t*, unsigned long, unsigned long);
void RemoveSelectedText(HWND, bool, std::wstring = std::wstring());
bool CheckFileChanges(const wchar_t* ta_text);
int CountAllCases(std::wstring ta_text, const wchar_t* wfind, int find_length);
void SelectText(int from, int to);
std::size_t FindCaseSensitive(std::wstring, const wchar_t*, std::size_t);
void FindAllCaseSensitiveOccurrences(std::wstring, const wchar_t*, std::vector<std::size_t>&);
COLORREF GetColorFromDialog(HWND w_Handle, HINSTANCE w_Inst);
template <typename T> std::wstring ConvertToString(T var);
void PushRecentFilename(const wchar_t* filename);
void PushRecentMenuItems(HWND w_Handle);
void ClearRecentFilenames();
std::vector<std::wstring> PullRecentFilenames();
bool CheckRecentFilenameExistence(const wchar_t* filename);
std::wstring LoadInfoFromFile(const wchar_t* path);

// ======================= PROCEDURES ==================================
LRESULT __stdcall WndProc(HWND, UINT, WPARAM, LPARAM);

LRESULT __stdcall DlgProc_Settings(HWND, UINT, WPARAM, LPARAM);
LRESULT __stdcall DlgProc_DefaultFonts(HWND, UINT, WPARAM, LPARAM);
LRESULT __stdcall DlgProc_About(HWND, UINT, WPARAM, LPARAM);
LRESULT __stdcall DlgProc_Help(HWND, UINT, WPARAM, LPARAM);
LRESULT __stdcall DlgProc_Find(HWND, UINT, WPARAM, LPARAM);

LRESULT __stdcall WndProc_StatusBarInfo(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

// ======================== VARIABLES ==================================

static SETTINGS_TUPLE CurrentSettingsTuple;
static wchar_t Runtime_CurrentPath[MAX_PATH];
static bool Runtime_CurrentPathOpened = false;
static bool Runtime_bHelpPatchNotes = false;
static bool Runtime_bDarkThemeEnabled = false;
static std::size_t Runtime_FindIndex = 0ull;
wchar_t* Runtime_DefaultSelectedFontFromDialog = nullptr;
static COLORREF Runtime_customColors[16];
static COLORREF Runtime_rgbCurrent = RGB(0xFF, 0xFF, 0xFF);
static bool Runtime_bFindDialogOpened = false;

// ========================= HANDLES ===================================

static HWND w_TextArea = nullptr;
static HWND w_StatusBar = nullptr;

// =====================================================================

LRESULT __stdcall WndProc(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hbr = nullptr;
	if (!hbr)
	{
		if (::Runtime_bDarkThemeEnabled)
			hbr = CreateSolidBrush(RGB(0x55, 0x55, 0x55));
		else
			hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	}

	switch (Msg)
	{
		case WM_CREATE:
		{
			InitUI(w_Handle, GetModuleHandle(nullptr));
			SetWindowSubclass(w_StatusBar, (SUBCLASSPROC)&WndProc_StatusBarInfo, 0u, 0u);

			if (wcslen(::Runtime_CurrentPath) > 1)
				WriteReadFileToTextArea(::Runtime_CurrentPath);
			UpdateStatusForLengthLine(Edit_GetTextLength(w_TextArea), Edit_GetLineCount(w_TextArea));
			PushRecentMenuItems(w_Handle);
			break;
		}
		case WM_CTLCOLOREDIT:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			if ((HWND)lParam == w_TextArea)
			{
				if (::Runtime_bDarkThemeEnabled)
				{
					// Set dark theme background color to #555555
					std::get<5>(::CurrentSettingsTuple) = 85;
					std::get<6>(::CurrentSettingsTuple) = 85;
					std::get<7>(::CurrentSettingsTuple) = 85;
				}

				SetBkColor(
					hdc,
					RGB(
						std::get<5>(::CurrentSettingsTuple),
						std::get<6>(::CurrentSettingsTuple),
						std::get<7>(::CurrentSettingsTuple)
					)
				);
				SetTextColor(
					hdc, 
					RGB(
						std::get<8>(::CurrentSettingsTuple),
						std::get<9>(::CurrentSettingsTuple),
						std::get<10>(::CurrentSettingsTuple)
					)
				);
				return (LONG_PTR)hbr;
			}
			break;
		}
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				int text_length = SendMessageW(w_TextArea, WM_GETTEXTLENGTH, 0u, 0u);
				int text_lines = SendMessageW(w_TextArea, EM_GETLINECOUNT, 0u, 0u);
				UpdateStatusForLengthLine(text_length, text_lines);
			}
			switch (wParam)
			{
				case ID_FILE_NEW:
				{
					UpdateStatusForLengthLine(0, 1);
					::Runtime_CurrentPathOpened = false;
					memset(::Runtime_CurrentPath, 0, sizeof(::Runtime_CurrentPath));
					SendMessageW(w_TextArea, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(L""));
					SendMessageW(w_StatusBar, SB_SETTEXTW, 2u, reinterpret_cast<LPARAM>(L"No file loaded."));
					break;
				}
				case ID_FILE_OPEN:
				{
					std::wstring path(OpenFileWithDialog(L"Text File\0*.txt\0All Files\0*.*\0", w_Handle, CRITERIA_FILE_OPEN));
					if (path == L"\0")
						return 1;
					WriteReadFileToTextArea(path.c_str());
					// We opened file for editing, so let's set variable.
					::Runtime_CurrentPathOpened = true;
					// Copy opened path to current buffer used for opened file path.
					wcscpy(::Runtime_CurrentPath, path.c_str());
					PushRecentFilename(path.c_str());
					break;
				}
				case ID_FILE_SAVE:
				{
					// Call the function that does all saving job for us.
					SaveTextProcedure(w_Handle, CRITERIA_FILE_SAVE);
					break;
				}
				case ID_FILE_SAVEAS:
				{
					SaveTextProcedure(w_Handle, CRITERIA_FILE_SAVE_AS);
					break;
				}
				case ID_FILE_EXIT:
					DestroyWindow(w_Handle);
					break;
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
				case ID_EDIT_CUT:
				{
					CopyTextToClipboard();
					::RemoveSelectedText(w_TextArea, false);
					break;
				}
				case ID_EDIT_COPY:
				{
					CopyTextToClipboard();
					break;
				}
				case ID_EDIT_PASTE:
				{
					std::wstring clipboard_text = GetTextFromClipboard();
					RemoveSelectedText(w_TextArea, true, clipboard_text);
					UpdateStatusForLengthLine(Edit_GetTextLength(w_TextArea), Edit_GetLineCount(w_TextArea));
					break;
				}
				case ID_EDIT_FIND:
				{
					if (!::Runtime_bFindDialogOpened)
					{
						Runtime_bFindDialogOpened = true;
						DialogBoxW(
							GetModuleHandleW(nullptr),
							MAKEINTRESOURCEW(IDD_FIND),
							nullptr,
							reinterpret_cast<DLGPROC>(&DlgProc_Find)
						);
					}
					break;
				}
				case ID_HELP_ABOUT:
				{
					DialogBox(
						GetModuleHandle(nullptr),
						MAKEINTRESOURCE(IDD_ABOUT),
						w_Handle,
						reinterpret_cast<DLGPROC>(&DlgProc_About)
					);

					break;
				}
				case ID_HELP_PATCHNOTES:
				{
					::Runtime_bHelpPatchNotes = true;
					DialogBoxW(
						GetModuleHandleW(nullptr),
						MAKEINTRESOURCEW(IDD_HELP),
						w_Handle,
						reinterpret_cast<DLGPROC>(&DlgProc_Help)
					);
					break;
				}
				case ID_HELP_HOWTOUSE:
				{
					::Runtime_bHelpPatchNotes = false;
					DialogBoxW(
						GetModuleHandleW(nullptr),
						MAKEINTRESOURCEW(IDD_HELP),
						w_Handle,
						reinterpret_cast<DLGPROC>(&DlgProc_Help)
					);
					break;
				}
			}
			break;
		}
		case WM_SIZE:
		{
			static int sbWidths[3];

			RECT wRect = { };
			GetClientRect(w_Handle, &wRect);

			RECT wTextAreaRect = { };
			int textAreaHeight = 0;
			GetWindowRect(w_TextArea, &wTextAreaRect);

			RECT wStatusRect = { };
			int statusHeight = 0;
			GetWindowRect(w_StatusBar, &wStatusRect);
			SendMessageW(w_StatusBar, WM_SIZE, 0u, 0u);

			statusHeight = wStatusRect.bottom - wStatusRect.top;
			textAreaHeight = wTextAreaRect.bottom - statusHeight;
			MoveWindow(w_TextArea, 0, 0, wRect.right, wRect.bottom - statusHeight, TRUE);
			MoveWindow(w_StatusBar, 0, 0, wRect.right, 0, TRUE);
			
			sbWidths[0] = wRect.right / 3;
			sbWidths[1] = wRect.right / 2;
			sbWidths[2] = - 1;
			SendMessageW(w_StatusBar, SB_SETPARTS, 3u, reinterpret_cast<LPARAM>(sbWidths));
			break;
		}
		case WM_CLOSE:
		{
			DeleteObject(hbr);
			DestroyWindow(w_Handle);
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProcW(w_Handle, Msg, wParam, lParam);
	}
	return 0;
}

LRESULT __stdcall DlgProc_Settings(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// Buttons used for settings box.
	HWND w_ButtonApply = GetDlgItem(w_Dlg, ID_SETTINGS_BUTTON_APPLY);
	HWND w_ButtonDefs = GetDlgItem(w_Dlg, IDC_SETTINGS_BUTTON_RESTORE_DEFAULTS);

	// ========================= Editing Styles ==================================
	HWND w_Font = GetDlgItem(w_Dlg, IDC_SETTINGS_EDIT_FONT); // APPLY AFFECTED
	HWND w_DefaultFonts = GetDlgItem(w_Dlg, IDC_SETTINGS_BUTTON_PDFONTS);
	HWND w_FontSize = GetDlgItem(w_Dlg, IDC_SETTINGS_EDIT_FONTSIZE); // APPLY AFFECTED
	HWND w_CheckUnderline = GetDlgItem(w_Dlg, IDC_SETTINGS_CHECK_UNDERLINE); // APPLY AFFECTED
	HWND w_CheckItalic = GetDlgItem(w_Dlg, IDC_SETTINGS_CHECK_ITALIC); // APPLY AFFECTED
	HWND w_CheckBold = GetDlgItem(w_Dlg, IDC_SETTINGS_CHECK_BOLD); // APPLY AFFECTED

	// ========================= Control Styles ==================================
	HWND w_TaBkColorR = GetDlgItem(w_Dlg, IDC_SETTINGS_EDIT_COLOR_R); // APPLY AFFECTED
	HWND w_TaBkColorG = GetDlgItem(w_Dlg, IDC_SETTINGS_EDIT_COLOR_G); // APPLY AFFECTED
	HWND w_TaBkColorB = GetDlgItem(w_Dlg, IDC_SETTINGS_EDIT_COLOR_B); // APPLY AFFECTED
	HWND w_TaBkColorTextR = GetDlgItem(w_Dlg, IDC_SETTINGS_EDIT_COLOR_R_TEXT); // APPLY AFFECTED
	HWND w_TaBkColorTextG = GetDlgItem(w_Dlg, IDC_SETTINGS_EDIT_COLOR_G_TEXT); // APPLY AFFECTED
	HWND w_TaBkColorTextB = GetDlgItem(w_Dlg, IDC_SETTINGS_EDIT_COLOR_B_TEXT); // APPLY AFFECTED
	HWND w_CheckDarkTheme = GetDlgItem(w_Dlg, IDC_CHECK_DARK_THEME); // APPLY AFFECTED

	// ========================= Recent Files =====================================
	HWND w_ListBoxRecentFiles = GetDlgItem(w_Dlg, IDC_LIST_RECENT_FILES_HISTORY);
	HWND w_ButtonClearRecent = GetDlgItem(w_Dlg, IDC_BUTTON_CLEAR_RECENT_HISTORY);
	HWND w_ButtonDeleteSelection = GetDlgItem(w_Dlg, IDC_BUTTON_DELETE_RECENT_HISTORY_SELECTION);

	// Used to check if default settings are applied.
	bool bDefaultSettingsApplied = false;

	static SETTINGS_TUPLE currentSetsTuple = ::CurrentSettingsTuple;
	bool bItalic, bUnderline, bBold;
	wchar_t font[255], font_size[255];
	int fontSize = 0;
	wchar_t bkR[10], bkG[10], bkB[10];
	wchar_t bktR[10], bktG[10], bktB[10];
	int textAreaBkR = 255, textAreaBkG = 255, textAreaBkB = 255;
	int textAreaTextBkR = 0, textAreaTextBkG = 0, textAreaTextBkB = 0;
	bool bDarkTheme;
	static HBRUSH dlgbr = nullptr;

	if (!dlgbr)
	{
		if (::Runtime_bDarkThemeEnabled)
			dlgbr = CreateSolidBrush(RGB(0x33, 0x33, 0x33));
		else
			dlgbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	}

	switch (Msg)
	{
		case WM_INITDIALOG:
		{
			std::vector<std::wstring> recent_vec = PullRecentFilenames();
			if (recent_vec.size() < 1)
			{
				EnableWindow(w_ButtonClearRecent, FALSE);
				EnableWindow(w_ButtonDeleteSelection, FALSE);
			}
			else
				for (auto& a : recent_vec)
					SendMessageW(w_ListBoxRecentFiles, LB_ADDSTRING, 0u, (LPARAM)a.c_str());
			
			bUnderline = std::get<0>(currentSetsTuple);
			bBold = std::get<1>(currentSetsTuple);
			bItalic = std::get<2>(currentSetsTuple);
			wcscpy(font, std::get<3>(currentSetsTuple).c_str());
			fontSize = std::get<4>(currentSetsTuple);
			bDarkTheme = std::get<11>(currentSetsTuple);

			if (bUnderline)
				SendMessageW(w_CheckUnderline, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0u);
			else
				SendMessageW(w_CheckUnderline, BM_SETCHECK, static_cast<WPARAM>(BST_UNCHECKED), 0u);

			if (bBold)
				SendMessageW(w_CheckBold, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0u);
			else
				SendMessageW(w_CheckBold, BM_SETCHECK, static_cast<WPARAM>(BST_UNCHECKED), 0u);

			if (bItalic)
				SendMessageW(w_CheckItalic, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0u);
			else
				SendMessageW(w_CheckItalic, BM_SETCHECK, static_cast<WPARAM>(BST_UNCHECKED), 0u);

			if (bDarkTheme)
				SendMessageW(w_CheckDarkTheme, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0u);
			else
				SendMessageW(w_CheckDarkTheme, BM_SETCHECK, static_cast<WPARAM>(BST_UNCHECKED), 0u);

			SendMessageW(w_Font, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(std::get<3>(currentSetsTuple).c_str()));

			std::wostringstream ss;
			ss << std::get<4>(currentSetsTuple);
			SendMessageW(w_FontSize, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(ss.str().c_str()));

			ss.str(L"");
			ss << std::get<5>(currentSetsTuple);
			SendMessageW(w_TaBkColorR, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(ss.str().c_str()));

			ss.str(L"");
			ss << std::get<6>(currentSetsTuple);
			SendMessageW(w_TaBkColorG, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(ss.str().c_str()));

			ss.str(L"");
			ss << std::get<7>(currentSetsTuple);
			SendMessageW(w_TaBkColorB, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(ss.str().c_str()));

			ss.str(L"");
			ss << std::get<8>(currentSetsTuple);
			SendMessageW(w_TaBkColorTextR, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(ss.str().c_str()));

			ss.str(L"");
			ss << std::get<9>(currentSetsTuple);
			SendMessageW(w_TaBkColorTextG, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(ss.str().c_str()));

			ss.str(L"");
			ss << std::get<10>(currentSetsTuple);
			SendMessageW(w_TaBkColorTextB, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(ss.str().c_str()));

			//EnableWindow(w_ButtonDeleteSelection, FALSE);
			break;
		}
		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDCANCEL:
					DeleteObject(dlgbr);
					dlgbr = nullptr;
					EndDialog(w_Dlg, IDCANCEL);
					break;
				case IDOK:
					DeleteObject(dlgbr);
					dlgbr = nullptr;
					EndDialog(w_Dlg, IDOK);
				case ID_SETTINGS_BUTTON_APPLY:
				{
					// Get font text and font size text.
					GetWindowText(w_Font, font, 255);
					GetWindowText(w_FontSize, font_size, 255);

					// Get check states of editing style's check buttons
					bUnderline = IsDlgButtonChecked(w_Dlg, IDC_SETTINGS_CHECK_UNDERLINE) ? true : false;
					bBold = IsDlgButtonChecked(w_Dlg, IDC_SETTINGS_CHECK_BOLD) ? true : false;
					bItalic = IsDlgButtonChecked(w_Dlg, IDC_SETTINGS_CHECK_ITALIC) ? true : false;
					bDarkTheme = IsDlgButtonChecked(w_Dlg, IDC_CHECK_DARK_THEME) ? true : false;

					// Convert font size text string into integer
					std::wstringstream ss_sz(font_size);
					ss_sz >> fontSize;
					
					GetWindowText(w_TaBkColorR, bkR, 10);
					GetWindowText(w_TaBkColorG, bkG, 10);
					GetWindowText(w_TaBkColorB, bkB, 10);

					std::wstringstream wss_R(bkR), wss_G(bkG), wss_B(bkB);
					wss_R >> textAreaBkR;
					wss_G >> textAreaBkG;
					wss_B >> textAreaBkB;

					if ((textAreaBkR > 255) || (textAreaBkR < 0))
					{
						MessageBoxW(w_Dlg, L"[6] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaBkR = 255;
					}
					if ((textAreaBkG > 255) || (textAreaBkG < 0))
					{
						MessageBoxW(w_Dlg, L"[7] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaBkG = 255;
					}
					if ((textAreaBkB > 255) || (textAreaBkB < 0))
					{
						MessageBoxW(w_Dlg, L"[8] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaBkB = 255;
					}

					// For text area text color.
					GetWindowText(w_TaBkColorTextR, bktR, 10);
					GetWindowText(w_TaBkColorTextG, bktG, 10);
					GetWindowText(w_TaBkColorTextB, bktB, 10);

					std::wstringstream wss_tR(bktR), wss_tG(bktG), wss_tB(bktB);
					wss_tR >> textAreaTextBkR;
					wss_tG >> textAreaTextBkG;
					wss_tB >> textAreaTextBkB;

					if ((textAreaTextBkR > 255) || (textAreaTextBkR < 0))
					{
						MessageBoxW(w_Dlg, L"[9] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaTextBkR = 255;
					}
					if ((textAreaTextBkG > 255) || (textAreaTextBkG < 0))
					{
						MessageBoxW(w_Dlg, L"[10] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaTextBkG = 0;
					}
					if ((textAreaTextBkB > 255) || (textAreaTextBkB < 0))
					{
						MessageBoxW(w_Dlg, L"[11] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaTextBkB = 0;
					}

					// Make new SETTINGS_TUPLE with these new informations.
					currentSetsTuple = std::make_tuple(
						bUnderline,
						bBold,
						bItalic,
						font,
						fontSize,
						textAreaBkR,
						textAreaBkG,
						textAreaBkB,
						textAreaTextBkR,
						textAreaTextBkG,
						textAreaTextBkB,
						bDarkTheme
					);
			
					// Apply new settings.
					if (!ApplyNotepad32Settings(currentSetsTuple))
					{
						MessageBoxW(w_Dlg, L"Cannot apply settings!", L"Error!", MB_OK | MB_ICONERROR);
						return -1;
					}
					break;
				}
				case IDC_SETTINGS_BUTTON_PDFONTS:
				{
					DialogBox(
						GetModuleHandle(nullptr), 
						MAKEINTRESOURCE(IDD_DEF_FONTS), 
						w_Dlg, 
						reinterpret_cast<DLGPROC>(&DlgProc_DefaultFonts)
					);
					break;
				}
				case IDC_SETTINGS_BUTTON_RESTORE_DEFAULTS:
				{
					// Check if default settings aren't applied.
					if (!bDefaultSettingsApplied)
					{
						// Ask if user really wants to apply default settings.
						int confirm_rd = MessageBoxW(
							w_Dlg, 
							L"Default settings will be applied.\nProceed?", 
							L"Restore Defaults",  
							MB_YESNO | MB_ICONQUESTION
						);

						// Check for answer.
						switch (confirm_rd)
						{
							case IDYES:
							{
								SetWindowText(w_Font, L"Arial");
								SetWindowText(w_FontSize, L"18");
								Button_SetCheck(w_CheckUnderline, 0);
								Button_SetCheck(w_CheckItalic, 0);
								Button_SetCheck(w_CheckBold, 0);
								SetWindowText(w_TaBkColorR, L"255");
								SetWindowText(w_TaBkColorG, L"255");
								SetWindowText(w_TaBkColorB, L"255");
								SetWindowText(w_TaBkColorTextR, L"0");
								SetWindowText(w_TaBkColorTextG, L"0");
								SetWindowText(w_TaBkColorTextB, L"0");
								Button_SetCheck(w_CheckDarkTheme, 0);
								SendMessageW(GetDlgItem(w_Dlg, ID_SETTINGS_BUTTON_APPLY), BM_CLICK, 0u, 0u);
								break;
							}
							case IDNO:
								break;
							default:
								break;
						}
					}
					break;
				}
				case IDC_BUTTON_PICK_COLOR_BG:
				{
					COLORREF bgcol = ::GetColorFromDialog(w_Dlg, GetModuleHandle(nullptr));
					BYTE R = GetRValue(bgcol), G = GetGValue(bgcol), B = GetBValue(bgcol);

					std::wostringstream oss;
					oss << R; SetWindowText(w_TaBkColorR, oss.str().c_str()); oss.str(std::wstring());
					oss << G; SetWindowText(w_TaBkColorG, oss.str().c_str()); oss.str(std::wstring());
					oss << B; SetWindowText(w_TaBkColorB, oss.str().c_str()); oss.str(std::wstring());
					break;
				}
				case IDC_BUTTON_PICK_COLOR_TEXT:
				{
					COLORREF txcolor = ::GetColorFromDialog(w_Dlg, GetModuleHandle(nullptr));
					BYTE R = GetRValue(txcolor), G = GetGValue(txcolor), B = GetBValue(txcolor);

					std::wostringstream oss;
					oss << R; SetWindowText(w_TaBkColorTextR, oss.str().c_str()); oss.str(std::wstring());
					oss << G; SetWindowText(w_TaBkColorTextG, oss.str().c_str()); oss.str(std::wstring());
					oss << B; SetWindowText(w_TaBkColorTextB, oss.str().c_str()); oss.str(std::wstring());
					break;
				}
				case IDC_BUTTON_CLEAR_RECENT_HISTORY:
				{
					SendMessageW(w_ListBoxRecentFiles, LB_RESETCONTENT, 0, 0);
					::ClearRecentFilenames();
					EnableWindow(w_ButtonClearRecent, FALSE);
					EnableWindow(w_ButtonDeleteSelection, FALSE);
					break;
				}
				case IDC_BUTTON_DELETE_RECENT_HISTORY_SELECTION:
				{
					std::wofstream file;
					file.open(PATH_RECENT_FILES);
					if (file.is_open())
					{
						int item_count = SendMessageW(w_ListBoxRecentFiles, LB_GETCOUNT, 0u, 0u);
						int selected_count = 0;

						// If there is just one item left, disable this and clear buttons because it will be useless...
						if (item_count == 1)
						{
							EnableWindow(w_ButtonClearRecent, FALSE);
							EnableWindow(w_ButtonDeleteSelection, FALSE);
						}

						for (int i = 0; i < item_count; ++i)
						{
							int sel_index = SendMessageW(w_ListBoxRecentFiles, LB_GETSEL, static_cast<WPARAM>(i), 0u);
							selected_count += (sel_index > 0) ? 1 : 0;

							if (sel_index > 0)
							{
								SendMessageW(w_ListBoxRecentFiles, LB_DELETESTRING, static_cast<WPARAM>(i), 0u);
								std::wofstream file;
								file.open(PATH_RECENT_FILES, std::wios::trunc);
								if (file.is_open())
								{
									int count = SendMessageW(w_ListBoxRecentFiles, LB_GETCOUNT, 0u, 0u);
									static wchar_t buffer[MAX_PATH];
									for (int i = 0; i < count; ++i)
									{
										ListBox_GetText(w_ListBoxRecentFiles, i, buffer);
										file << buffer << std::endl;
									}
									file.close();
								}
							}
						}
						if (selected_count < 1)
						{
							MessageBox(w_Dlg, L"Item not selected.", L"Select Item", MB_OK);
							break;
						}
					}
					break;
				}
			}
		}
		case WM_CTLCOLOREDIT:
		{
			if (::Runtime_bDarkThemeEnabled)
			{
				HDC hdc = (HDC)wParam;
				SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
				SetBkColor(hdc, RGB(0x55, 0x55, 0x55));
				return (INT_PTR)CreateSolidBrush(RGB(0x55, 0x55, 0x55));
			}
			return (INT_PTR)dlgbr;
		}
		case WM_CTLCOLORBTN:
			return (LONG_PTR)dlgbr;
		case WM_CTLCOLORDLG:
			return reinterpret_cast<INT_PTR>(dlgbr);
		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			LOGBRUSH lbr {};

			GetObject(dlgbr, sizeof(HBRUSH), &lbr);
			SetBkColor(hdc, lbr.lbColor);

			if ((HWND)lParam == GetDlgItem(w_Dlg, IDC_STATIC_CHANGES_TAKES_EFFECT))
				SetTextColor(hdc, RGB(0xFF, 0x00, 0x00));
			else
			{
				if (::Runtime_bDarkThemeEnabled)
					SetTextColor(hdc, RGB(0xAA, 0xAA, 0xAA));
			}
			return (LONG_PTR)dlgbr;
		}
		case WM_CLOSE:
		{
			DeleteObject(dlgbr);
			dlgbr = nullptr;
			EndDialog(w_Dlg, 0);
			break;
		}
	}
	return 0;
}

// Arial; Tahoma; Times New Roman; Impact
LRESULT __stdcall DlgProc_DefaultFonts(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND w_FontsComboBox = GetDlgItem(w_Dlg, IDC_DEFFONT_COMBO_FONTS);
	switch (Msg)
	{
		case WM_INITDIALOG:
		{
			std::vector<std::wstring> fonts;

			std::wifstream file;
			file.open(PATH_FONT_SETTINGS);
			if (file.is_open())
			{
				std::wstring line;
				while (std::getline(file, line))
					fonts.push_back(line);
				file.close();
			}
			else
				MessageBoxW(GetParent(w_FontsComboBox), L"Cannot load font list from file.\nDefault fonts will be loaded.", L"Error", MB_OK | MB_ICONEXCLAMATION);

			if(fonts.size() < 1)
				fonts = std::vector<std::wstring> { L"Arial", L"Tahoma", L"Times New Roman", L"Impact", L"System" };

			for (std::size_t i = 0u; i < fonts.size(); ++i)
				SendMessageW(w_FontsComboBox, CB_ADDSTRING, 0u, reinterpret_cast<LPARAM>(fonts[i].c_str()));
			SendMessageW(w_FontsComboBox, CB_SETCURSEL, 0u, 0u);
			break;
		}
		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDOK:
				{
					::Runtime_DefaultSelectedFontFromDialog = new wchar_t[255];
					std::size_t current_select_index = static_cast<std::size_t>(
						SendMessageW(w_FontsComboBox, CB_GETCURSEL, 0u, 0u)
					);

					SendMessageW(
						w_FontsComboBox, 
						CB_GETLBTEXT, 
						static_cast<WPARAM>(current_select_index), 
						reinterpret_cast<LPARAM>(::Runtime_DefaultSelectedFontFromDialog)
					);
					SetWindowText(GetDlgItem(GetParent(w_Dlg), IDC_SETTINGS_EDIT_FONT), ::Runtime_DefaultSelectedFontFromDialog);
					delete[] ::Runtime_DefaultSelectedFontFromDialog;
					EndDialog(w_Dlg, IDOK);
					break;
				}
				case IDCANCEL:
					EndDialog(w_Dlg, IDCANCEL);
					break;
			}
			break;
		}
		case WM_CLOSE:
			EndDialog(w_Dlg, 0);
			break;
	}
	return 0;
}

LRESULT __stdcall DlgProc_About(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND w_AboutBy = GetDlgItem(w_Dlg, IDC_STATIC_ABOUT_ABOUTBY);
	HWND w_AboutStatic = GetDlgItem(w_Dlg, IDC_STATIC_ABOUTINFO);
	static wchar_t buffer_about[150] = { };

	switch (Msg)
	{
		case WM_INITDIALOG:
		{
			::LoadString(GetModuleHandle(nullptr), IDS_STRING_ABOUTBY, buffer_about, 150);
			SetWindowText(w_AboutBy, buffer_about);
			::LoadString(GetModuleHandle(nullptr), IDS_STRING_ABOUT, buffer_about, 150);
			SetWindowText(w_AboutStatic, buffer_about);
			break;
		}
		case WM_COMMAND:
			if (wParam == IDOK)
				EndDialog(w_Dlg, 0);
			break;
		case WM_CLOSE:
			EndDialog(w_Dlg, 0);
			break;
	}
	return 0;
}

LRESULT __stdcall DlgProc_Help(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND w_HelpBox = GetDlgItem(w_Dlg, IDC_EDIT_HELPEDIT);
	wchar_t char_buff[950];

	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		if (::Runtime_bHelpPatchNotes)
		{
			//std::wstring char_buff = LoadInfoFromFile(PATH_PATCH_NOTES);
			LoadString(GetModuleHandle(nullptr), IDS_STRING_PATCH, char_buff, 950);
			SetWindowTextW(w_HelpBox, char_buff);
			SetWindowTextW(w_Dlg, L"Patch Notes");
		}
		else
		{
			//std::wstring char_buff = LoadInfoFromFile(PATH_HOW_TO_USE);
			LoadString(GetModuleHandle(nullptr), IDS_STRING_HOWTOUSE, char_buff, 950);
			SetWindowTextW(w_HelpBox, char_buff);
			SetWindowTextW(w_Dlg, L"How to Use");
		}
		break;
	}
	case WM_COMMAND:
		if (wParam == IDOK)
			EndDialog(w_Dlg, 0);
		break;
	case WM_CLOSE:
		EndDialog(w_Dlg, 0);
		break;
	}
	return 0;
}

LRESULT __stdcall DlgProc_Find(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND w_ButtonFind = GetDlgItem(w_Dlg, IDC_BUTTON_FIND);
	HWND w_ButtonCount = GetDlgItem(w_Dlg, IDC_BUTTON_FIND_COUNT);
	HWND w_EditFind = GetDlgItem(w_Dlg, IDC_EDIT_FIND);
	HWND w_CheckMatchCase = GetDlgItem(w_Dlg, IDC_CHECK_MATCH_CASE);
	HWND w_CheckWrapAround = GetDlgItem(w_Dlg, IDC_CHECK_WRAP_AROUND);
	HWND w_CheckStoreToMem = GetDlgItem(w_Dlg, IDC_CHECK_STORE_INDEX_TO_MEMORY);
	HWND w_EditFoundIndex = GetDlgItem(w_Dlg, IDC_EDIT_FOUND_INDEX);
	HWND w_EditFoundLine = GetDlgItem(w_Dlg, IDC_EDIT_FOUND_LINE);

	static unsigned int sequence_index = 0u;
	wchar_t* buffer = nullptr;
	static bool bMatchCase = false;
	static bool bWrapAround = false;
	static bool bStoreToMem = false;

	switch (Msg)
	{
		case WM_INITDIALOG:
		{
			Button_SetCheck(GetDlgItem(w_Dlg, IDC_CHECK_MATCH_CASE), bMatchCase);
			Button_SetCheck(GetDlgItem(w_Dlg, IDC_CHECK_WRAP_AROUND), bWrapAround);
			Button_SetCheck(GetDlgItem(w_Dlg, IDC_CHECK_STORE_INDEX_TO_MEMORY), bStoreToMem);
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_BUTTON_FIND:
				{
					bMatchCase = IsDlgButtonChecked(w_Dlg, IDC_CHECK_MATCH_CASE);
					bWrapAround = IsDlgButtonChecked(w_Dlg, IDC_CHECK_WRAP_AROUND);
					bStoreToMem = IsDlgButtonChecked(w_Dlg, IDC_CHECK_STORE_INDEX_TO_MEMORY);

					std::vector<std::size_t> IndexVec;
					std::wstring temp_buffer_ta;

					// Get text from main text area.
					int ta_text_len = SendMessageW(w_TextArea, WM_GETTEXTLENGTH, 0u, 0u);
					if (ta_text_len < 1) return 1;
					ta_text_len += 1;
					wchar_t* tbuffer = new wchar_t[ta_text_len * sizeof(wchar_t)];
					SendMessageW(w_TextArea, WM_GETTEXT, (WPARAM)ta_text_len, reinterpret_cast<LPARAM>(tbuffer));
					temp_buffer_ta = tbuffer;
					delete[] tbuffer;

					// Get text from edit box.
					int text_len = SendMessageW(w_EditFind, WM_GETTEXTLENGTH, 0u, 0u);
					if (text_len < 1)
					{
						MessageBox(w_Dlg, L"Specify at least one character.", L"Find", MB_OK);
						sequence_index = 0;
						return 1;
					}

					text_len += 1;
					buffer = new wchar_t[text_len * sizeof(wchar_t)];
					SendMessageW(w_EditFind, WM_GETTEXT, (WPARAM)text_len, reinterpret_cast<LPARAM>(buffer));

					// Find all occurrences and store indexes of all occurrences inside a vector.
					std::size_t pos = temp_buffer_ta.find(buffer);
					// If there are no occurrences...
					if (pos == std::wstring::npos)
					{
						MessageBox(w_Dlg, L"There are no occurences with specified text.", L"No Occurrences.", MB_OK | MB_ICONINFORMATION);
						sequence_index = 0;
						delete[] buffer;
						break;
					}
					// If there ARE occurrences...
					while (pos != std::wstring::npos)
					{
						IndexVec.push_back(pos);
						pos = temp_buffer_ta.find(buffer, pos + 1);
					}

					// Selected text occurrence.
					if (IndexVec.size() > 0)
					{
						// Count of lines in a MainTextArea, starting from 1.
						int lines_count = 1; 
						lines_count += CountAllCases(temp_buffer_ta, L"\r\n", 1);

						if (sequence_index == IndexVec.size())
							sequence_index = 0u;
						SelectText(IndexVec[sequence_index], IndexVec[sequence_index] - 1 + text_len);
						SetWindowText(w_EditFoundIndex, ConvertToString<int>(IndexVec[sequence_index]).c_str());
						++sequence_index;
						
						// =========================== IN PROGRESS... =====================================
						//int current_line = SendMessageW(w_TextArea, EM_LINEFROMCHAR, (WPARAM)IndexVec[sequence_index], 0u);
						//SetWindowText(w_EditFoundLine, ConvertToString<int>(current_line).c_str());
						// ================================================================================
					}
					delete[] buffer;
					SetFocus(w_TextArea);
					break;
				}
				case IDC_BUTTON_FIND_COUNT:
				{
					std::wstring ta_text;
					int text_len = SendMessageW(w_TextArea, WM_GETTEXTLENGTH, 0u, 0u);
					if (text_len < 1)
						return 1;
					text_len += 1;
					buffer = new wchar_t[text_len * sizeof(wchar_t)];
					SendMessageW(w_TextArea, WM_GETTEXT, (WPARAM)text_len, reinterpret_cast<LPARAM>(buffer));
					ta_text = buffer;
					delete[] buffer;

					text_len = SendMessageW(w_EditFind, WM_GETTEXTLENGTH, 0u, 0u);
					buffer = new wchar_t[text_len * sizeof(wchar_t)];
					SendMessageW(w_EditFind, WM_GETTEXT, (WPARAM)text_len, reinterpret_cast<LPARAM>(buffer));
					
					std::wstringstream wss;
					wss << "Found " << CountAllCases(ta_text, buffer, text_len) << " occurrences.";
					delete[] buffer;
					MessageBoxW(w_Dlg, wss.str().c_str(), L"Found Occurrences", MB_OK | MB_ICONINFORMATION);
					break;
				}
				case IDCANCEL:
					EndDialog(w_Dlg, IDCANCEL);
					::Runtime_bFindDialogOpened = false;
					break;
			}
			break;
		}
		case WM_CLOSE:
			EndDialog(w_Dlg, 0);
			::Runtime_bFindDialogOpened = false;
			sequence_index = 0;
			break;
	}
	return 0;
}

LRESULT __stdcall WndProc_StatusBarInfo(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (Msg)
	{
		case WM_LBUTTONDOWN:
		{
			POINT cpos;
			GetCursorPos(&cpos);
			if (ScreenToClient(w_Handle, &cpos))
			{
				int arr_pos[3] = { };
				wchar_t sbpart_buff[255] = L"";

				SendMessageW(w_StatusBar, SB_GETPARTS, 3u, reinterpret_cast<LPARAM>(arr_pos));
				if (cpos.x <= arr_pos[0])
					SendMessageW(w_StatusBar, SB_GETTEXTW, 0u, reinterpret_cast<LPARAM>(sbpart_buff));
				else if (cpos.x <= arr_pos[1])
					SendMessageW(w_StatusBar, SB_GETTEXTW, 1u, reinterpret_cast<LPARAM>(sbpart_buff));
				else if (cpos.x > arr_pos[1])
					SendMessageW(w_StatusBar, SB_GETTEXTW, 2u, reinterpret_cast<LPARAM>(sbpart_buff));

				MessageBoxW(GetParent(w_Handle), sbpart_buff, L"Status Bar Info", MB_OK);
			}
			break;
		}
		default:
			return DefSubclassProc(w_Handle, Msg, wParam, lParam);
	}
	return 0;
}

int __stdcall WinMain(HINSTANCE w_Inst, HINSTANCE w_PrevInst, char* lpCmdLine, int nCmdShow)
{
	// Parse arguments...
	std::vector<std::wstring> ArgsVec;
	std::wstring open_with_path = L"NO_FILE";

	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	for (int i = 0; i < argc; ++i)
		ArgsVec.push_back(argv[i]);
	HeapFree(GetProcessHeap(), NULL, argv);

	if (argc > 1)
	{
		wcscpy(::Runtime_CurrentPath, ArgsVec[1].c_str());
		::Runtime_CurrentPathOpened = true;
	}

	::CurrentSettingsTuple = LoadNotepad32Settings();
	::Runtime_bDarkThemeEnabled = std::get<11>(::CurrentSettingsTuple);

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
	if (::Runtime_bDarkThemeEnabled)
		wcex.hbrBackground = CreateSolidBrush(RGB(0x33, 0x33, 0x33));
	else
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcex.hIcon = LoadIcon(nullptr, MAKEINTRESOURCE(IDI_WICON));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hIconSm = LoadIcon(nullptr, MAKEINTRESOURCE(IDI_WICON));

	if (!RegisterClassEx(&wcex))
	{
		MessageBoxW(nullptr, L"Cannot register class!", L"Error!", MB_OK | MB_ICONERROR);
		return -1;
	}

	HWND w_Handle = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		LP_CLASS_NAME,
		L"Notepad32",
		WS_OVERLAPPEDWINDOW,
		100, 100, 650, 500,
		nullptr, nullptr, nullptr, nullptr
	);

	if (w_Handle == nullptr)
	{
		MessageBoxW(nullptr, L"Cannot create a window!", L"Error!", MB_OK | MB_ICONERROR);
		return -1;
	}

	ShowWindow(w_Handle, SW_SHOWDEFAULT);
	UpdateWindow(w_Handle);

	MSG Msg = { };
	while (GetMessageW(&Msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		if (Msg.message == WM_KEYDOWN)
		{
			switch (Msg.wParam)
			{
				case 'S':
				{
					if (GetAsyncKeyState(VK_CONTROL))
						SaveTextProcedure(w_Handle, CRITERIA_FILE_SAVE);
					if (KeydownCombo(VK_CONTROL) && KeydownCombo(VK_SHIFT))
						SaveTextProcedure(w_Handle, CRITERIA_FILE_SAVE_AS);
					break;
				}
				case 'N':
				{
					if (GetAsyncKeyState(VK_CONTROL))
					{
						UpdateStatusForLengthLine(0, 0);
						::Runtime_CurrentPathOpened = false;
						memset(::Runtime_CurrentPath, 0, sizeof(::Runtime_CurrentPath));
						SendMessageW(w_TextArea, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(L""));
						SendMessageW(w_StatusBar, SB_SETTEXTW, 2u, reinterpret_cast<LPARAM>(L"No file loaded."));
					}
					break;
				}
			}
		}
		DispatchMessageW(&Msg);
	}
	return 0;
}

void InitUI(HWND w_Handle, HINSTANCE w_Inst)
{
	DWORD defStyle = (WS_VISIBLE | WS_CHILD);

	w_TextArea = CreateWindowW(
		WC_EDITW, nullptr,
		defStyle | WS_HSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
		0, 0, 300, 300,
		w_Handle, reinterpret_cast<HMENU>(IDC_EDIT_TEXTAREA), w_Inst, nullptr
	);

	int sbWidths[] = { 150, 350, -1 };
	w_StatusBar = CreateWindowExW(
		0, STATUSCLASSNAMEW, nullptr,
		defStyle | SBS_SIZEGRIP,
		0, 0, 0, 0,
		w_Handle, nullptr, w_Inst, nullptr
	);

	SendMessageW(w_StatusBar, SB_SETPARTS, 3u, reinterpret_cast<LPARAM>(sbWidths));
	UpdateStatusForLengthLine(0, 0);
	SendMessageW(w_StatusBar, SB_SETTEXTW, 2u, reinterpret_cast<LPARAM>(L"No file loaded."));

	const std::size_t control_num = 1;
	HWND w_Controls[control_num] = { w_TextArea };

	for (std::size_t i = 0; i < control_num; ++i)
	{
		if (w_Controls[i] == w_TextArea)
		{
			HFONT hFont = CreateFontW(
				std::get<4>(::CurrentSettingsTuple),
				0, 0, 0, 0,
				(DWORD)std::get<2>(::CurrentSettingsTuple),
				(DWORD)std::get<0>(::CurrentSettingsTuple),
				0, 0, 0, 0, 0, 0, (LPCWSTR)std::get<3>(::CurrentSettingsTuple).c_str()
			);

			SendMessageW(w_Controls[i], WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 1u);
			continue;
		}
		SendMessageW(w_Controls[i], WM_SETFONT, reinterpret_cast<WPARAM>((HFONT)GetStockObject(DEFAULT_GUI_FONT)), 1u);
	}
}

std::wstring OpenFileWithDialog(const wchar_t* Filters, HWND w_Handle, int criteria)
{
	OPENFILENAMEW ofn = { };
	wchar_t* _Path = new wchar_t[MAX_PATH];

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = w_Handle;
	ofn.lpstrFilter = Filters;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = _Path;
	ofn.lpstrFile[0] = '\0';
	if(criteria == 1) ofn.lpstrTitle = L"Open File";
	else ofn.lpstrTitle = L"Save File";
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = (OFN_EXPLORER | OFN_PATHMUSTEXIST);

	if (criteria == 1)
	{
		if (!GetOpenFileNameW(&ofn))
			return std::wstring(L"\0");
	}
	else
	{
		if (!GetSaveFileNameW(&ofn))
			return std::wstring(L"\0");
	}

	std::wstring __Path(_Path);
	delete[] _Path;
	return __Path;
}

bool WriteReadFileToTextArea(const wchar_t* Path)
{
	SendMessageW(w_StatusBar, SB_SETTEXTW, 2u, reinterpret_cast<LPARAM>(Path));
	wcscpy(Runtime_CurrentPath, Path);

	if (Path != nullptr)
	{
		std::wifstream file;
		file.open(Path);
		if (file.is_open())
		{
			//std::wstring textBuffer;
			//file.seekg(0, std::wios::end);
			//std::size_t file_size = file.tellg();
			//file.seekg(0, std::wios::beg);
			//
			//wchar_t* buffer = new wchar_t[file_size];
			//file.read(buffer, file_size);
			//buffer[file_size - 1] = '\0';
			//
			//textBuffer = buffer;
			//
			//std::vector<std::size_t> index_vec;
			//FindAllCaseSensitiveOccurrences(textBuffer, L"\n", index_vec);
			//if (index_vec.size() > 0)
			//	for (auto& a : index_vec)
			//		textBuffer[a] = L'\r\n';
			//delete[] buffer;

			std::wstring textBuffer;
			std::wstring line;
			while (std::getline(file, line))
			{
				// Needs fix for last new line append. After last line, there shouldn't be new line if that's not in file.
				textBuffer += line.c_str();
				textBuffer += L"\r\n";
			}

			SendMessageW(w_TextArea, WM_SETTEXT, 0u, reinterpret_cast<LPARAM>(textBuffer.c_str()));
			int text_length = SendMessageW(w_TextArea, WM_GETTEXTLENGTH, 0u, 0u);
			int text_lines = SendMessageW(w_TextArea, EM_GETLINECOUNT, 0u, 0u);
			UpdateStatusForLengthLine(text_length, text_lines);
			file.close();
		}
		else
			return false;
	}
	return true;
}

bool SaveTextProcedure(HWND w_Handle, int criteria)
{
	if (criteria == CRITERIA_FILE_SAVE)
	{
		// Get number of characters in Text Area.
		int text_len = SendMessageW(w_TextArea, WM_GETTEXTLENGTH, 0u, 0u) + 2;
		// If text length is greater than 0, save file.
		if (text_len > 0)
		{
			// Check if there was file already opened and edited.
			if (!::Runtime_CurrentPathOpened)
			{
				// If there is no opened file, save file with dialog.
				std::wstring path(OpenFileWithDialog(L"Text File\0*.txt\0", w_Handle, CRITERIA_FILE_SAVE));
				if (path == L"\0")
					return 1;
				if (!SaveTextToFile(path.c_str()))
				{
					MessageBoxW(w_Handle, L"Cannot Save File!", L"Error!", MB_OK | MB_ICONERROR);
					return -1;
				}
				// Set boolean that tells us that we have one existing file that's been edited right now.
				::Runtime_CurrentPathOpened = true;
				PushRecentFilename(path.c_str());
			}
			else
			{
				// If there is already one file that is opened and edited, just save without dialog.
				if (!SaveTextToFile(::Runtime_CurrentPath))
				{
					// If failure occures during saving process, return error code.
					MessageBoxW(w_Handle, L"Cannot Save File!", L"Error!", MB_OK | MB_ICONERROR);
					return -1;
				}
			}
		}
		else
			MessageBoxW(GetParent(w_TextArea), L"File\'s text length must be at least 1", L"Notepad32", MB_OK | MB_ICONINFORMATION);
	}
	else if (criteria == CRITERIA_FILE_SAVE_AS)
	{
		std::size_t text_len = static_cast<std::size_t>(SendMessageW(w_TextArea, WM_GETTEXTLENGTH, 0u, 0u));
		if (text_len > 0u)
		{
			std::wstring path(OpenFileWithDialog(L"Text File\0*.txt\0All Files\0*.*\0", w_Handle, CRITERIA_FILE_SAVE));
			if (path == L"\0")
				return true;
			if (!SaveTextToFile(path.c_str()))
			{
				MessageBoxW(w_Handle, L"Cannot Save File!", L"Error!", MB_OK | MB_ICONERROR);
				return -1;
			}
			::Runtime_CurrentPathOpened = true;
			PushRecentFilename(path.c_str());
		}
	}
}

bool SaveTextToFile(const wchar_t* path)
{
	int text_len = SendMessageW(w_TextArea, WM_GETTEXTLENGTH, 0u, 0u) + 1;
	wchar_t* buffer = nullptr;

	SendMessageW(w_StatusBar, SB_SETTEXTW, 2u, reinterpret_cast<LPARAM>(path));
	wcscpy(::Runtime_CurrentPath, path);
	
	std::wofstream file(path);
	
	if (file.is_open())
	{
		buffer = new wchar_t[text_len];
		if (buffer == nullptr)
		{
			MessageBoxW(GetParent(w_TextArea), L"Cannot allocate memory buffer for file!", L"Fatal Error!", MB_OK | MB_ICONERROR);
			return false;
		}

		SendMessageW(w_TextArea, WM_GETTEXT, static_cast<WPARAM>(text_len), reinterpret_cast<LPARAM>(buffer));

		// ============= ADDED to prevent line separating after save. ==============
		std::wstring tempp(buffer);
		std::vector<std::size_t> poses_vec;
		FindAllCaseSensitiveOccurrences(tempp, L"\r", poses_vec);
		if(poses_vec.size() > 0)
			for (auto& a : poses_vec)
				tempp.erase(a, 1);
		// =========================================================================
		//file.write(buffer, text_len);
		file.write(tempp.c_str(), text_len - poses_vec.size() - 1);
		file.close();
	}
	else
		return false;
	delete[] buffer;
	return true;
}

SETTINGS_TUPLE LoadNotepad32Settings()
{
	SETTINGS_TUPLE tTemplate;
	bool bItalic, bUnderline, bBold;
	std::wstring font;
	int fontSize = 0;
	int textAreaBkR = 255, textAreaBkG = 255, textAreaBkB = 255;
	int textAreaTextBkR = 0, textAreaTextBkG = 0, textAreaTextBkB = 0;
	bool bDarkTheme;

	std::wfstream file;
	file.open(PATH_MAIN_SETTINGS, std::wios::in | std::wios::out);
	if (file.is_open())
	{
		std::wstring line;
		std::wstring settingsListStr;
		std::size_t index = 0u;
		std::wstringstream woss_R, woss_G, woss_B;
		std::wstringstream woss_tR, woss_tG, woss_tB;

		while (std::getline(file, line))
		{
			switch (index)
			{
				case 0:
					bUnderline = CheckSettingsValueAndSet(line);
					break;
				case 1:
					bBold = CheckSettingsValueAndSet(line);
					break;
				case 2:
					bItalic = CheckSettingsValueAndSet(line);
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
				case 5:
				{
					if (!CheckColorSetting(line))
						break;
					woss_R << line;
					woss_R >> textAreaBkR;
					if ((textAreaBkR > 255) || (textAreaBkR < 0))
					{
						MessageBoxW(0, L"[6] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaBkR = 255;
					}
					break;
				}
				case 6:
				{
					if (!CheckColorSetting(line))
						break;
					woss_G << line;
					woss_G >> textAreaBkG;
					if ((textAreaBkG > 255) || (textAreaBkG < 0))
					{
						MessageBoxW(0, L"[7] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaBkG = 255;
					}
					break;
				}
				case 7:
				{
					if (!CheckColorSetting(line))
						break;
					woss_B << line;
					woss_B >> textAreaBkB;
					if ((textAreaBkB > 255) || (textAreaBkB < 0))
					{
						MessageBoxW(0, L"[8] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaBkB = 255;
					}
					break;
				}
				case 8:
				{
					if (!CheckColorSetting(line))
						break;
					woss_tR << line;
					woss_tR >> textAreaTextBkR;
					if ((textAreaTextBkR > 255) || (textAreaTextBkR < 0))
					{
						MessageBoxW(0, L"[8] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaTextBkR = 0;
					}
					break;
				}
				case 9:
				{
					if (!CheckColorSetting(line))
						break;
					woss_tG << line;
					woss_tG >> textAreaTextBkG;
					if ((textAreaTextBkG > 255) || (textAreaTextBkG < 0))
					{
						MessageBoxW(0, L"[9] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaTextBkG = 0;
					}
					break;
				}
				case 10:
				{
					if (!CheckColorSetting(line))
						break;
					woss_tB << line;
					woss_tB >> textAreaTextBkB;
					if ((textAreaTextBkB > 255) || (textAreaTextBkB < 0))
					{
						MessageBoxW(0, L"[10] Color value must be:\n0 > value <= 255.", L"Error", MB_OK | MB_ICONINFORMATION);
						textAreaTextBkB = 0;
					}
					break;
				}
				case 11:
					bDarkTheme = CheckSettingsValueAndSet(line);
					break;
			}
			++index;
		}
	}
	else
		return std::make_tuple(false, false, false, L"Arial", 0, 255, 255, 255, 0, 0, 0, false);
	return std::make_tuple(
		bUnderline, bBold, bItalic, 
		font.c_str(), fontSize, 
		textAreaBkR, textAreaBkG, textAreaBkB,
		textAreaTextBkR, textAreaTextBkG, textAreaTextBkB,
		bDarkTheme
	);
}

bool ApplyNotepad32Settings(SETTINGS_TUPLE& settings_tuple)
{
	std::wostringstream ss_temp;
	std::wofstream file;
	file.open(PATH_MAIN_SETTINGS);
	if (file.is_open())
	{
		ss_temp << CheckSettingsValueAndApply(std::get<0>(settings_tuple)).c_str() << std::endl;
		ss_temp << CheckSettingsValueAndApply(std::get<1>(settings_tuple)).c_str() << std::endl;
		ss_temp << CheckSettingsValueAndApply(std::get<2>(settings_tuple)).c_str() << std::endl;
		ss_temp << std::get<3>(settings_tuple).c_str() << std::endl;
		ss_temp << std::get<4>(settings_tuple) << std::endl;
		ss_temp << std::get<5>(settings_tuple) << std::endl;
		ss_temp << std::get<6>(settings_tuple) << std::endl;
		ss_temp << std::get<7>(settings_tuple) << std::endl;
		ss_temp << std::get<8>(settings_tuple) << std::endl;
		ss_temp << std::get<9>(settings_tuple) << std::endl;
		ss_temp << std::get<10>(settings_tuple) << std::endl;
		ss_temp << CheckSettingsValueAndApply(std::get<11>(settings_tuple)).c_str(); // << std::endl; Last line must be excepted if there is no more settings lines!
		file << ss_temp.str().c_str();
		file.close();
	}
	else
		return false;
	return true;
}

void UpdateStatusForLengthLine(int length, int lines)
{
	std::wostringstream len_ss, line_ss;
	len_ss << "Length: " << length;
	line_ss << "Lines: " << lines;
	SendMessageW(w_StatusBar, SB_SETTEXTW, 0u, reinterpret_cast<LPARAM>(len_ss.str().c_str()));
	SendMessageW(w_StatusBar, SB_SETTEXTW, 1u, reinterpret_cast<LPARAM>(line_ss.str().c_str()));
}

int KeydownCombo(int key)
{
	return (GetAsyncKeyState(key) & 0x8000) != 0;
}

void CopyTextToClipboard()
{
	try
	{
		unsigned long begin_sel;
		unsigned long end_sel;
		SendMessageW(w_TextArea, EM_GETSEL, (WPARAM)&begin_sel, (LPARAM)&end_sel);

		int sel_len = SendMessageW(w_TextArea, WM_GETTEXTLENGTH, 0u, 0u) + 1;
		if (sel_len < 1)
			return;

		wchar_t* buff = new wchar_t[sel_len];
		SendMessageW(w_TextArea, WM_GETTEXT, (WPARAM)sel_len, reinterpret_cast<LPARAM>(buff));
		
		// TODO: This next line is likely to throw exceptions commonly.
		// It will be fixed in future versions.
		std::wstring selected_text = SubstringSelectedText(buff, begin_sel, end_sel);
		delete[] buff;

		if (selected_text.length() < 1)
			return;

		const std::size_t len = wcslen(selected_text.c_str());
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (len + 1) * sizeof(wchar_t));
		wchar_t* gdst = reinterpret_cast<wchar_t*>(GlobalLock(hMem));
		memcpy(gdst, selected_text.c_str(), len * sizeof(wchar_t));
		GlobalUnlock(hMem);
		OpenClipboard(nullptr);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
		CloseClipboard();
		GlobalFree(hMem);
	}
	catch (std::exception& e)
	{
		MessageBoxA(0, e.what(), "Exception Thrown!", MB_OK | MB_ICONERROR);
		return;
	}
	return;
}

std::wstring GetTextFromClipboard()
{
	static std::wstring return_hlp = std::wstring();
	
	OpenClipboard(nullptr);
	HGLOBAL hMem = (HGLOBAL)GetClipboardData(CF_UNICODETEXT);
	wchar_t* clipboard_text = (wchar_t*)GlobalLock(hMem);
	if (clipboard_text)
	{
		return_hlp = clipboard_text;
		GlobalUnlock(hMem);
	}
	CloseClipboard();
	return return_hlp;
}

bool CheckColorSetting(std::wstring line)
{
	bool bErr = false;
	for (std::wstring::iterator itr = line.begin(); itr != line.end(); ++itr)
	{
		if (!std::isdigit(*itr))
		{
			bErr = true;
			break;
		}
		else
			continue;
	}

	if (bErr)
		return false;
	return true;
}

std::wstring SubstringSelectedText(const wchar_t* pEditText, unsigned long begin, unsigned long end)
{
	static std::wstring temp_buff(pEditText);
	std::wstring selected_text = temp_buff.substr(begin, end);
	selected_text = selected_text.erase(end - begin, selected_text.size());
	return selected_text;
}

void RemoveSelectedText(HWND w_Handle, bool bPaste, std::wstring clipboard_text)
{
	unsigned long begin_sel;
	unsigned long end_sel;
	SendMessageW(w_Handle, EM_GETSEL, reinterpret_cast<WPARAM>(&begin_sel), reinterpret_cast<LPARAM>(&end_sel));

	std::size_t sel_len = (std::size_t)(end_sel - begin_sel);

	int len = GetWindowTextLengthW(w_Handle);
	if (len < 1) return;
	len += 1;

	wchar_t* buff = new wchar_t[len];
	GetWindowTextW(w_Handle, buff, len * sizeof(wchar_t));

	std::wstring temp_buff(buff);
	std::wstring primary_part_before = temp_buff.substr(0ull, begin_sel);
	std::wstring primary_part_after = temp_buff.substr(primary_part_before.length() + sel_len, temp_buff.length());
	
	if (bPaste)
		temp_buff = primary_part_before + clipboard_text + primary_part_after;
	else
		temp_buff = primary_part_before + primary_part_after;

	SetWindowTextW(w_Handle, temp_buff.c_str());
	
	if(bPaste)
		SendMessageW(w_Handle, EM_SETSEL, (WPARAM)primary_part_before.size() + clipboard_text.size(), (LPARAM)primary_part_before.size() + clipboard_text.size());
	else
		SendMessageW(w_Handle, EM_SETSEL, (WPARAM)primary_part_before.size(), (LPARAM)primary_part_before.size());
	delete[] buff;
	return;
}

bool CheckFileChanges(const wchar_t* ta_text)
{
	std::wstring temp_opened(ta_text);
	std::wostringstream woss;
	
	std::wifstream file;
	file.open(::Runtime_CurrentPath);
	if (file.is_open())
	{
		woss << file.rdbuf();
		file.close();
		if (woss.str() != temp_opened)
		{
			int confirm = MessageBoxW(GetParent(w_TextArea), L"File is not saved.\nSave?", L"Unsaved File", MB_YESNO | MB_ICONQUESTION);
			if (confirm == IDYES)
			{
				SaveTextToFile(::Runtime_CurrentPath);
				return true;
			}
		}
	}
	return false;
}

int CountAllCases(std::wstring ta_text, const wchar_t* wfind, int find_length)
{
	// TODO: Always missing one occurrence. Needs fix!
	int count = 0;
	std::size_t pos = ta_text.find(wfind);
	while (pos != std::wstring::npos)
	{
		pos = ta_text.find(wfind, pos + 1);
		count += 1;
	}
	return count;
}

void SelectText(int from, int to)
{
	SendMessageW(w_TextArea, EM_SETSEL, static_cast<WPARAM>(from), static_cast<LPARAM>(to));
	return;
}

std::size_t FindCaseSensitive(std::wstring tstr, const wchar_t* wfind, std::size_t pos = 0ull)
{
	std::wstring wsfind(wfind);
	std::transform(tstr.begin(), tstr.end(), tstr.begin(), ::tolower);
	std::transform(wsfind.begin(), wsfind.end(), wsfind.begin(), ::tolower);
	return tstr.find(wfind, pos);
}

void FindAllCaseSensitiveOccurrences(std::wstring tstr, const wchar_t* wfind, std::vector<std::size_t>& pos_vec)
{
	std::wstring wsfind(wfind);
	std::size_t pos = tstr.find(wsfind);
	while (pos != std::wstring::npos)
	{
		pos_vec.push_back(pos);
		pos = FindCaseSensitive(tstr, wfind, pos + wsfind.size());
	}
	return;
}

COLORREF GetColorFromDialog(HWND w_Handle, HINSTANCE w_Inst)
{
	CHOOSECOLOR lcc;

	memset(&lcc, 0, sizeof(CHOOSECOLOR));
	lcc.lStructSize = sizeof(CHOOSECOLOR);
	lcc.Flags = CC_FULLOPEN | CC_RGBINIT;
	lcc.hwndOwner = w_Handle;
	lcc.hInstance = nullptr;
	lcc.rgbResult = ::Runtime_rgbCurrent;
	lcc.lpCustColors = ::Runtime_customColors;

	if(ChooseColor(&lcc))
	{
		::Runtime_rgbCurrent = lcc.rgbResult;
		return lcc.rgbResult;
	}
	return ::Runtime_rgbCurrent;
}

void PushRecentFilename(const wchar_t* filename)
{
	if (CheckRecentFilenameExistence(filename))
		return;

	std::wofstream file;
	file.open(PATH_RECENT_FILES, std::wios::out | std::wios::app);
	if (file.is_open())
	{
		file << filename << std::endl;
		file.close();
	}
	else
		MessageBoxW(GetParent(w_TextArea), L"Cannot push recent file.", L"File Open Error", MB_OK | MB_ICONERROR);
	return;
}

void PushRecentMenuItems(HWND w_Handle)
{
	HMENU w_MenuBar = GetMenu(w_Handle);
	HMENU w_File = GetSubMenu(w_MenuBar, 0);
	HMENU w_Recent = GetSubMenu(w_File, 6);
	//AppendMenuA(w_Recent, MF_POPUP | MF_STRING, (UINT_PTR)40015, "dasdasd"); // TODO: TEST
	return;
}

void ClearRecentFilenames()
{
	std::wofstream file;
	file.open(PATH_RECENT_FILES, std::wios::trunc);
	file.close();
	return;
}

std::vector<std::wstring> PullRecentFilenames()
{
	std::vector<std::wstring> pull_vec;
	std::wifstream file;
	file.open(PATH_RECENT_FILES);
	if (file.is_open())
	{
		std::wstring line;
		while (std::getline(file, line))
			pull_vec.push_back(line);
		file.close();
	}
	return pull_vec;
}

bool CheckRecentFilenameExistence(const wchar_t* filename)
{
	std::wifstream file;
	file.open(filename);
	if (file.is_open())
	{
		std::wostringstream wss;
		wss << file.rdbuf();
		file.close();

		std::size_t pos = wss.str().find(filename);
		if (pos != std::wstring::npos)
			return true;
	}
	return false;
}

std::wstring LoadInfoFromFile(const wchar_t* path)
{
	static std::wstring res_buffer_content;
	std::wifstream file;
	file.open(path);
	if (file.is_open())
	{
		std::wostringstream woss;
		std::wstring line;
		while (std::getline(file, line))
			woss << line;
		file.close();
		res_buffer_content = woss.str();
	}
	else
		MessageBox(nullptr, L"Cannot load info from file.", L"Cannot load info", MB_OK | MB_ICONERROR);
	return res_buffer_content;
}

bool CheckSettingsValueAndSet(std::wstring value)
{
	if (value == L"true")
		return true;
	else
		return false;
}

std::wstring CheckSettingsValueAndApply(bool value)
{
	if (value)
		return L"true";
	return L"false";
}

template<typename T>
std::wstring ConvertToString(T var)
{
	std::wstring temp;
	std::wostringstream woss;
	woss << var;
	temp = woss.str();
	return temp;
}
