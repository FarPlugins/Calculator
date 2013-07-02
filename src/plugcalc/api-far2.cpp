//
//  Copyright (c) uncle-vunkis 2009-2012 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include "api.h"

#include <farplugin/2.0/plugin.hpp>
#include <farplugin/2.0/farkeys.hpp>
#include <farplugin/2.0/farcolor.hpp>


static const wchar_t *PluginMenuStrings, *PluginConfigStrings;
static CalcDialog::CalcDialogCallback *msg_table = NULL;

/////////////////////////////////////////////////////////////////////////////////

enum
{
	DefaultColor   = 0xf,
	ConsoleMask    = 0xf,
	ConsoleBgShift = 4,
	ConsoleFgShift = 0
};

inline void Far2ColorToCalcColor(int Color, CalcColor *clr)
{
	clr->Flags = CALC_FCF_FG_4BIT | CALC_FCF_BG_4BIT;
	clr->ForegroundColor = (Color >> ConsoleFgShift) & ConsoleMask;
	clr->BackgroundColor = (Color >> ConsoleBgShift) & ConsoleMask;
	clr->Reserved = NULL;
}

inline int CalcColorToFar2Color(CalcColor *clr)
{
#if 0
	const CALCCOLORFLAGS consoleColors = CALC_FCF_FG_4BIT | CALC_FCF_BG_4BIT;
	if ((clr->Flags & consoleColors) != consoleColors) 
		return DefaultColor;
#endif

	return	((clr->ForegroundColor & ConsoleMask) << ConsoleFgShift) | 
			((clr->BackgroundColor & ConsoleMask) << ConsoleBgShift);
}


/////////////////////////////////////////////////////////////////////////////////


class CalcDialogFuncsFar2 : public CalcDialogFuncs
{
public:
	CalcDialogFuncsFar2(PluginStartupInfo *Info)
	{
		this->Info = Info;
		cache_used = false;
		cache_used2 = false;
	}

	virtual void EnableRedraw(DLGHANDLE hdlg, bool en)
	{
		Info->SendDlgMessage(hdlg, DM_ENABLEREDRAW, en ? TRUE : FALSE, 0);
	}
	virtual void ResizeDialog(DLGHANDLE hdlg, const CalcCoord & dims)
	{
		Info->SendDlgMessage(hdlg, DM_RESIZEDIALOG, 0, (LONG_PTR)&dims);
	}
	virtual void RedrawDialog(DLGHANDLE hdlg)
	{
		Info->SendDlgMessage(hdlg, DM_REDRAW, 0, 0);
	}
	virtual void Close(DLGHANDLE hdlg, int exitcode)
	{
		Info->SendDlgMessage(hdlg, DM_CLOSE, exitcode, 0);
	}
	virtual void GetDlgRect(DLGHANDLE hdlg, CalcRect *rect)
	{
		Info->SendDlgMessage(hdlg, DM_GETDLGRECT, 0, (LONG_PTR)rect);
	}

	virtual void GetDlgItemShort(DLGHANDLE hdlg, int id, CalcDialogItem *item)
	{
		FarDialogItem fdi;
		Info->SendDlgMessage(hdlg, DM_GETDLGITEMSHORT, id, (LONG_PTR)&fdi);
		ConvertToDlgItem((void *)&fdi, item);
	}
	virtual void SetDlgItemShort(DLGHANDLE hdlg, int id, const CalcDialogItem & item)
	{
		FarDialogItem fdi;
		ConvertFromDlgItem(&item, (void *)&fdi);
		Info->SendDlgMessage(hdlg, DM_SETDLGITEMSHORT, id, (LONG_PTR)&fdi);
	}
	virtual void SetItemPosition(DLGHANDLE hdlg, int id, const CalcRect & rect)
	{
		Info->SendDlgMessage(hdlg, DM_SETITEMPOSITION, id, (LONG_PTR)&rect);
	}
	virtual int  GetFocus(DLGHANDLE hdlg)
	{
		return (int)Info->SendDlgMessage(hdlg, DM_GETFOCUS, 0, 0);
	}
	virtual void SetFocus(DLGHANDLE hdlg, int id)
	{
		Info->SendDlgMessage(hdlg, DM_SETFOCUS, id, 0);
	}
	virtual void EditChange(DLGHANDLE hdlg, int id, const CalcDialogItem & item)
	{
		FarDialogItem fdi;
		ConvertFromDlgItem(&item, (void *)&fdi);
		Info->SendDlgMessage(hdlg, DN_EDITCHANGE, id, (LONG_PTR)&fdi);
	}
	virtual void SetSelection(DLGHANDLE hdlg, int id, const CalcEditorSelect & sel)
	{
		Info->SendDlgMessage(hdlg, DM_SETSELECTION, id, (LONG_PTR)&sel);
	}
	virtual void SetCursorPos(DLGHANDLE hdlg, int id, const CalcCoord & pos)
	{
		Info->SendDlgMessage(hdlg, DM_SETCURSORPOS, id, (LONG_PTR)&pos);
	}
	virtual void GetText(DLGHANDLE hdlg, int id, std::wstring &str)
	{
		int len = (int)Info->SendDlgMessage(hdlg, DM_GETTEXTPTR, id, 0);
		str.clear();
		str.resize(len);
		Info->SendDlgMessage(hdlg, DM_GETTEXTPTR, id, (LONG_PTR)str.data());
	}
	virtual void SetText(DLGHANDLE hdlg, int id, const std::wstring & str)
	{
		Info->SendDlgMessage(hdlg, DM_SETTEXTPTR, id, (LONG_PTR)str.c_str());
	}
	virtual void AddHistory(DLGHANDLE hdlg, int id, const std::wstring & str)
	{
		Info->SendDlgMessage(hdlg, DM_ADDHISTORY, id, (LONG_PTR)str.c_str());
	}
	virtual bool  IsChecked(DLGHANDLE hdlg, int id)
	{
		return ((int)Info->SendDlgMessage(hdlg, DM_GETCHECK, id, 0) == BSTATE_CHECKED);
	}

	////////////////////////////////////////////////////////////////////////////////////

	inline void *cache_malloc(size_t s)
	{
		if (cache_used)
		{
			 if (cache_used2)
				return malloc(s);
			 cache_used2 = true;
			 return cache_storage2;
		}
		cache_used = true;
		return cache_storage;
	}

	inline void cache_free(void *b)
	{
		if (b == cache_storage)
			cache_used = false;
		else if (b == cache_storage2)
			cache_used2 = false;
		else
			free(b);
	}

	virtual DLGHANDLE DialogInit(int id, int X1, int Y1, int X2, int Y2, const wchar_t *HelpTopic, 
								struct CalcDialogItem *Item, unsigned int ItemsNumber,
								CALCDLGPROC dlgProc)
	{
		FarDialogItem *faritems = (FarDialogItem *)malloc(ItemsNumber * sizeof(FarDialogItem));
		for (unsigned i = 0; i < ItemsNumber; i++)
		{
			ConvertFromDlgItem(&Item[i], (void *)&faritems[i]);
		}
		return Info->DialogInit(Info->ModuleNumber, X1, Y1, X2, Y2, HelpTopic, faritems, ItemsNumber, 0, 0, 
								(FARWINDOWPROC)dlgProc, NULL);
	}

	virtual intptr_t DialogRun(DLGHANDLE hdlg)
	{
		return (intptr_t)Info->DialogRun(hdlg);
	}

	virtual void DialogFree(DLGHANDLE hdlg)
	{
		Info->DialogFree(hdlg);
	}

	virtual CALC_INT_PTR DefDlgProc(DLGHANDLE hdlg, int msg, int param1, void *param2)
	{
		return Info->DefDlgProc(hdlg, msg, param1, (LONG_PTR)param2);
	}
	virtual void ConvertToDlgItem(void *faritem, CalcDialogItem *calcitem)
	{
		FarDialogItem *fi = (FarDialogItem *)faritem;
		calcitem->Type = (CalcDialogItemTypes)fi->Type;
		calcitem->X1 = fi->X1;
		calcitem->Y1 = fi->Y1;
		calcitem->X2 = fi->X2;
		calcitem->Y2 = fi->Y2;
		calcitem->Reserved = (intptr_t)fi->Reserved;
		calcitem->Flags = fi->Flags;
		if (fi->Focus)
			calcitem->Flags |= CALC_DIF_FOCUS;
		if (fi->DefaultButton)
			calcitem->Flags |= CALC_DIF_DEFAULTBUTTON;
		calcitem->PtrData = fi->PtrData;
	}
	virtual void ConvertFromDlgItem(const CalcDialogItem *calcitem, void *faritem)
	{
		FarDialogItem *fi = (FarDialogItem *)faritem;
		fi->Type = calcitem->Type;
		fi->X1 = calcitem->X1;
		fi->Y1 = calcitem->Y1;
		fi->X2 = calcitem->X2;
		fi->Y2 = calcitem->Y2;
		fi->Focus = (calcitem->Flags & CALC_DIF_FOCUS) ? 1 : 0;
		fi->Reserved = (DWORD_PTR)calcitem->Reserved;
		fi->Flags = (DWORD)calcitem->Flags;		// TODO: it works?
		fi->DefaultButton = (calcitem->Flags & CALC_DIF_DEFAULTBUTTON) ? 1 : 0;
		fi->PtrData = calcitem->PtrData;
		fi->MaxLen = 0;
	}

	virtual CALC_INT_PTR PreProcessMessage(DLGHANDLE hdlg, int msg, int &param1, void * &param2)
	{
		if (msg == DN_KEY)
		{
			DWORD key = (DWORD)param2;
			INPUT_RECORD *ir = (INPUT_RECORD *)cache_malloc(sizeof(INPUT_RECORD));
			ir->EventType = KEY_EVENT;
			ir->Event.KeyEvent.bKeyDown = TRUE;
			ir->Event.KeyEvent.dwControlKeyState = 0;
			ir->Event.KeyEvent.wVirtualKeyCode = VK_NONCONVERT;
			if (key & KEY_CTRLMASK)
			{
				if (key & KEY_CTRL)
					ir->Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
				if (key & KEY_ALT)
					ir->Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
				if (key & KEY_SHIFT)
					ir->Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
			}
			key &= ~KEY_CTRLMASK;
			if (key < EXTENDED_KEY_BASE)
			{
				//ir->Event.KeyEvent.AsciiChar = (CHAR)key;
				if (key == KEY_ENTER)
					ir->Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
			}
			else if (key >= EXTENDED_KEY_BASE && key <= KEY_END_FKEY)
				ir->Event.KeyEvent.wVirtualKeyCode = (WORD)(key - EXTENDED_KEY_BASE);
			else if (key >= INTERNAL_KEY_BASE)
			{
				ir->Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;
				switch (key)
				{
				case KEY_NUMENTER:
					ir->Event.KeyEvent.wVirtualKeyCode = VK_RETURN; break;
				case KEY_NUMDEL:
					ir->Event.KeyEvent.wVirtualKeyCode = VK_DELETE; break;
				};
			}
			param2 = (void *)ir;
		}
		else if (msg == DN_EDITCHANGE)
		{
			CalcDialogItem *Item = (CalcDialogItem *)cache_malloc(sizeof(CalcDialogItem));	// no ctor!
			ConvertToDlgItem(param2, Item);
			param2 = (void *)Item;
		}
		else if (msg == DN_CTLCOLORDLGITEM)
		{
			CalcDialogItemColors *Item = (CalcDialogItemColors *)cache_malloc(sizeof(CalcDialogItemColors) + sizeof(CalcColor)*4);
			Item->StructSize = sizeof(CalcDialogItemColors);
			Item->Flags = 0;
			Item->ColorsCount = 4;
			Item->Colors = (CalcColor *)(Item + 1);
			Far2ColorToCalcColor(LOBYTE(LOWORD(param2)), &Item->Colors[0]);
			Far2ColorToCalcColor(HIBYTE(LOWORD(param2)), &Item->Colors[1]);
			Far2ColorToCalcColor(LOBYTE(HIWORD(param2)), &Item->Colors[2]);
			Far2ColorToCalcColor(HIBYTE(HIWORD(param2)), &Item->Colors[3]);
			param2 = (void *)Item;
		}
		return 0;
	}

	virtual CALC_INT_PTR PostProcessMessage(DLGHANDLE hdlg, int msg, CALC_INT_PTR &ret, int &param1, void * &param2)
	{
		if (msg == DN_EDITCHANGE)
		{
			cache_free(param2);
		}
		else if (msg == DN_KEY)
		{
			cache_free(param2);
		}
		else if (msg == DN_CTLCOLORDLGITEM)
		{
			CalcDialogItemColors *Item = (CalcDialogItemColors *)param2;
			ret = MAKELONG(
				MAKEWORD(CalcColorToFar2Color(&Item->Colors[0]), CalcColorToFar2Color(&Item->Colors[1])),
				MAKEWORD(CalcColorToFar2Color(&Item->Colors[2]), CalcColorToFar2Color(&Item->Colors[3]))
				);
			cache_free(param2);
		}
		return 0;
	}

	virtual CalcDialog::CalcDialogCallback *GetMessageTable()
	{
		if (msg_table == NULL)
		{
			const int max_msg_id = DM_USER + 32;
			msg_table = (CalcDialog::CalcDialogCallback *)malloc(max_msg_id * sizeof(CalcDialog::CalcDialogCallback));
			if (msg_table == NULL)
				return NULL;
			
			memset(msg_table, 0, max_msg_id * sizeof(CalcDialog::CalcDialogCallback));	
			// fill the LUT
			msg_table[DN_INITDIALOG]      = &CalcDialog::OnInitDialog;
			msg_table[DN_CLOSE]           = &CalcDialog::OnClose;
			msg_table[DN_RESIZECONSOLE]   = &CalcDialog::OnResizeConsole;
			msg_table[DN_DRAWDIALOG]      = &CalcDialog::OnDrawDialog;
			msg_table[DN_BTNCLICK]        = &CalcDialog::OnButtonClick;
			msg_table[DN_GOTFOCUS]        = &CalcDialog::OnGotFocus;
			msg_table[DN_EDITCHANGE]      = &CalcDialog::OnEditChange;
			msg_table[DN_KEY]             = &CalcDialog::OnInput;	// See PreProcessMessage
			msg_table[DN_CTLCOLORDLGITEM] = &CalcDialog::OnCtrlColorDlgItem;
		}
		return msg_table;
	}

protected:
	PluginStartupInfo *Info;

	char cache_storage[1024], cache_storage2[1024];
	bool cache_used, cache_used2;
};

static CalcDialogFuncsFar2 *dlg_funcs = NULL;

/////////////////////////////////////////////////////////////////////////////////

class CalcApiFar2 : public CalcApi
{
public:
	
	virtual void GetPluginInfo(void *pinfo, const wchar_t *name)
	{
		struct PluginInfo *pInfo = (struct PluginInfo *)pinfo;
		pInfo->StructSize = sizeof(*pInfo);
		pInfo->Flags = PF_EDITOR | PF_VIEWER;
		pInfo->DiskMenuStringsNumber = 0;

		PluginMenuStrings = name;
		pInfo->PluginMenuStrings = &PluginMenuStrings;
		pInfo->PluginMenuStringsNumber = 1;

		PluginConfigStrings = name;
		pInfo->PluginConfigStrings = &PluginConfigStrings;
		pInfo->PluginConfigStringsNumber = 1;
	}

	virtual bool IsOpenedFromEditor(void *, int OpenFrom)
	{
		return (OpenFrom == OPEN_EDITOR);
	}

	virtual const wchar_t *GetMsg(int MsgId)
	{
		return Info.GetMsg(Info.ModuleNumber, MsgId);
	}

	virtual int GetMinVersion(void *ver)
	{
		return MAKEFARVERSION(2, 0, 994);
	}

	virtual CalcDialogFuncs *GetDlgFuncs()
	{
		if (!dlg_funcs)
			dlg_funcs = new CalcDialogFuncsFar2(&Info);
		return dlg_funcs;
	}

	virtual intptr_t Message(unsigned long Flags, const wchar_t *HelpTopic, const wchar_t * const *Items,
						int ItemsNumber, int ButtonsNumber)
	{
		return (intptr_t)Info.Message(Info.ModuleNumber, Flags, HelpTopic, Items, ItemsNumber, ButtonsNumber);
	}

	virtual intptr_t Menu(int X, int Y, int MaxHeight, unsigned long long Flags,
						const wchar_t *Title, const wchar_t *HelpTopic, 
						const std::vector<CalcMenuItem> & Items)
	{
		/// \TODO: find less ugly impl.
		const struct FarMenuItem *items = (const struct FarMenuItem *)&(*Items.begin());

		return (intptr_t)Info.Menu(Info.ModuleNumber, X, Y, MaxHeight, (DWORD)Flags, Title, NULL, HelpTopic,
							NULL, NULL, items, (int)Items.size());
	}

	virtual void EditorGet(CalcEditorGetString *string, CalcEditorInfo *info)
	{
		EditorGetString egs;
		egs.StringNumber = (int)string->StringNumber;
		Info.EditorControl(ECTL_GETSTRING, &egs);
		string->StructSize = sizeof(CalcEditorGetString);
		string->StringNumber = egs.StringNumber;
		string->StringLength = egs.StringLength;
		string->StringText = egs.StringText;
		string->StringEOL = egs.StringEOL;
		string->SelStart = egs.SelStart;
		string->SelEnd = egs.SelEnd;

		EditorInfo ei;
		Info.EditorControl(ECTL_GETINFO, &ei);
		info->StructSize = sizeof(CalcEditorInfo);
		info->EditorID = ei.EditorID;
		info->WindowSizeX = ei.WindowSizeX;
		info->WindowSizeY = ei.WindowSizeY;
		info->TotalLines = ei.TotalLines;
		info->CurLine = ei.CurLine;
		info->CurPos = ei.CurPos;
		info->CurTabPos = ei.CurTabPos;
		info->TopScreenLine = ei.TopScreenLine;
		info->LeftPos = ei.LeftPos;
		info->Overtype = ei.Overtype;
		info->BlockType = ei.BlockType;
		info->BlockStartLine = ei.BlockStartLine;
		info->Options = ei.Options;
		info->TabSize = ei.TabSize;
		info->BookmarkCount = 0;
		info->SessionBookmarkCount = 0;
		info->CurState = ei.CurState;
		info->CodePage = ei.CodePage;
	}

	virtual void EditorSelect(const CalcEditorSelect & sel)
	{
		Info.EditorControl(ECTL_SELECT, (void *)&sel);
	}

	virtual void EditorInsert(const wchar_t *text)
	{
		Info.EditorControl(ECTL_INSERTTEXT, (void *)text);
	}

	virtual void EditorRedraw()
	{
		Info.EditorControl(ECTL_REDRAW, 0);
	}

	virtual void GetDlgColors(CalcColor *edit_color, CalcColor *sel_color, CalcColor *highlight_color)
	{
		int e_color, s_color, h_color;

		e_color = (int)Info.AdvControl(Info.ModuleNumber, ACTL_GETCOLOR, (void*)COL_DIALOGEDIT);
		s_color = (int)Info.AdvControl(Info.ModuleNumber, ACTL_GETCOLOR, (void*)COL_DIALOGEDITSELECTED);
		h_color = (int)Info.AdvControl(Info.ModuleNumber, ACTL_GETCOLOR, (void*)COL_DIALOGHIGHLIGHTTEXT);

		Far2ColorToCalcColor(e_color, edit_color);
		Far2ColorToCalcColor(s_color, sel_color);
		Far2ColorToCalcColor(h_color, highlight_color);
	}

	virtual int GetCmdLine(std::wstring &cmd)
	{
		int len = Info.Control(INVALID_HANDLE_VALUE, FCTL_GETCMDLINE, 0, 0);
		cmd.clear();
		if (len > 0)
		{
			cmd.resize(len);
			Info.Control(INVALID_HANDLE_VALUE, FCTL_GETCMDLINE, len, (LONG_PTR)cmd.data());
			// rtrim
			if (cmd[len - 1] == '\0')
				cmd.resize(len - 1);
		}
		return len;
	}

	virtual void SetCmdLine(const std::wstring & cmd)
	{
		Info.Control(INVALID_HANDLE_VALUE, FCTL_SETCMDLINE, (int)cmd.size(), (LONG_PTR)cmd.data());
	}

	virtual bool SettingsBegin()
	{
		hReg = 0;
		wchar_t Key[256];
		swprintf(Key, 256, L"%s\\calculator", Info.RootKey);
		return RegCreateKeyEx(HKEY_CURRENT_USER, Key, 0, NULL, REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS, NULL, &hReg, NULL) == ERROR_SUCCESS;
	}
	virtual bool SettingsEnd()
	{
		RegCloseKey(hReg);
		return true;
	}

	virtual bool SettingsGet(const wchar_t *name, std::wstring *sval, unsigned long *ival)
	{
		bool ret = false;
		DWORD len = 0;
		if (RegQueryValueEx(hReg, name, 0, NULL, NULL, &len) == ERROR_SUCCESS && len > 0)
		{
			if (sval)
			{
				sval->clear();
				sval->resize(len);
				if (RegQueryValueEx(hReg, name, 0, NULL, (PBYTE)sval->data(), &len) == ERROR_SUCCESS)
					ret = true;
			}
			else if (ival)
			{
				if (RegQueryValueEx(hReg, name, 0, NULL, (PBYTE)ival, &len) == ERROR_SUCCESS)
					ret = true;
				if (len != 4)
					ret = false;
			}
		}
		return ret;
	}

	virtual bool SettingsSet(const wchar_t *name, const std::wstring *sval, const unsigned long *ival)
	{
		bool ret = false;

		if (sval)
		{
			if (RegSetValueEx(hReg, name, 0, REG_SZ, (UCHAR*)sval->c_str(), (DWORD)(sval->size() * sizeof(wchar_t))) == ERROR_SUCCESS)
				ret = true;
		}
		else if (ival)
		{
			if (RegSetValueEx(hReg, name, 0, REG_DWORD, (UCHAR*)ival, sizeof(DWORD)) == ERROR_SUCCESS)
				ret = true;
		}
		return ret;
	}

	virtual const wchar_t *GetModuleName()
	{
		return Info.ModuleName;
	}

protected:
	friend CalcApi *CreateApiFar2(void *info);

	PluginStartupInfo Info;
	HKEY hReg;
};

/////////////////////////////////////////////////////////////////////////////////

CalcApi *CreateApiFar2(void *info)
{
	PluginStartupInfo *psi = (PluginStartupInfo *)info;

	if (psi == NULL || psi->StructSize < sizeof(PluginStartupInfo) ||			// far container is too old
						psi->AdvControl == NULL)
		return NULL;

	// check far version directly
	DWORD ver = (DWORD)psi->AdvControl(psi->ModuleNumber, ACTL_GETFARVERSION, NULL);
	if (HIBYTE(ver) != 2)
		return NULL;

	CalcApiFar2 *api = new CalcApiFar2();

	if (api == NULL)
		return NULL;

	memset(&api->Info, 0, sizeof(api->Info));
	memmove(&api->Info, psi, (psi->StructSize > sizeof(api->Info)) ? sizeof(api->Info) : psi->StructSize);

	return api;
}

