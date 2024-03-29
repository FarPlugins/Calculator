//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2012 <uncle-vunkis@yandex.ru>

//  XXX: Modified version! Added support for FAR2/UNICODE, BigNumbers etc.

//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <windows.h>

#include <sgml/sgml.h>

#include "api.h"

#include "newparse.h"
#include "calc.h"
#include "config.h"
#include "messages.h"

bool badFar = true;

wchar_t *coreReturn = nullptr;

CalcDialogItem *cur_dlg_items = nullptr;
int cur_dlg_items_num = 0, cur_dlg_id = 0, cur_dlg_items_numedits = 0;
int cur_dlg_need_recalc = 0;
CalcCoord cur_dlg_dlg_size;

// XXX:
int calc_cx = 55, calc_cy = 8, calc_edit_length = calc_cx - 23;
int cx_column_width[10] = { 0 };
int start_from_line = 5;

static struct Addons
{
	unsigned num_custom;
	int max_len;
	int radio_id1, radio_id2;
	int edit_id1, edit_id2;
} addons_info;

const CalcDialogItem dialog_template[] = 
{
	{ CALC_DI_DOUBLEBOX,    3,  1, 52, 11, 0, 0, L"" },
	{ CALC_DI_TEXT,         5,  3,  0,  0, 0, 0, L"&expr:" },
	{ CALC_DI_EDIT,        10,  3, 49,  0, (intptr_t)L"calc_expr", CALC_DIF_FOCUS | CALC_DIF_HISTORY, L"" },
	{ CALC_DI_TEXT,         5,  4,  0,  0, 0, 0, L"" },
}; //type,x1,y1,x2,y2,Sel,Flags,Data

static const int CALC_EDIT_ID = 2, CALC_TYPE_ID = 3;

static const wchar_t types[][20] = 
{
	L"type:big number", 
	L"type:int64", L"type:uint64", L"type:int ", L"type:short ", L"type:char ",
	L"type:uint ", L"type:ushort", L"type:byte", L"type:double", L"type:float" 
};


CalcParser *parser = nullptr;

int calc_error = 0;
int curRadio = 4;

//////////////////////////////////////////////////////////////
// FAR exports
void CalcStartup()
{
	badFar = false;
	srand((unsigned)time(nullptr));
	InitConfig();
}

bool CalcOpen(bool editor)
{
	if (badFar)
	{
		// XXX:
		const wchar_t *errver[] = {L"Error", L"Bad FAR version. Need >= Far 2.00", L"die" };
		api->Message(0, nullptr, &errver[0], 3, 1);
		return false;
	}
	InitDynamicData();
	if (editor)
		EditorDialog();
	else
		ShellDialog();
	DeInitDynamicData();
	return true;
}

bool CalcConfig()
{
	LoadConfig();
	CheckConfig();
	SaveConfig();

	if (ConfigDialog())
	{
		CheckConfig();
		SaveConfig();
		return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////
// loading settings
void InitDynamicData()
{
	srand((unsigned)time(nullptr));

	LoadConfig();
	CheckConfig();
	SaveConfig();
	
	CalcParser::SetDelims(props.decimal_point[0], props.args[0], props.use_delim ? props.digit_delim[0] : 0);

	CalcParser::InitTables(props.rep_fraction_max_start, props.rep_fraction_max_period, props.cont_fraction_max);
	
	PSgmlEl BaseRc = new CSgmlEl;
	BaseRc->parse(api->GetModuleName(), L"calcset.csr");
	
	CalcParser::ProcessData(BaseRc, props.case_sensitive != 0);
	CalcParser::AddAll();
	
	delete BaseRc;

	memset(&addons_info, 0, sizeof(addons_info));

	delete parser;
	parser = new CalcParser();

	CalcParser::ProcessAddons();
	CalcParser::ProcessDialogData();

}

void DeInitDynamicData()
{
	if (parser != nullptr)
	{
		delete parser;
		parser = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
// editor dialog
void EditorDialog()
{
	wchar_t *Text = nullptr;
	int s, e, i;
	bool After = true, skip_eq = false;
	int num_fmi = 1 + CalcParser::main_addons_num;
	std::vector<CalcMenuItem> fmi(num_fmi);
	fmi[0].Text = api->GetMsg(mMenuName);
	fmi[0].Selected = 1;
	for (i = 0; i < num_fmi - 1; i++)
	{
		fmi[i + 1].Text = CalcParser::addons[i].name.c_str();
	}

	i = (int)api->Menu(-1, -1, num_fmi, CALC_FMENU_WRAPMODE, L"action", nullptr, fmi);
	
	if (i == -1) 
		return;
	if (i == 0)
	{
		CalcShowDialog();
		if (coreReturn) 
		{
			api->EditorInsert(coreReturn);
			free(coreReturn);
			coreReturn = nullptr;
		}
		return;
	}

	CalcEditorGetString EditStr = { sizeof(CalcEditorGetString) };
	CalcEditorInfo EditInfo = { sizeof(CalcEditorInfo) };
	EditStr.StringNumber = -1;
	api->EditorGet(&EditStr, &EditInfo);

	for (e = (int)EditInfo.CurPos - 1; e > 0; e--)
		if (EditStr.StringText[e] != ' ') 
			break;
	for (s = e; s > 0; s--)
		if (*PWORD(EditStr.StringText + s) == 0x2020) 
			break;
	
	if (EditInfo.BlockStartLine == EditInfo.CurLine &&
			EditStr.SelStart != -1 && EditStr.SelEnd != -1)
	{
		// selection
		s = (int)EditStr.SelStart;
		e = (int)EditStr.SelEnd - 1;
		CalcEditorSelect EditSel = { sizeof(CalcEditorSelect) };
		EditSel.BlockType = CALC_BTYPE_NONE;
		api->EditorSelect(EditSel);
		After = (int)EditInfo.CurPos >= (s + e)/2;
	}

	Text = (wchar_t *)malloc((e - s + 2) * sizeof(wchar_t));
	if (Text)
	{
		MoveMemory(Text, EditStr.StringText + s, (e - s + 1) * sizeof(wchar_t));
		Text[e - s + 1] = 0;

		// if the last char is '='
		if (Text[e - s] == '=')
		{
			Text[e - s] = 0;
			skip_eq = true;
		}

		SArg Res = parser->Parse(Text, props.case_sensitive != 0);
		free(Text);
  
		Text = convertToString(Res, i - 1, 0, false, props.pad_zeroes != 0, false, nullptr);
		if (Text)
		{
			if (After && !skip_eq) 
				api->EditorInsert(L"=");
			if (!parser->GetError()) 
				api->EditorInsert(Text);
			if (!After && !skip_eq) 
				api->EditorInsert(L"=");

			EditStr.StringNumber = -1;
			api->EditorGet(&EditStr, &EditInfo);
			CalcEditorSelect EditSel = { sizeof(CalcEditorSelect) };
			EditSel.BlockType = CALC_BTYPE_STREAM;
			EditSel.BlockStartLine = EditInfo.CurLine;
			EditSel.BlockStartPos  = EditInfo.CurPos - (int)wcslen(Text) - !After;
			EditSel.BlockWidth = (int)wcslen(Text);
			EditSel.BlockHeight = 1;
			api->EditorSelect(EditSel);
			free(Text);
		}
	}

	api->EditorRedraw();
}

//////////////////////////////////////////////////////////////////////////
// shell dialog
void ShellDialog()
{
	int i = 0;
	if (CalcParser::GetNumDialogs() > 0)
	{
		while(i = CalcMenu(i))
		{
			if (i == -1) 
				return;
			ShowUnitsDialog(i);
		}
	}
	
	CalcShowDialog();
	if (coreReturn)
	{
		std::wstring cmd;
		if (api->GetCmdLine(cmd))
		{
			cmd += coreReturn;
			api->SetCmdLine(cmd);
		}
		free(coreReturn);
		coreReturn = nullptr;
	}
}

int CalcMenu(int c)
{
	unsigned i;
	int ret;
	
	std::vector<CalcMenuItem> MenuEls(CalcParser::DialogsNum + 1);
	MenuEls[0].Text = _wcsdup(api->GetMsg(mName));
	
	PDialogData dd;
	for(i = 1, dd = CalcParser::DialogData; dd; dd = dd->Next, i++)
		MenuEls[i].Text = _wcsdup(dd->Name);
	
	MenuEls[c].Selected = TRUE;
	
	ret = (int)api->Menu(-1, -1, 0, CALC_FMENU_WRAPMODE | CALC_FMENU_AUTOHIGHLIGHT,
					api->GetMsg(mDialogs), L"Contents", MenuEls);
	
	for(i = 0; i < MenuEls.size(); i++)
		free((void *)MenuEls[i].Text);

	return ret;
}

/////////////////////////////////////////////////////////////////////

void getConsoleWindowSize(int *sX, int *sY)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	*sX = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	*sY = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void SetUnitsDialogDims()
{
	int i, j, d, cx, cy;
	PDialogData dd = CalcParser::DialogData;
	PDialogElem de, de1;

	CalcDialogItem *dialog = cur_dlg_items;

	for (i = 0; i < cur_dlg_id-1; i++)
		dd = dd->Next;
	de = dd->Elem;
	
	int sX, sY;
	getConsoleWindowSize(&sX, &sY);
	cx = sX - 2;
	cy = sY - 1;
	j = sY - 5;
	
	if (!((dd->num + 2) / j))
	{
		cy = dd->num + 4;
		cx = sX * 2 / 3;
	}
	
	dialog[0].X1 = 3;
	dialog[0].Y1 = 1;
	dialog[0].X2 = cx - 3;
	dialog[0].Y2 = cy - 2;
	
	cur_dlg_dlg_size.X = cx;
	cur_dlg_dlg_size.Y = cy;
	
	cur_dlg_items = dialog;
	
	int col_width = (cx - 9) / (dd->num / (j + 1) + 1);
	for (int i = 0; i < 10; i++)
		cx_column_width[i] = 0;
	
	int oldk = -1, col_textlen = 0;
	for (d = 1, i = 0, de1 = de;  i < dd->num;  i++,d++, de1 = de1->Next)
	{
		int k = 5 + (i / j) * (cx - 9) / (dd->num / (j + 1) + 1);
		if (k != oldk)
		{
			// find max text length for this column
			PDialogElem de2 = de1;
			col_textlen = 0;
			for (int ii = i; de2 && ii < i + j; ii++, de2 = de2->Next)
			{
				if (de2->Type != 0)
				{
					int l = (int)wcslen(de2->Name);
					if (l > col_textlen)
						col_textlen = l;
				}
			}
			oldk = k;
		}
		de1->column_idx = i/j;
		if (!de1->Type)
		{
			dialog[d].X1 = k;
			dialog[d].Y1 = i%j + 2;
		} else 
		{
			int cl = (col_width/2 > col_textlen+2) ? col_textlen+2 : col_width/2;
			dialog[d].X1 = k + 1;
			dialog[d].Y1 = i%j + 2;
			dialog[d].X2 = dialog[d].X1 + cl;
			d++;
			dialog[d].X1 = k + cl;
			dialog[d].Y1 = i%j + 2;
			int w = col_width/2 + (col_width/2 - cl);
			dialog[d].X2 = dialog[d].X1 + w - 2;
			if (cx_column_width[i/j] < w)
				cx_column_width[i/j] = w;
		}
	}
	
}

class DlgUnits : public CalcDialog
{
public:
	PDialogData dd;
	bool inside_setdlgmessage;

	DlgUnits(PDialogData d)
	{
		inside_setdlgmessage = false;
		dd = d;
	}

	virtual ~DlgUnits()
	{
	}

	virtual CALC_INT_PTR OnInitDialog(int param1, void *param2)
	{
		if (coreReturn)
		{
			free(coreReturn);
			coreReturn = nullptr;
		}
		return TRUE;
	}

	virtual CALC_INT_PTR OnResizeConsole(int param1, void *param2)
	{
		SetUnitsDialogDims();
		EnableRedraw(false);
		ResizeDialog(cur_dlg_dlg_size);
		for (int i = 0; i < cur_dlg_items_num; i++)
		{
			CalcRect rect{};
			rect.Left = cur_dlg_items[i].X1;
			rect.Top = cur_dlg_items[i].Y1;
			rect.Right = cur_dlg_items[i].X2;
			rect.Bottom = cur_dlg_items[i].Y2;
			SetItemPosition(i, rect);
		}
		EnableRedraw(true);
		
		int curid = GetFocus();
		if (curid >= 0)
		{
			std::wstring str;
			GetText(curid, str);
			SetText(curid, str);
		}
		return TRUE;
	}

	virtual CALC_INT_PTR OnGotFocus(int param1, void *param2)
	{
		std::wstring str;
		GetText(param1, str);
		int id = 1;
		PDialogElem de;
		for (de = dd->Elem;  de;  de = de->Next, id++)
		{
			if (de->Type)
			{
				id++;
				CalcEditorSelect EditSel = { sizeof(CalcEditorSelect) };
				EditSel.BlockType = CALC_BTYPE_STREAM;
				EditSel.BlockStartLine = -1;
				EditSel.BlockHeight = 1;
				EditSel.BlockStartPos = 0;
				EditSel.BlockWidth = (id == param1) ? (int)str.size() : 0;
				SetSelection(id, EditSel);
			}
		}
		return TRUE;
	}

	virtual CALC_INT_PTR OnEditChange(int param1, void *param2)
	{
		if (inside_setdlgmessage)
			return FALSE;

		if (!props.auto_update)
			return FALSE;
		return OnEditChangeInternal(param1, param2);
	}

	virtual CALC_INT_PTR OnEditChangeInternal(int param1, void *param2)
	{
		CalcDialogItem *Item = (CalcDialogItem *)param2;
		Big val = 0;

        EnableRedraw(false);
		bool was_error = false;
		SArg res = parser->Parse(Item->PtrData, props.case_sensitive != 0);
		if (parser->GetError())
			was_error = true;
        
		int id = 1;
		PDialogElem de;
		for (de = dd->Elem;  de;  de = de->Next, id++)
		{
			if (de->Type) id++;
			if (id == param1) 
			{
				if (de->input != nullptr && de->input->parser != nullptr)
				{
					SArg args[2];
					args[0] = res;
					try
					{
						val = de->input->parser->eval(args).GetBig();
					}
					catch(CALC_ERROR)
					{
						was_error = true;
					}
				} else
					val = res.GetBig() * de->scale;
				break;
			}
		}
		
		id = 1;
		for (de = dd->Elem;  de;  de = de->Next, id++)
		{
			if (de->Type)
			{
				id++;
				std::wstring str;
				if (!was_error)
				{
					int idx = CALC_CONV_UNITS;
					Big tmp;
					if (de->addon_idx >= 0)
					{
						idx = de->addon_idx;
						tmp = val;
					}
					else
					{
						tmp = de->scale != 0 ? val / de->scale : 0;
					}
					
					CALC_ERROR errcode = ERR_OK;
					wchar_t *ret = convertToString(tmp, idx, cx_column_width[de->column_idx] - 4, false, props.pad_zeroes != 0, true, &errcode);
					if (errcode != ERR_OK)
						str.clear();
					else
						str = ret;
				}
        
				if (id != param1)
				{
					//XXX:
					inside_setdlgmessage = true;
					SetText(id, str);
					CalcCoord coord{};
					coord.X = 0;
					SetCursorPos(id, coord);
					inside_setdlgmessage = false;
				}
			}
		}
        EnableRedraw(true);
		return TRUE;
	}
  
	virtual CALC_INT_PTR OnInput(int param1, void *param2)
	{
		INPUT_RECORD *ir = (INPUT_RECORD *)param2;
		if (ir->EventType != KEY_EVENT || !ir->Event.KeyEvent.bKeyDown)
			return -1;
		if (ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
		{
			std::wstring str;
			GetText(param1, str);
			if ((ir->Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) ||
				(ir->Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED))
			{
				free(coreReturn);
				coreReturn = _wcsdup(str.c_str());
				Close(CALC_EDIT_ID);
				return TRUE;
			}
  
			SArg res = parser->Parse(str.c_str(), props.case_sensitive != 0);

			str = convertToString(res, CALC_CONV_ENTER, props.result_length, false, props.pad_zeroes != 0, false, nullptr);

			if (!props.auto_update)
			{
				CalcDialogItem Item{};
				Item.PtrData = str.c_str();
				OnEditChangeInternal(param1, (void *)&Item);
			}

			SetText(param1, str);
			OnGotFocus(param1, nullptr);

			return TRUE;
		}
		return -1;
	}

	virtual CALC_INT_PTR OnCtrlColorDlgItem(int param1, void *param2)
	{
		CalcDialogItem item{};
		GetDlgItemShort(param1, &item);
		if (item.Flags & CALC_DIF_BOXCOLOR)
		{
			CalcDialogItemColors *icolors = (CalcDialogItemColors *)param2;
			CalcColor clrs[3];
			api->GetDlgColors(&clrs[0], &clrs[1], &clrs[2]);
			icolors->Colors[0] = clrs[2];
			icolors->Colors[2] = clrs[2];
		}
		return -1;
	}
};

void ShowUnitsDialog(int no)
{
	int i, d;
	PDialogData dd = CalcParser::DialogData;
	PDialogElem de, de1;
	
	cur_dlg_id = no;
	for (i = 0; i < no-1; i++)
		dd = dd->Next;
	de = dd->Elem;

	int dsize = 1+dd->num*2;
	
	CalcDialogItem *dialog = new CalcDialogItem [dsize];
	memset(dialog, 0, sizeof(CalcDialogItem) * dsize);
	dialog[0].Type = CALC_DI_DOUBLEBOX;
	
	// XXX:
	dialog[0].PtrData = _wcsdup(dd->Name);

	for (d = 1, i = 0, de1 = de;  i < dd->num;  i++,d++, de1 = de1->Next)
	{
		if (!de1->Type)
		{
			dialog[d].Type = CALC_DI_TEXT;
			dialog[d].Flags = CALC_DIF_BOXCOLOR;	// used to set highlight colors
			dialog[d].PtrData = _wcsdup(de1->Name);
		} else 
		{
			dialog[d].Type = CALC_DI_TEXT;
			dialog[d].PtrData = _wcsdup(de1->Name);
			d++;
			dialog[d].Type = CALC_DI_EDIT;
		}
	}

	cur_dlg_items = dialog;
	cur_dlg_items_num = dsize;
	SetUnitsDialogDims();
	
	DlgUnits units(dd);
	units.Init(CALC_DIALOG_UNITS, -1, -1, cur_dlg_dlg_size.X, cur_dlg_dlg_size.Y, L"Contents", dialog, dsize);
	units.Run();

	cur_dlg_items = nullptr;

	for (i = 0;  i < dsize; i++)
  		free((void *)dialog[i].PtrData);
	delete [] dialog;
}

//////////////////////////////////////////////////////////////////////////////////////


void SetCalcDialogDims(CalcDialogItem *dialog)
{
	int sX, sY;
	getConsoleWindowSize(&sX, &sY);
	
	int basenum = sizeof(dialog_template)/sizeof(dialog_template[0]);
	
	calc_cx = sX > 80 ? 55 * sX / 80 : 55;
	if (calc_cx > 90)
		calc_cx = 90;
	
	calc_edit_length = calc_cx - 20 - addons_info.max_len;
	
	calc_cy = 8 + addons_info.num_custom;
	
	dialog[0].X2 = calc_cx - 3;
	dialog[0].Y2 = calc_cy - 2;
	dialog[CALC_EDIT_ID].X2 = calc_cx - 6;
	
	cur_dlg_dlg_size.X = calc_cx;
	cur_dlg_dlg_size.Y = calc_cy;
	
	for (int i = 0; i < cur_dlg_items_numedits; i++)
	{
		dialog[basenum + cur_dlg_items_numedits + i].X1 = 12 + addons_info.max_len;
		dialog[basenum + cur_dlg_items_numedits + i].X2 = calc_cx - 6;
		dialog[basenum + cur_dlg_items_numedits + i].Y1 = start_from_line + i;
	}
}

class DlgCalc : public CalcDialog
{
public:

	virtual CALC_INT_PTR OnInitDialog(int param1, void *param2)
	{
		if (coreReturn)
		{
			free(coreReturn);
			coreReturn = nullptr;
		}
		return FALSE;
	}

	virtual CALC_INT_PTR OnResizeConsole(int param1, void *param2)
	{
		SetCalcDialogDims(cur_dlg_items);
		EnableRedraw(false);
		ResizeDialog(cur_dlg_dlg_size);

		for (int i = 0; i < cur_dlg_items_num; i++)
		{
			CalcRect rect{};
			rect.Left = cur_dlg_items[i].X1;
			rect.Top = cur_dlg_items[i].Y1;
			rect.Right = cur_dlg_items[i].X2;
			rect.Bottom = cur_dlg_items[i].Y2;
			SetItemPosition(i, rect);
		}

		EnableRedraw(true);
		
		cur_dlg_need_recalc = 1;
		return -1;
	}

	virtual CALC_INT_PTR OnDrawDialog(int param1, void *param2)
	{
		if (cur_dlg_need_recalc)
		{
			cur_dlg_need_recalc = 0;
			std::wstring str;
			GetText(CALC_EDIT_ID, str);
			SetText(CALC_EDIT_ID, str);
		}
		return -1;
	}

	virtual CALC_INT_PTR OnButtonClick(int param1, void *param2)
	{
	    curRadio = param1;
		return -1;
	}
	
	virtual CALC_INT_PTR OnClose(int param1, void *param2)
	{
		std::wstring str;
		GetText(CALC_EDIT_ID, str);
		AddHistory(CALC_EDIT_ID, str);
		CalcRect sr{};
		GetDlgRect(&sr);
		//api->SettingsSet(L"calcX", nullptr, sr.Left);
		//api->SettingsSet(L"calcY", nullptr, sr.Top);
		return -1;
	}

	virtual CALC_INT_PTR OnInput(int param1, void *param2)
	{
		INPUT_RECORD *ir = (INPUT_RECORD *)param2;
		if (ir->EventType != KEY_EVENT || !ir->Event.KeyEvent.bKeyDown)
			return -1;
		if (ir->Event.KeyEvent.wVirtualKeyCode == VK_F2)
		{
			int i = CalcMenu(0);
			if (i && i != -1) ShowUnitsDialog(i);
			if (coreReturn)
			{
				std::wstring str;
				GetText(CALC_EDIT_ID, str);
				str += coreReturn;
				SetText(CALC_EDIT_ID, str);
				// to update item2
				CalcDialogItem item{};
				GetDlgItemShort(param1, &item);
				EditChange(CALC_EDIT_ID, item);
			}
		}
		if (ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
		{
			CalcDialogItem *force_update_item = nullptr;
			std::wstring str;
			GetText(CALC_EDIT_ID, str);

			if ((ir->Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) ||
					(ir->Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED))
			{
				free(coreReturn);
				coreReturn = _wcsdup(str.c_str());
				Close(CALC_EDIT_ID);
			}
				
			AddHistory(CALC_EDIT_ID, str);
			SArg res = parser->Parse(str.c_str(), props.case_sensitive != 0);
			if (!props.auto_update)
			{
				force_update_item = (CalcDialogItem *)malloc(sizeof(CalcDialogItem) + (str.size() + 1) * sizeof(wchar_t));
				if (force_update_item)
				{
					force_update_item->PtrData = (const wchar_t *)(force_update_item + 1);
					wcscpy((wchar_t *)force_update_item->PtrData, str.c_str());
				}
			}
			
			if (param1 >= addons_info.radio_id1 && param1 <= addons_info.radio_id2 && curRadio != param1)
			{
				CalcDialogItem fdi{};
				GetDlgItemShort(curRadio, &fdi);
				fdi.Selected = 0;
				SetDlgItemShort(curRadio, fdi);

				GetDlgItemShort(param1, &fdi);
				fdi.Selected = 1;
				SetDlgItemShort(param1, fdi);
				
				curRadio = param1;
			}
      
			int loc_Radio = curRadio;
			if (param1 >= addons_info.radio_id1 && param1 <= addons_info.radio_id2)
				loc_Radio = param1;
			loc_Radio -= addons_info.radio_id1;

			wchar_t *text = L"";
			if (loc_Radio >= 0)
			{
				text = convertToString(res, loc_Radio, props.result_length, true, props.pad_zeroes != 0, false, nullptr);
				if (text == nullptr)
				{
					loc_Radio = -1;
					text = L"";
				}
			}

			int set_sel = calc_error;
			int text_len = (int)wcslen(text);
			SetText(CALC_EDIT_ID, text);
			
			if (loc_Radio >= 0)
				free(text);

			if (!props.auto_update)
			{
				OnEditChangeInternal(CALC_EDIT_ID, force_update_item, true);
			} else
			{
				// to update item2
				CalcDialogItem item{};
				GetDlgItemShort(param1, &item);
				EditChange(CALC_EDIT_ID, item);
			}

			if (set_sel)
			{
				CalcEditorSelect EditSel = { sizeof(CalcEditorSelect) };
				EditSel.BlockType = CALC_BTYPE_STREAM;
				EditSel.BlockStartLine = -1;
				EditSel.BlockHeight = 1;
				EditSel.BlockStartPos = 0;
				EditSel.BlockWidth = text_len;
				SetSelection(CALC_EDIT_ID, EditSel);
			} else
			{
				CalcCoord cursor{};
				cursor.X = 0;
				cursor.Y = 0;
				SetCursorPos(CALC_EDIT_ID, cursor);
				cursor.X = text_len;
				SetCursorPos(CALC_EDIT_ID, cursor);
			}
			SetFocus(CALC_EDIT_ID);
			return TRUE;
		}
		return -1;
	}

	virtual CALC_INT_PTR OnEditChange(int param1, void *param2)
	{
		return OnEditChangeInternal(param1, param2, false);
	}
	
	CALC_INT_PTR OnEditChangeInternal(int param1, void *param2, bool force_update)
	{
		if (param1 == CALC_EDIT_ID)
		{
			CalcDialogItem *Item = (CalcDialogItem *)param2;

			if (!props.auto_update && !force_update)
			{
				// do nothing
			}
			else if (Item->PtrData[0] == '\0')
			{
				EnableRedraw(false);
				SetText(CALC_TYPE_ID, L"");
				for (int i = addons_info.edit_id1; i <= addons_info.edit_id2; i++)
					SetText(i, L"");
				EnableRedraw(true);
			} else
			{
				EnableRedraw(false);
				SArg res = parser->Parse(Item->PtrData, props.case_sensitive != 0);

				if (parser->GetError())
				{
					calc_error = 1;

					SetText(CALC_TYPE_ID, api->GetMsg(mNoError + parser->GetError()));
					for (int i = addons_info.edit_id1; i <= addons_info.edit_id2; i++)
						SetText(i, L"");
					
				} else
				{
					calc_error = 0;

					CalcCoord cursor{};
					cursor.X = 0;
					cursor.Y = 0;
					std::wstring tmp = types[res.gettype()];
					if (res.gettype() == SA_CHAR)
					{
						if ((char)res != 0)
						{
							tmp += L"'";
							tmp += (char)res;
							if ((char)res == '&')
								tmp += '&';
							tmp += L"'";
						}
					}
					tmp += L"        ";
					SetText(CALC_TYPE_ID, tmp);
		      
					for (unsigned i = 0; i < parser->main_addons_num; i++)
					{
						CALC_ERROR error_code = ERR_OK;
						wchar_t *text = convertToString(res, i, 0, false, props.pad_zeroes != 0, true, &error_code);
						if (error_code != ERR_OK)
						{
							SetText(addons_info.edit_id1 + i, api->GetMsg(mNoError + error_code));
							SetCursorPos(addons_info.edit_id1 + i, cursor);
							
						}
						if (text)
						{
							SetText(addons_info.edit_id1 + i, text);
							SetCursorPos(addons_info.edit_id1 + i, cursor);
							free(text);
						}
					}
				}
				EnableRedraw(true);
			}

			if (force_update)
				free(Item);
		}

		return TRUE;
	}
	
	virtual CALC_INT_PTR OnCtrlColorDlgItem(int param1, void *param2)
	{
		if (param1 >= addons_info.edit_id1 && param1 <= addons_info.edit_id2)
		{
			CalcDialogItemColors *icolors = (CalcDialogItemColors *)param2;
			CalcColor clrs[3];
			api->GetDlgColors(&clrs[0], &clrs[1], &clrs[2]);
			icolors->Colors[0] = clrs[0];
			return 0;
		}
		return -1;
	}
};

// XXX:
int get_visible_len(const wchar_t *str) 
{
	int len = 0;
	for (int k = 0; str[k]; k++)
	{
		if (str[k] == '&' && str[k+1] == '&')
		{
			len++;
			k++;
		}
		else if (str[k] != '&')
			len++;
	}
	return len;
}

// XXX:
void CalcShowDialog()
{
	CalcDialogItem *dialog;

	unsigned basenum = sizeof(dialog_template)/sizeof(dialog_template[0]);
	unsigned totalnum = basenum;
	unsigned i;
	
	addons_info.num_custom = 0;
	addons_info.max_len = 0;

	for (addons_info.num_custom = 0; addons_info.num_custom < parser->main_addons_num; addons_info.num_custom++)
	{
		int len = get_visible_len(parser->addons[addons_info.num_custom].name.c_str());
		if (len > addons_info.max_len)
			addons_info.max_len = len;
	}

	totalnum = basenum + addons_info.num_custom * 2;
	dialog = new CalcDialogItem [totalnum];

	for (i = 0; i < basenum; i++)
		dialog[i] = dialog_template[i];
	dialog[0].PtrData = _wcsdup(api->GetMsg(mName));

	if (!props.autocomplete)
		dialog[2].Flags |= CALC_DIF_NOAUTOCOMPLETE;

	addons_info.radio_id1 = basenum;
	addons_info.radio_id2 = addons_info.radio_id1 + addons_info.num_custom - 1;
	addons_info.edit_id1 = addons_info.radio_id2 + 1;
	addons_info.edit_id2 = addons_info.edit_id1 + addons_info.num_custom - 1;

	// add radio-buttons
	for (i = 0; i < addons_info.num_custom; i++)
	{
		CalcDialogItem it{};
		memset(&it, 0, sizeof(CalcDialogItem));
		it.Type = CALC_DI_RADIOBUTTON;
		it.X1 = 6;
		it.Y1 = start_from_line + i;

		const wchar_t *from = parser->addons[i].name.c_str();
		wchar_t *tmp = (wchar_t *)malloc((wcslen(from) + addons_info.max_len + 2) * sizeof(wchar_t));
		if (tmp)
		{
			int pad_len = addons_info.max_len - get_visible_len(from);
			
			memset(tmp, ' ', pad_len * sizeof(wchar_t));	// guarantee that _wcsnset won't stop at '\0'
			_wcsnset(tmp, ' ', pad_len);
			
			wcscpy(tmp + pad_len, from);
			wcscat(tmp, L":");
		}
		
		it.PtrData = tmp;

		it.Flags = (i == 0) ? CALC_DIF_GROUP : 0;
		it.Selected = (curRadio == i + basenum) ? 1 : 0;
		
		dialog[basenum + i] = it;
	}

	// add edit-boxes
	for (i = 0; i < addons_info.num_custom; i++)
	{
		CalcDialogItem it{};
		memset(&it, 0, sizeof(CalcDialogItem));
		it.Type = CALC_DI_EDIT;
		it.PtrData = L"";
		it.Flags = CALC_DIF_DISABLE;
		
		dialog[basenum + addons_info.num_custom + i] = it;
	}

	cur_dlg_items = dialog;
	cur_dlg_items_num = totalnum;
	cur_dlg_items_numedits = addons_info.num_custom;
	SetCalcDialogDims(dialog);
	
	DlgCalc calc;
	calc.Init(CALC_DIALOG_MAIN, -1, -1, cur_dlg_dlg_size.X, cur_dlg_dlg_size.Y, 
				L"Contents", dialog, totalnum);

	calc.Run();

	free((void *)dialog[0].PtrData);

	for (i = 0; i < addons_info.num_custom; i++)
		free((void *)dialog[basenum + i].PtrData);

	delete [] dialog;
}

SDialogElem::SDialogElem()
{
	Next = nullptr;
	addon_idx = -1;
	input = nullptr;
	scale_expr = nullptr;
}

SDialogElem::~SDialogElem()
{
	delete Next;
	delete input;
	delete scale_expr;
}

SDialogData::SDialogData()
{
	Next = nullptr;
	Elem = nullptr;
}

SDialogData::~SDialogData()
{
	delete Elem;
	delete Next;
}
