//
//  Copyright (c) uncle-vunkis 2009-2012 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include <cmath>
#include <string>
#include <initguid.h>

#include "api.h"
#include "version.h"

#include <farplugin/3.0/plugin.hpp>
#include <farplugin/3.0/farcolor.hpp>

// {894EAABB-C57F-4549-95FC-4AC6F3102A36}
DEFINE_GUID(MainGuid,    0x894eaabb, 0xc57f, 0x4549, 0x95, 0xfc, 0x4a, 0xc6, 0xf3, 0x10, 0x2a, 0x36);
// {DCEFBFC8-9C43-49e5-ABE6-875FED184652}
DEFINE_GUID(PMenuGuid,   0xdcefbfc8, 0x9c43, 0x49e5, 0xab, 0xe6, 0x87, 0x5f, 0xed, 0x18, 0x46, 0x52);
// {BED0CE4A-A56F-4b78-AC76-A2BC41F1AD72}
DEFINE_GUID(PConfigGuid, 0xbed0ce4a, 0xa56f, 0x4b78, 0xac, 0x76, 0xa2, 0xbc, 0x41, 0xf1, 0xad, 0x72);
// {8855681E-60E0-4fc3-93AF-462C19456E47}
DEFINE_GUID(MsgGuid,     0x8855681e, 0x60e0, 0x4fc3, 0x93, 0xaf, 0x46, 0x2c, 0x19, 0x45, 0x6e, 0x47);
// {52264845-1004-428e-8C3D-CC14AEA6A9FC}
DEFINE_GUID(MenuGuid,    0x52264845, 0x1004, 0x428e, 0x8c, 0x3d, 0xcc, 0x14, 0xae, 0xa6, 0xa9, 0xfc);


static const GUID dlg_guids[] = 
{
	// {E45555AE-6499-443c-AA04-12A1AADAB989}
	{ 0xe45555ae, 0x6499, 0x443c, { 0xaa, 0x4, 0x12, 0xa1, 0xaa, 0xda, 0xb9, 0x89 } },
	// {ED8EAEEA-FF70-4c53-B891-C0D5EA85D0E7}
	{ 0xed8eaeea, 0xff70, 0x4c53, { 0xb8, 0x91, 0xc0, 0xd5, 0xea, 0x85, 0xd0, 0xe7 } },
	// {C7ACE7B7-DAB1-4968-A202-3CE94D750400}
	{ 0xc7ace7b7, 0xdab1, 0x4968, { 0xa2, 0x2, 0x3c, 0xe9, 0x4d, 0x75, 0x4, 0x0 } },

};

static const wchar_t *PluginMenuStrings, *PluginConfigStrings;
static CalcDialog::CalcDialogCallback *msg_table = nullptr;

/////////////////////////////////////////////////////////////////////////////////

class CalcDialogFuncsFar3 : public CalcDialogFuncs
{
public:
	CalcDialogFuncsFar3(PluginStartupInfo *Info)
	{
		this->Info = Info;
	}

	void EnableRedraw(DLGHANDLE hdlg, bool en) override
	{
		Info->SendDlgMessage(hdlg, DM_ENABLEREDRAW, en ? TRUE : FALSE, nullptr);
	}
	void ResizeDialog(DLGHANDLE hdlg, const CalcCoord & dims) override
	{
		Info->SendDlgMessage(hdlg, DM_RESIZEDIALOG, 0, (void *)&dims);
	}
	void RedrawDialog(DLGHANDLE hdlg) override
	{
		Info->SendDlgMessage(hdlg, DM_REDRAW, 0, nullptr);
	}
	void Close(DLGHANDLE hdlg, int exitcode) override
	{
		Info->SendDlgMessage(hdlg, DM_CLOSE, exitcode, nullptr);
	}
	void GetDlgRect(DLGHANDLE hdlg, CalcRect *rect) override
	{
		Info->SendDlgMessage(hdlg, DM_GETDLGRECT, 0, (void *)rect);
	}

	void GetDlgItemShort(DLGHANDLE hdlg, int id, CalcDialogItem *item) override
	{
		FarDialogItem fdi{};
		Info->SendDlgMessage(hdlg, DM_GETDLGITEMSHORT, id, (void *)&fdi);
		ConvertToDlgItem((void *)&fdi, item);
	}
	void SetDlgItemShort(DLGHANDLE hdlg, int id, const CalcDialogItem & item) override
	{
		FarDialogItem fdi{};
		ConvertFromDlgItem(&item, (void *)&fdi);
		Info->SendDlgMessage(hdlg, DM_SETDLGITEMSHORT, id, (void *)&fdi);
	}
	void SetItemPosition(DLGHANDLE hdlg, int id, const CalcRect & rect) override
	{
		Info->SendDlgMessage(hdlg, DM_SETITEMPOSITION, id, (void *)&rect);
	}
	int GetFocus(DLGHANDLE hdlg) override
	{
		return (int)Info->SendDlgMessage(hdlg, DM_GETFOCUS, 0, nullptr);
	}
	void SetFocus(DLGHANDLE hdlg, int id) override
	{
		Info->SendDlgMessage(hdlg, DM_SETFOCUS, id, nullptr);
	}
	void EditChange(DLGHANDLE hdlg, int id, const CalcDialogItem & item) override
	{
		FarDialogItem fdi{};
		ConvertFromDlgItem(&item, (void *)&fdi);
		Info->SendDlgMessage(hdlg, DN_EDITCHANGE, id, (void *)&fdi);
	}
	void SetSelection(DLGHANDLE hdlg, int id, const CalcEditorSelect & sel) override
	{
		Info->SendDlgMessage(hdlg, DM_SETSELECTION, id, (void *)&sel);
	}
	void SetCursorPos(DLGHANDLE hdlg, int id, const CalcCoord & pos) override
	{
		Info->SendDlgMessage(hdlg, DM_SETCURSORPOS, id, (void *)&pos);
	}
	void GetText(DLGHANDLE hdlg, int id, std::wstring &str) override
	{
		FarDialogItemData item = { sizeof(FarDialogItemData) };
		item.PtrLength = Info->SendDlgMessage(hdlg, DM_GETTEXT, id, nullptr);
		str.clear();
		str.resize(item.PtrLength + 1);
		item.PtrData = (wchar_t *)str.data();
		Info->SendDlgMessage(hdlg, DM_GETTEXT, id, &item);
	}
	void SetText(DLGHANDLE hdlg, int id, const std::wstring & str) override
	{
		Info->SendDlgMessage(hdlg, DM_SETTEXTPTR, id, (void *)str.c_str());
	}
	void AddHistory(DLGHANDLE hdlg, int id, const std::wstring & str) override
	{
		Info->SendDlgMessage(hdlg, DM_ADDHISTORY, id, (void *)str.c_str());
	}
	bool IsChecked(DLGHANDLE hdlg, int id) override
	{
		return ((int)Info->SendDlgMessage(hdlg, DM_GETCHECK, id, nullptr) == BSTATE_CHECKED);
	}

	////////////////////////////////////////////////////////////////////////////////////

	DLGHANDLE DialogInit(int id, int X1, int Y1, int X2, int Y2, const wchar_t *HelpTopic,
								struct CalcDialogItem *Item, unsigned int ItemsNumber,
								CALCDLGPROC dlgProc) override
	{
		if (id >= sizeof(dlg_guids) / sizeof(dlg_guids[0]))
			return nullptr;

		FarDialogItem *faritems = (FarDialogItem *)malloc(ItemsNumber * sizeof(FarDialogItem));
		for (unsigned i = 0; i < ItemsNumber; i++)
		{
			ConvertFromDlgItem(&Item[i], (void *)&faritems[i]);
		}
		
		return Info->DialogInit(&MainGuid, &dlg_guids[id], X1, Y1, X2, Y2, HelpTopic, faritems, ItemsNumber, 0, 0, 
			(FARWINDOWPROC)dlgProc, nullptr);
	}

	intptr_t DialogRun(DLGHANDLE hdlg) override
	{
		return Info->DialogRun(hdlg);
	}

	void DialogFree(DLGHANDLE hdlg) override
	{
		Info->DialogFree(hdlg);
	}

	CALC_INT_PTR DefDlgProc(DLGHANDLE hdlg, int msg, int param1, void *param2) override
	{
		return Info->DefDlgProc(hdlg, msg, param1, (void *)param2);
	}
	void ConvertToDlgItem(void *faritem, CalcDialogItem *calcitem) override
	{
		FarDialogItem *fi = (FarDialogItem *)faritem;
		calcitem->Type = (CalcDialogItemTypes)fi->Type;
		calcitem->X1 = (int)fi->X1;
		calcitem->Y1 = (int)fi->Y1;
		calcitem->X2 = (int)fi->X2;
		calcitem->Y2 = (int)fi->Y2;
		calcitem->Reserved = fi->Reserved0;
		calcitem->Flags = fi->Flags;
		calcitem->PtrData = (const wchar_t *)fi->Data;
	}
	void ConvertFromDlgItem(const CalcDialogItem *calcitem, void *faritem) override
	{
		FarDialogItem *fi = (FarDialogItem *)faritem;
		fi->Type = (FARDIALOGITEMTYPES)calcitem->Type;
		fi->X1 = calcitem->X1;
		fi->Y1 = calcitem->Y1;
		fi->X2 = calcitem->X2;
		fi->Y2 = calcitem->Y2;
		fi->Reserved0 = calcitem->Reserved;
		fi->Flags = calcitem->Flags;
		fi->Data = calcitem->PtrData;
		fi->History = (calcitem->Flags & CALC_DIF_HISTORY) ? L"calc" : nullptr;
		fi->Mask = nullptr;
		fi->UserData = 0;
		fi->MaxLength = 0;
	}

	CALC_INT_PTR PreProcessMessage(DLGHANDLE hdlg, int msg, int &param1, void * &param2) override
	{
		if (msg == DN_EDITCHANGE)
		{
			CalcDialogItem *Item = new CalcDialogItem();
			ConvertToDlgItem(param2, Item);
			param2 = (void *)Item;
		}
		return 0;
	}

	CALC_INT_PTR PostProcessMessage(DLGHANDLE hdlg, int msg, CALC_INT_PTR &ret, int &param1, void * &param2) override
	{
		if (msg == DN_EDITCHANGE)
		{
			delete param2;
		}
		return 0;
	}

	CalcDialog::CalcDialogCallback *GetMessageTable() override
	{
		if (msg_table == nullptr)
		{
			const int max_msg_id = DM_USER + 32;
			msg_table = (CalcDialog::CalcDialogCallback *)malloc(max_msg_id * sizeof(CalcDialog::CalcDialogCallback));
			if (msg_table == nullptr)
				return nullptr;
			memset(msg_table, 0, max_msg_id * sizeof(CalcDialog::CalcDialogCallback));
			// fill the LUT
			msg_table[DN_INITDIALOG]      = &CalcDialog::OnInitDialog;
			msg_table[DN_CLOSE]           = &CalcDialog::OnClose;
			msg_table[DN_RESIZECONSOLE]   = &CalcDialog::OnResizeConsole;
			msg_table[DN_DRAWDIALOG]      = &CalcDialog::OnDrawDialog;
			msg_table[DN_BTNCLICK]        = &CalcDialog::OnButtonClick;
			msg_table[DN_GOTFOCUS]        = &CalcDialog::OnGotFocus;
			msg_table[DN_EDITCHANGE]      = &CalcDialog::OnEditChange;
			msg_table[DN_CONTROLINPUT]    = &CalcDialog::OnInput;
			msg_table[DN_CTLCOLORDLGITEM] = &CalcDialog::OnCtrlColorDlgItem;
		}
		return msg_table;
	}

protected:
	PluginStartupInfo *Info;
};

static CalcDialogFuncsFar3 *dlg_funcs = nullptr;

///////////////////////////////////////////////////////////////////////////////////

class CalcApiFar3 : public CalcApi
{
public:
	void GetPluginInfo(void *pinfo, const wchar_t *name) override
	{
		struct PluginInfo *pInfo = (struct PluginInfo *)pinfo;
		pInfo->StructSize = sizeof(*pInfo);
		pInfo->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG;

		PluginMenuStrings = name;
		pInfo->PluginMenu.Guids = &PMenuGuid;
		pInfo->PluginMenu.Strings = &PluginMenuStrings;
		pInfo->PluginMenu.Count = 1;

		PluginConfigStrings = name;
		pInfo->PluginConfig.Guids = &PConfigGuid; 
		pInfo->PluginConfig.Strings = &PluginConfigStrings;
		pInfo->PluginConfig.Count = 1;

		plugin_name = name;
	}

	bool IsOpenedFromEditor(void *oinfo, int OpenFrom) override
	{
		struct OpenInfo *oInfo = (struct OpenInfo *)oinfo;
		return (oInfo->OpenFrom == OPEN_EDITOR);
	}

	const wchar_t *GetMsg(int MsgId) override
	{
		return Info.GetMsg(&MainGuid, MsgId);
	}

	CalcDialogFuncs *GetDlgFuncs() override
	{
		if (!dlg_funcs)
			dlg_funcs = new CalcDialogFuncsFar3(&Info);
		return dlg_funcs;
	}

	intptr_t Message(unsigned long Flags, const wchar_t *HelpTopic, const wchar_t * const *Items,
						int ItemsNumber, int ButtonsNumber) override
	{
		return Info.Message(&MainGuid, &MsgGuid, Flags, HelpTopic, Items, ItemsNumber, ButtonsNumber);
	}

	intptr_t Menu(int X, int Y, int MaxHeight, unsigned long long Flags,
		const wchar_t *Title, const wchar_t *HelpTopic, 
		const std::vector<CalcMenuItem> & Items) override
	{
		std::vector<struct FarMenuItem> Items3(Items.size());

		/// \TODO: find less ugly impl.
		struct FarMenuItem *items = (struct FarMenuItem *)&(*Items3.begin());

		memset((void *)items, 0, sizeof(struct FarMenuItem) * Items.size());
		for (unsigned i = 0; i < Items.size(); i++)
		{
			items[i].Text = Items[i].Text;
			if (Items[i].Selected)
				items[i].Flags |= MIF_SELECTED;
			if (Items[i].Checked)
				items[i].Flags |= MIF_CHECKED;
			if (Items[i].Separator)
				items[i].Flags |= MIF_SEPARATOR;
		}

		/// \TODO: 3.x flags are partially incompatible!
		FARMENUFLAGS flags = (FARMENUFLAGS)Flags;

		return Info.Menu(&MainGuid, &MenuGuid, X, Y, MaxHeight, flags, Title, nullptr, HelpTopic,
                         nullptr, nullptr, items, Items.size());
	}

	void EditorGet(CalcEditorGetString *string, CalcEditorInfo *info) override
	{
		Info.EditorControl(-1, ECTL_GETSTRING, 0, string);
		Info.EditorControl(-1, ECTL_GETINFO, 0, info);
	}

	void EditorSelect(const CalcEditorSelect & sel) override
	{
		Info.EditorControl(-1, ECTL_SELECT, 0, (void *)&sel);
	}

	void EditorInsert(const wchar_t *text) override
	{
		Info.EditorControl(-1, ECTL_INSERTTEXT, 0, (void *)text);
	}

	void EditorRedraw() override
	{
		Info.EditorControl(-1, ECTL_REDRAW, 0, nullptr);
	}
	
	void GetDlgColors(CalcColor *edit_color, CalcColor *sel_color, CalcColor *highlight_color) override
	{
		Info.AdvControl(&MainGuid, ACTL_GETCOLOR, COL_DIALOGEDIT, (void *)edit_color);
		Info.AdvControl(&MainGuid, ACTL_GETCOLOR, COL_DIALOGEDITSELECTED, (void *)sel_color);
		Info.AdvControl(&MainGuid, ACTL_GETCOLOR, COL_DIALOGHIGHLIGHTTEXT, (void *)highlight_color);
	}

	int GetCmdLine(std::wstring &cmd) override
	{
		int len = (int)Info.PanelControl(INVALID_HANDLE_VALUE, FCTL_GETCMDLINE, 0, nullptr);
		cmd.clear();
		if (len > 0)
		{
			cmd.resize(len);
			Info.PanelControl(INVALID_HANDLE_VALUE, FCTL_GETCMDLINE, len, (void *)cmd.data());
			// rtrim
			if (cmd[len - 1] == '\0')
				cmd.resize(len - 1);
		}
		return len;
	}

	void SetCmdLine(const std::wstring & cmd) override
	{
		Info.PanelControl(INVALID_HANDLE_VALUE, FCTL_SETCMDLINE, (int)cmd.size(), (void *)cmd.data());
	}


	bool SettingsBegin() override
	{
		FarSettingsCreate settings = { sizeof(FarSettingsCreate), MainGuid, INVALID_HANDLE_VALUE };
		bool ret = false;
		if (Info.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &settings))
		{
			settings_handle = settings.Handle;
			return true;
		}
		else
		{
			settings_handle = nullptr;
			return false;
		}
	}
	
	bool SettingsEnd() override
	{
		Info.SettingsControl(settings_handle, SCTL_FREE, 0, nullptr);
		return true;
	}

	bool SettingsGet(const wchar_t *name, std::wstring *sval, unsigned long *ival) override
	{
		bool ret = false;
		if (settings_handle)
		{
			FarSettingsItem item{0};
            item.StructSize = sizeof(FarSettingsItem);
			item.Name = name;
			item.Type = (sval) ? FST_STRING : FST_QWORD;
			if (Info.SettingsControl(settings_handle, SCTL_GET, 0, &item))
			{
				if (sval)
				{
					*sval = item.String;
					ret = true;
				}
				if (ival)
				{
					*ival = (DWORD)item.Number;
					ret = true;
				}
			}
		}
		return ret;
	}

	bool SettingsSet(const wchar_t *name, const std::wstring *sval, const unsigned long *ival) override
	{
		bool ret = false;
		if (settings_handle)
		{
			FarSettingsItem item{0};
            item.StructSize = sizeof(FarSettingsItem);
			item.Name = name;
			item.Type = (sval) ? FST_STRING : FST_QWORD;
			if (sval)
				item.String = sval->c_str();
			if (ival)
				item.Number = *ival;
			ret = Info.SettingsControl(settings_handle, SCTL_SET, 0, &item) != FALSE;
		}
		return ret;
	}
	
	const wchar_t *GetModuleName() override
	{
		return Info.ModuleName;
	}

protected:
	friend CalcApi *CreateApiFar3(void *info);

	PluginStartupInfo Info;

	HANDLE settings_handle;

	std::wstring plugin_name;
};

/////////////////////////////////////////////////////////////////////////////////

CalcApi *CreateApiFar3(void *info)
{
	PluginStartupInfo *psi = (PluginStartupInfo *)info;

	if (psi == nullptr || psi->StructSize < sizeof(PluginStartupInfo) ||			// far container is too old
		psi->AdvControl == nullptr || psi->FSF == nullptr)
		return nullptr;

	// check far version directly
	VersionInfo ver{};
	int ret = 0;
	try
	{
		ret = (int)psi->AdvControl(&MainGuid, ACTL_GETFARMANAGERVERSION, 0, &ver);
	}
	catch (...)
	{
		return nullptr;
	}
	
	if (ret != 1 || ver.Major < 3 || ver.Major > 10)
		return nullptr;

	CalcApiFar3 *api = new CalcApiFar3();

	if (api == nullptr)
		return nullptr;

	memset(&api->Info, 0, sizeof(api->Info));
	memmove(&api->Info, psi, (psi->StructSize > sizeof(api->Info)) ? sizeof(api->Info) : psi->StructSize);

	return api;
}

void GetGlobalInfoFar3(void *ginfo, const wchar_t *name)
{
	GlobalInfo *gInfo = (GlobalInfo *)ginfo;
	gInfo->StructSize = sizeof(GlobalInfo);
	gInfo->MinFarVersion = MAKEFARVERSION(3, 0, 0, 2927, VS_RELEASE);
	gInfo->Version = MAKEFARVERSION(PLUGIN_VER_MAJOR, PLUGIN_VER_MINOR, PLUGIN_VER_PATCH, 0, VS_RELEASE);
	gInfo->Guid = MainGuid;
	gInfo->Title = name;
	gInfo->Description = PLUGIN_DESC;
	gInfo->Author = L"uncle-vunkis";
}

