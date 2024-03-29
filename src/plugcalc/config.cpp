//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2011 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include <cstdlib>
#include <windows.h>
#include <algorithm>
#include "api.h"
#include "newparse.h"
#include "calc.h"
#include "config.h"
#include "messages.h"

CalcProperties props;

enum CALC_PARAM_CHECK_TYPE
{
	CALC_PARAM_CHECK_BOOL,
	CALC_PARAM_CHECK_RADIO,
	CALC_PARAM_CHECK_LINES,
	CALC_PARAM_CHECK_PERIOD,
	CALC_PARAM_CHECK_DECIMAL,
	CALC_PARAM_CHECK_ARGS,
	CALC_PARAM_CHECK_DELIM,
	CALC_PARAM_CHECK_LENGTH,
	CALC_PARAM_CHECK_FRAC,
};

static const struct
{
	int *ival;
	wchar_t *sval;	// if sval != nullptr then (int)ival = max strlen

	const wchar_t *reg_name;
	const wchar_t *def_value;

	CALC_PARAM_CHECK_TYPE check_type;
} param_table[] =
{
	{ &props.auto_update, nullptr, L"autoUpdate", L"1", CALC_PARAM_CHECK_BOOL },
	{ &props.case_sensitive, nullptr, L"caseSensitive", L"0", CALC_PARAM_CHECK_BOOL },
	{ &props.pad_zeroes, nullptr, L"padZeroes", L"1", CALC_PARAM_CHECK_BOOL },
	/*{ &props.right_align, nullptr, L"rightAlign", L"0", CALC_PARAM_CHECK_BOOL },*/

	{ &props.history_hide, nullptr, L"historyHide", L"1", CALC_PARAM_CHECK_RADIO },
	{ &props.history_above, nullptr, L"historyAbove", L"0", CALC_PARAM_CHECK_RADIO },
	{ &props.history_below, nullptr, L"historyBelow", L"0", CALC_PARAM_CHECK_RADIO },
	{ &props.history_lines, nullptr, L"historyLines", L"8", CALC_PARAM_CHECK_LINES },
	{ &props.autocomplete, nullptr, L"autoComplete", L"0", CALC_PARAM_CHECK_BOOL },

	{ &props.use_regional, nullptr, L"useRegional", L"0", CALC_PARAM_CHECK_BOOL },
	{ (int *)sizeof(props.decimal_point), props.decimal_point, L"decimalPoint", L".", CALC_PARAM_CHECK_DECIMAL },
	{ (int *)sizeof(props.args), props.args, L"Args", L",", CALC_PARAM_CHECK_ARGS },
	{ &props.use_delim, nullptr, L"useDelim", L"0", CALC_PARAM_CHECK_BOOL },
	{ (int *)sizeof(props.digit_delim), props.digit_delim, L"digitDelim", L"'", CALC_PARAM_CHECK_DELIM },

	// registry-only
	{ &props.max_period, nullptr, L"maxPeriod", L"10", CALC_PARAM_CHECK_PERIOD },
	{ &props.result_length, nullptr, L"resultLength", L"128", CALC_PARAM_CHECK_LENGTH },
	{ &props.rep_fraction_max_start, nullptr, L"repFractionMaxStart", L"32", CALC_PARAM_CHECK_FRAC },
	{ &props.rep_fraction_max_period, nullptr, L"repFractionMaxPeriod", L"128", CALC_PARAM_CHECK_FRAC },
	{ &props.cont_fraction_max, nullptr, L"contFractionMax", L"64", CALC_PARAM_CHECK_FRAC },

	{ nullptr, nullptr, L"", nullptr, CALC_PARAM_CHECK_BOOL },
};

static const struct 
{
	CalcDialogItemTypes Type;
	int				    X1, Y1, X2, Y2;		// if Y1 == 0, then Y1=last_Y1;  if Y1 == -1, then Y1=last_Y1+1
	unsigned long long  Flags;
	int				    name_ID;
	int				    *prop_ivalue;
	wchar_t			    *prop_svalue;
} dlgItems[] = 
{
	{ CALC_DI_DOUBLEBOX,    3,  1, 54, 14, 0, mConfigName, nullptr, nullptr },
	{ CALC_DI_CHECKBOX,     5, -1,  0,  0, CALC_DIF_FOCUS, mConfigShowResults, &props.auto_update, nullptr },
	{ CALC_DI_CHECKBOX,     5, -1,  0,  0, 0, mConfigCaseSensitive, &props.case_sensitive, nullptr },
	
	{ CALC_DI_CHECKBOX,     5, -1,  0,  0, 0, mConfigPadZeroes, &props.pad_zeroes, nullptr },
#if 0
	{ CALC_DI_CHECKBOX,     5, -1,  0,  0, 0, mConfigRightAligned, &props.right_align, nullptr },

	{ CALC_DI_TEXT,        41,  0,  0,  0, CALC_DIF_DISABLE, mConfigMaxPeriod, nullptr, nullptr },
	{ CALC_DI_FIXEDIT,     53,  0, 55,  0, CALC_DIF_DISABLE, 0, &props.max_period, nullptr },
#endif

	{ CALC_DI_TEXT,         5, -1,  0,  0, CALC_DIF_BOXCOLOR|CALC_DIF_SEPARATOR, mConfigHistory, nullptr, nullptr },
	
	{ CALC_DI_RADIOBUTTON,  5, -1,  0,  0, CALC_DIF_GROUP, mConfigHide, &props.history_hide, nullptr },
	{ CALC_DI_RADIOBUTTON, 19,  0,  0,  0, CALC_DIF_DISABLE, mConfigShowAbove, &props.history_above, nullptr },
	{ CALC_DI_RADIOBUTTON, 37,  0,  0,  0, CALC_DIF_DISABLE, mConfigShowBelow, &props.history_below, nullptr },
	
	{ CALC_DI_FIXEDIT,      5, -1,  7,  0, CALC_DIF_DISABLE, 0, &props.history_lines, nullptr },
	{ CALC_DI_TEXT,         9,  0,  0,  0, CALC_DIF_DISABLE, mConfigLines, nullptr, nullptr },
	{ CALC_DI_CHECKBOX,    19,  0,  0,  0, 0, mConfigAutocomplete, &props.autocomplete, nullptr },

	{ CALC_DI_TEXT,         5, -1,  0,  0, CALC_DIF_BOXCOLOR|CALC_DIF_SEPARATOR, mConfigDelimiters, nullptr, nullptr },
	
	{ CALC_DI_TEXT,         5, -1,  0,  0, 0, mConfigDecimalSymbol, nullptr, nullptr },
	{ CALC_DI_FIXEDIT,     22,  0, 22,  0 , 0, 0, (int *)sizeof(props.decimal_point), props.decimal_point },
	{ CALC_DI_CHECKBOX,    25,  0,  0,  0, 0, mConfigDigitDelimiter, &props.use_delim, nullptr },
	{ CALC_DI_FIXEDIT,     51,  0, 51,  0, 0, 0, (int *)sizeof(props.digit_delim), props.digit_delim },

	{ CALC_DI_TEXT,         5, -1,  0,  0, 0, mConfigArguments, nullptr, nullptr },
	{ CALC_DI_FIXEDIT,     22,  0, 22,  0, 0, 0, (int *)sizeof(props.args), props.args },
	{ CALC_DI_CHECKBOX,    25,  0,  0,  0, 0, mConfigUseRegional, &props.use_regional, nullptr },
	
	{ CALC_DI_TEXT,         5, -1,  0,  0, CALC_DIF_BOXCOLOR|CALC_DIF_SEPARATOR, 0, nullptr, nullptr },
	
	{ CALC_DI_BUTTON,       0, -1,  0,  0, CALC_DIF_CENTERGROUP | CALC_DIF_DEFAULTBUTTON, mOk, nullptr, nullptr },
	{ CALC_DI_BUTTON,       0,  0,  0,  0, CALC_DIF_CENTERGROUP, mCancel, nullptr, nullptr },
};


BOOL InitConfig()
{
	return TRUE;
}

BOOL LoadConfig()
{
	api->SettingsBegin();
	for (int i = 0; param_table[i].ival; i++)
	{
		if (param_table[i].sval)
		{
			std::wstring str;
			if (api->SettingsGet(param_table[i].reg_name, &str, nullptr))
				wcsncpy(param_table[i].sval, str.c_str(), (int)param_table[i].ival / sizeof(wchar_t));
			else
				wcsncpy(param_table[i].sval, param_table[i].def_value, (int)param_table[i].ival / sizeof(wchar_t));
		} else
		{
			if (!api->SettingsGet(param_table[i].reg_name, nullptr, (DWORD *)param_table[i].ival))
				*param_table[i].ival = _wtoi(param_table[i].def_value);
		}
	}
	api->SettingsEnd();
	return TRUE;
}

BOOL SaveConfig()
{
	api->SettingsBegin();
	for (int i = 0; param_table[i].ival; i++)
	{
		if (param_table[i].sval)
		{
			std::wstring str = param_table[i].sval;
			api->SettingsSet(param_table[i].reg_name, &str, nullptr);
		}
		else
		{
			api->SettingsSet(param_table[i].reg_name, nullptr, (DWORD *)param_table[i].ival);
		}
	}
	api->SettingsEnd();
	return TRUE;
}

BOOL CheckConfig()
{
	wchar_t decimal = 0, args = 0;
	for (int i = 0; param_table[i].ival; i++)
	{
		int *v = param_table[i].ival;
		wchar_t *s = param_table[i].sval;
		switch (param_table[i].check_type)
		{
		case CALC_PARAM_CHECK_BOOL:
			*v = (*v != 0) ? 1 : 0;
			break;
		case CALC_PARAM_CHECK_RADIO:
			*v = std::min(std::max(*v, 0), 2);
			break;
		case CALC_PARAM_CHECK_LINES:
			*v = std::min(std::max(*v, 0), 10);
			break;
		case CALC_PARAM_CHECK_PERIOD:
			*v = std::min(std::max(*v, 0), 100);
			break;
		case CALC_PARAM_CHECK_LENGTH:
		case CALC_PARAM_CHECK_FRAC:
			*v = std::min(std::max(*v, 16), 512);
			break;
		case CALC_PARAM_CHECK_DECIMAL:
			if (props.use_regional)
			{
				GetLocaleInfo(GetSystemDefaultLCID(), LOCALE_SDECIMAL, s, 2);
			}
			if (wcschr(L".,", s[0]) == nullptr)
				s[0] = '.';
			decimal = s[0];
			s[1] = '\0';
			break;
		case CALC_PARAM_CHECK_ARGS:
			if (s[0] == '\0' || wcschr(L",;.:", s[0]) == nullptr)
				s[0] = ',';
			if (s[0] == decimal)
				s[0] = (decimal == L',') ? ';' : ',';
			args = s[0];
			s[1] = '\0';
			break;
		case CALC_PARAM_CHECK_DELIM:
			if (props.use_regional)
			{
				GetLocaleInfo(GetSystemDefaultLCID(), LOCALE_SMONTHOUSANDSEP , s, 2);
			}
			if (wcschr(L" ,'`\xa0", s[0]) == nullptr)
				s[0] = ' ';
			if (s[0] == decimal || s[0] == args)
				s[0] = ' ';

			s[1] = '\0';
			break;
		}
	}
	return TRUE;
}

class DlgConfig : public CalcDialog
{

};

bool ConfigDialog()
{
	const int num = sizeof(dlgItems)/sizeof(dlgItems[0]);
	wchar_t tmpnum[num][32];

	int ExitCode, ok_id = -2;
	
	auto *dlg = new CalcDialogItem[num];

	int Y1 = 0;
	for (int i = 0; i < num; i++)
	{
		dlg[i].Type = dlgItems[i].Type;	dlg[i].Flags = dlgItems[i].Flags;
		dlg[i].X1 = dlgItems[i].X1;		dlg[i].X2 = dlgItems[i].X2;
		
		dlg[i].Y1 = (dlgItems[i].Y1 == -1) ? (Y1+1) : ((dlgItems[i].Y1 == 0) ? Y1 : dlgItems[i].Y1);
		dlg[i].Y2 = (dlgItems[i].Type == CALC_DI_FIXEDIT) ? dlg[i].Y1 : dlgItems[i].Y2;
		
		Y1 = dlg[i].Y1;
		
		if (dlgItems[i].name_ID == mOk)
			ok_id = i;
		
		if (dlgItems[i].Type == CALC_DI_FIXEDIT)
		{
			if (dlgItems[i].prop_svalue)
				dlg[i].PtrData = dlgItems[i].prop_svalue;
			else if (dlgItems[i].prop_ivalue)
			{
				_itow(*dlgItems[i].prop_ivalue, tmpnum[i], 10);
				dlg[i].PtrData = tmpnum[i];
			} 
		}
		else
		{
			dlg[i].PtrData = dlgItems[i].name_ID ? api->GetMsg(dlgItems[i].name_ID) : L"";
			if (dlgItems[i].prop_ivalue && !dlgItems[i].prop_svalue)
				dlg[i].Selected = *dlgItems[i].prop_ivalue;
		}
	}
	
	dlg[0].Y2 = Y1 + 1;

	DlgConfig cfg;
	if (!cfg.Init(CALC_DIALOG_CONFIG, -1, -1, 58, 15, L"Config", dlg, num))
		return false;

	ExitCode = (int)cfg.Run();

	if (ExitCode == ok_id)
	{
		for (int i = 0; i < num; i++)
		{
			std::wstring str;
			if (dlgItems[i].prop_svalue)
			{
				cfg.GetText(i, str);
				wcsncpy(dlgItems[i].prop_svalue, str.c_str(), (int)dlgItems[i].prop_ivalue / sizeof(wchar_t));
			}
			else if (dlgItems[i].prop_ivalue)
			{
				if (dlgItems[i].Type == CALC_DI_FIXEDIT)
				{
					cfg.GetText(i, str);
					*dlgItems[i].prop_ivalue = _wtoi(str.c_str());
				} 
				else
				{
					*dlgItems[i].prop_ivalue = cfg.IsChecked(i) ? 1 : 0;
				}
			} 
		}
	}

	delete [] dlg;

	return true;
}

