//
//  Copyright (c) uncle-vunkis 2009-2012 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include <math.h>
#include <string>
#include <initguid.h>

#include "api.h"
#include "calc.h"

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
static CalcDialog::CalcDialogCallback *msg_table = NULL;

/////////////////////////////////////////////////////////////////////////////////

class CalcDialogFuncsFar3 : public CalcDialogFuncs
{
public:
	CalcDialogFuncsFar3(PluginStartupInfo *Info)
	{
		this->Info = Info;
	}

	virtual void EnableRedraw(DLGHANDLE hdlg, bool en)
	{
		Info->SendDlgMessage(hdlg, DM_ENABLEREDRAW, en ? TRUE : FALSE, 0);
	}
	virtual void ResizeDialog(DLGHANDLE hdlg, const CalcCoord & dims)
	{
		Info->SendDlgMessage(hdlg, DM_RESIZEDIALOG, 0, (void *)&dims);
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
		Info->SendDlgMessage(hdlg, DM_GETDLGRECT, 0, (void *)rect);
	}

	virtual void GetDlgItemShort(DLGHANDLE hdlg, int id, CalcDialogItem *item)
	{
		FarDialogItem fdi;
		Info->SendDlgMessage(hdlg, DM_GETDLGITEMSHORT, id, (void *)&fdi);
		ConvertToDlgItem((void *)&fdi, item);
	}
	virtual void SetDlgItemShort(DLGHANDLE hdlg, int id, const CalcDialogItem & item)
	{
		FarDialogItem fdi;
		ConvertFromDlgItem(&item, (void *)&fdi);
		Info->SendDlgMessage(hdlg, DM_SETDLGITEMSHORT, id, (void *)&fdi);
	}
	virtual void SetItemPosition(DLGHANDLE hdlg, int id, const CalcRect & rect)
	{
		Info->SendDlgMessage(hdlg, DM_SETITEMPOSITION, id, (void *)&rect);
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
		Info->SendDlgMessage(hdlg, DN_EDITCHANGE, id, (void *)&fdi);
	}
	virtual void SetSelection(DLGHANDLE hdlg, int id, const CalcEditorSelect & sel)
	{
		Info->SendDlgMessage(hdlg, DM_SETSELECTION, id, (void *)&sel);
	}
	virtual void SetCursorPos(DLGHANDLE hdlg, int id, const CalcCoord & pos)
	{
		Info->SendDlgMessage(hdlg, DM_SETCURSORPOS, id, (void *)&pos);
	}
	virtual void GetText(DLGHANDLE hdlg, int id, std::wstring &str)
	{
		FarDialogItemData item = { sizeof(FarDialogItemData) };
		item.PtrLength = Info->SendDlgMessage(hdlg, DM_GETTEXT, id, 0);
		str.clear();
		str.resize(item.PtrLength + 1);
		item.PtrData = (wchar_t *)str.data();
		Info->SendDlgMessage(hdlg, DM_GETTEXT, id, &item);
	}
	virtual void SetText(DLGHANDLE hdlg, int id, const std::wstring & str)
	{
		Info->SendDlgMessage(hdlg, DM_SETTEXTPTR, id, (void *)str.c_str());
	}
	virtual void AddHistory(DLGHANDLE hdlg, int id, const std::wstring & str)
	{
		Info->SendDlgMessage(hdlg, DM_ADDHISTORY, id, (void *)str.c_str());
	}
	virtual bool  IsChecked(DLGHANDLE hdlg, int id)
	{
		return ((int)Info->SendDlgMessage(hdlg, DM_GETCHECK, id, 0) == BSTATE_CHECKED);
	}

	////////////////////////////////////////////////////////////////////////////////////

	virtual DLGHANDLE DialogInit(int id, int X1, int Y1, int X2, int Y2, const wchar_t *HelpTopic, 
								struct CalcDialogItem *Item, unsigned int ItemsNumber,
								CALCDLGPROC dlgProc)
	{
		if (id >= sizeof(dlg_guids) / sizeof(dlg_guids[0]))
			return NULL;

		FarDialogItem *faritems = (FarDialogItem *)malloc(ItemsNumber * sizeof(FarDialogItem));
		for (unsigned i = 0; i < ItemsNumber; i++)
		{
			ConvertFromDlgItem(&Item[i], (void *)&faritems[i]);
		}
		
		return Info->DialogInit(&MainGuid, &dlg_guids[id], X1, Y1, X2, Y2, HelpTopic, faritems, ItemsNumber, 0, 0, 
			(FARWINDOWPROC)dlgProc, NULL);
	}

	virtual intptr_t DialogRun(DLGHANDLE hdlg)
	{
		return Info->DialogRun(hdlg);
	}

	virtual void DialogFree(DLGHANDLE hdlg)
	{
		Info->DialogFree(hdlg);
	}

	virtual CALC_INT_PTR DefDlgProc(DLGHANDLE hdlg, int msg, int param1, void *param2)
	{
		return Info->DefDlgProc(hdlg, msg, param1, (void *)param2);
	}
	virtual void ConvertToDlgItem(void *faritem, CalcDialogItem *calcitem)
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
	virtual void ConvertFromDlgItem(const CalcDialogItem *calcitem, void *faritem)
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
		fi->History = (calcitem->Flags & CALC_DIF_HISTORY) ? L"calc" : NULL;
		fi->Mask = NULL;
		fi->UserData = NULL;
		fi->MaxLength = 0;
	}

	virtual CALC_INT_PTR PreProcessMessage(DLGHANDLE hdlg, int msg, int &param1, void * &param2)
	{
		if (msg == DN_EDITCHANGE)
		{
			CalcDialogItem *Item = new CalcDialogItem();
			ConvertToDlgItem(param2, Item);
			param2 = (void *)Item;
		}
		return 0;
	}

	virtual CALC_INT_PTR PostProcessMessage(DLGHANDLE hdlg, int msg, CALC_INT_PTR &ret, int &param1, void * &param2)
	{
		if (msg == DN_EDITCHANGE)
		{
			delete param2;
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
			msg_table[DN_CONTROLINPUT]    = &CalcDialog::OnInput;
			msg_table[DN_CTLCOLORDLGITEM] = &CalcDialog::OnCtrlColorDlgItem;
		}
		return msg_table;
	}

protected:
	PluginStartupInfo *Info;
};

static CalcDialogFuncsFar3 *dlg_funcs = NULL;

///////////////////////////////////////////////////////////////////////////////////

class CalcApiFar3 : public CalcApi
{
public:
	virtual void GetPluginInfo(void *pinfo, const wchar_t *name)
	{
		struct PluginInfo *pInfo = (struct PluginInfo *)pinfo;
		pInfo->StructSize = sizeof(*pInfo);
		pInfo->Flags = PF_EDITOR | PF_VIEWER;

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

	virtual bool IsOpenedFromEditor(void *oinfo, int OpenFrom)
	{
		struct OpenInfo *oInfo = (struct OpenInfo *)oinfo;
		return (oInfo->OpenFrom == OPEN_EDITOR);
	}

	virtual const wchar_t *GetMsg(int MsgId)
	{
		return Info.GetMsg(&MainGuid, MsgId);
	}

	virtual int GetMinVersion(void *ver)
	{
		if (ver)
		{
			struct VersionInfo *vi = (struct VersionInfo *)ver;
			*vi = MAKEFARVERSION(2, 0, 0, 994, VS_RELEASE);
		}
		return 0;
	}

	virtual CalcDialogFuncs *GetDlgFuncs()
	{
		if (!dlg_funcs)
			dlg_funcs = new CalcDialogFuncsFar3(&Info);
		return dlg_funcs;
	}

	virtual intptr_t Message(unsigned long Flags, const wchar_t *HelpTopic, const wchar_t * const *Items,
						int ItemsNumber, int ButtonsNumber)
	{
		return Info.Message(&MainGuid, &MsgGuid, Flags, HelpTopic, Items, ItemsNumber, ButtonsNumber);
	}

	virtual intptr_t Menu(int X, int Y, int MaxHeight, unsigned long long Flags,
		const wchar_t *Title, const wchar_t *HelpTopic, 
		const std::vector<CalcMenuItem> & Items)
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

		return Info.Menu(&MainGuid, &MenuGuid, X, Y, MaxHeight, flags, Title, NULL, HelpTopic,
							NULL, NULL, items, Items.size());
	}

	virtual void EditorGet(CalcEditorGetString *string, CalcEditorInfo *info)
	{
		Info.EditorControl(-1, ECTL_GETSTRING, 0, string);
		Info.EditorControl(-1, ECTL_GETINFO, 0, info);
	}

	virtual void EditorSelect(const CalcEditorSelect & sel)
	{
		Info.EditorControl(-1, ECTL_SELECT, 0, (void *)&sel);
	}

	virtual void EditorInsert(const wchar_t *text)
	{
		Info.EditorControl(-1, ECTL_INSERTTEXT, 0, (void *)text);
	}

	virtual void EditorRedraw()
	{
		Info.EditorControl(-1, ECTL_REDRAW, 0, 0);
	}
	
	virtual void GetDlgColors(CalcColor *edit_color, CalcColor *sel_color, CalcColor *highlight_color)
	{
		Info.AdvControl(&MainGuid, ACTL_GETCOLOR, COL_DIALOGEDIT, (void *)edit_color);
		Info.AdvControl(&MainGuid, ACTL_GETCOLOR, COL_DIALOGEDITSELECTED, (void *)sel_color);
		Info.AdvControl(&MainGuid, ACTL_GETCOLOR, COL_DIALOGHIGHLIGHTTEXT, (void *)highlight_color);
	}

	virtual int GetCmdLine(std::wstring &cmd)
	{
		int len = (int)Info.PanelControl(INVALID_HANDLE_VALUE, FCTL_GETCMDLINE, 0, 0);
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

	virtual void SetCmdLine(const std::wstring & cmd)
	{
		Info.PanelControl(INVALID_HANDLE_VALUE, FCTL_SETCMDLINE, (int)cmd.size(), (void *)cmd.data());
	}


	virtual bool SettingsBegin()
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
			settings_handle = NULL;
			return false;
		}
	}
	
	virtual bool SettingsEnd()
	{
		Info.SettingsControl(settings_handle, SCTL_FREE, 0, 0);
		return true;
	}

	virtual bool SettingsGet(const wchar_t *name, std::wstring *sval, unsigned long *ival)
	{
		bool ret = false;
		if (settings_handle)
		{
			FarSettingsItem item = {0};
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

	virtual bool SettingsSet(const wchar_t *name, const std::wstring *sval, const unsigned long *ival)
	{
		bool ret = false;
		if (settings_handle)
		{
			FarSettingsItem item = {0};
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
	
	virtual const wchar_t *GetModuleName()
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

	if (psi == NULL || psi->StructSize < sizeof(PluginStartupInfo) ||			// far container is too old
		psi->AdvControl == NULL || psi->FSF == NULL)
		return NULL;

	// check far version directly
	VersionInfo ver;
	int ret = 0;
	try
	{
		ret = (int)psi->AdvControl(&MainGuid, ACTL_GETFARMANAGERVERSION, 0, &ver);
	}
	catch (...)
	{
		return NULL;
	}
	
	if (ret != 1 || ver.Major < 3 || ver.Major > 10)
		return NULL;

	CalcApiFar3 *api = new CalcApiFar3();

	if (api == NULL)
		return NULL;

	memset(&api->Info, 0, sizeof(api->Info));
	memmove(&api->Info, psi, (psi->StructSize > sizeof(api->Info)) ? sizeof(api->Info) : psi->StructSize);

	return api;
}

void GetGlobalInfoFar3(void *ginfo, const wchar_t *name)
{
	GlobalInfo *gInfo = (GlobalInfo *)ginfo;
	gInfo->StructSize = sizeof(GlobalInfo);
	gInfo->MinFarVersion = MAKEFARVERSION(3, 0, 0, 2927, VS_RELEASE);
	gInfo->Version = MAKEFARVERSION(3, 23, 0, 0, VS_RELEASE);
	gInfo->Guid = MainGuid;
	gInfo->Title = name;
	gInfo->Description = name;
	gInfo->Author = L"uncle-vunkis";
}

