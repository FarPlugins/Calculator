//
//  Copyright (c) uncle-vunkis 2009-2012 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#define _CRT_NON_CONFORMING_SWPRINTFS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <float.h>

#include "api.h"
#include "newparse.h"
#include "messages.h"
#include "sarg.h"
#include "syntax.h"

extern int calc_edit_length;


std::wstring cur_op_name = L"";

void MathOpNameCallback(std::wstring name)
{
	cur_op_name = name;
}


PSyntax CalcParser::Consts = nullptr;
PVars CalcParser::Vars   = nullptr;
PSyntax CalcParser::Ops    = nullptr;
PSyntax CalcParser::Functs = nullptr;
PSyntax CalcParser::Addons = nullptr;
PSyntax CalcParser::Numerals = nullptr;
std::vector<CalcAddon> CalcParser::addons;
unsigned CalcParser::main_addons_num = 0;
wchar_t CalcParser::delim_decimal, CalcParser::delim_args, CalcParser::delim_digit;

std::unordered_map<std::wstring, CalcParser *> CalcParser::user_bin_ops, CalcParser::user_un_ops;
CalcParser::UserFunctionList *CalcParser::user_funcs = nullptr;
ttmath::Conv CalcParser::from_convs[17], CalcParser::to_convs[17];

CalcParser::FunctionList CalcParser::allFunctions;
CalcParser::NamedConstantList CalcParser::allNamedConstants;
CalcParser::UnaryOperationTable CalcParser::allUnaryOpTable;
CalcParser::BinaryOperationTable CalcParser::allBinaryOpTable;
std::vector<BigInt> CalcParser::rep_fraction_coefs;
std::vector<Big> CalcParser::rep_mul1, CalcParser::rep_mul2;
Big CalcParser::rep_fraction_thr, CalcParser::rep_fraction_thr2;
int CalcParser::rep_fraction_max_start = 0, CalcParser::rep_fraction_max_period = 0;
int CalcParser::cont_fraction_max = 0;

PDialogData CalcParser::DialogData = 0;
int CalcParser::DialogsNum = 0;


CalcAddonPart::CalcAddonPart(const CalcAddonPart & p)
{
	expr = p.expr;
	str_pos = p.str_pos;
	parser = (p.parser != nullptr) ? new CalcParser(*p.parser) : nullptr;
}

CalcAddonPart::~CalcAddonPart()
{
  delete parser;
}

CalcParser::CalcParser()
{
	srand((unsigned)time(0));
	math_error = ERR_OK;

	Functions = allFunctions;
	NamedConstants = allNamedConstants;
	UnaryOpTable = allUnaryOpTable;
	BinaryOpTable = allBinaryOpTable;

	DELIM = delim_args;
}

CalcParser::CalcParser(const CalcParser &p)
{
	math_error = p.math_error;
	func_name = p.func_name;

	Functions = p.Functions;
	NamedConstants = p.NamedConstants;
	UnaryOpTable = p.UnaryOpTable;
	BinaryOpTable = p.BinaryOpTable;
}

CalcParser::~CalcParser()
{
	Functions.clear_all();
	NamedConstants.clear_all();
	UnaryOpTable.clear_all();
	BinaryOpTable.clear_all();
}

bool CalcParser::AddAll(bool add_user_ops_and_funcs)
{
	static struct 
	{
		string name;
		int priority;
	} builtin_ops[] = 
	{
		// should be lower-case!

		{ L"_lor", 1 },
		{ L"_land", 1 },
		
		{ L"_or", 2 },
		{ L"_xor", 3 },
		{ L"_and", 4 },
		{ L"_neq", 5 },
		{ L"_eq", 5 },
		{ L"_gt", 5 },
		{ L"_lt", 5 },
		{ L"_shr", 7 },
		{ L"_shl", 7 },
		{ L"_ror", 7 },
		{ L"_rol", 7 },
		{ L"_minus", 8 },
		{ L"_plus", 8 },
		{ L"_mod", 9 },
		{ L"_div", 9 },
		{ L"_mul", 9 },
		{ L"_pow", 10 },
		
		{ L"", -1 },
	};
	
	static struct
	{
		void *f;
		wchar_t *name;
		int num_args;
	} builtin_funcs[] = 
	{
		// should be lower-case!

		{ SArg::_factor, L"_factor", 1 },
		{ SArg::_frac, L"_frac", 1 },
		{ SArg::_floor, L"_floor", 1 },
		{ SArg::_ceil, L"_ceil", 1 },
		{ SArg::_sin, L"_sin", 1 },
		{ SArg::_cos, L"_cos", 1 },
		{ SArg::_tan, L"_tan", 1 },
		{ SArg::_arctan, L"_arctan", 1 },
		{ SArg::_ln, L"_ln", 1 },
		{ SArg::_rnd, L"_rnd", 0 },
		//{ SArg::_sum, L"_sum", 1 },
		//{ SArg::_avr, L"_avr", 1 },
		{ SArg::_if, L"_if", 3 },

		{ SArg::_f2b, L"_f2b", 1 },
		{ SArg::_d2b, L"_d2b", 1 },
		{ SArg::_b2f, L"_b2f", 1 },
		{ SArg::_b2d, L"_b2d", 1 },
		{ SArg::_finf, L"_finf", 0 },
		{ SArg::_fnan, L"_fnan", 0 },

		{ SArg::_numer, L"_numer", 1 },
		{ SArg::_denom, L"_denom", 1 },
		{ SArg::_gcd,   L"_gcd", 2 },
		
		// Functional operators
		{ SArg::to_int64, L"_int64", 1 },
		{ SArg::to_uint64, L"_uint64", 1 },
		{ SArg::to_int, L"_int", 1 },
		{ SArg::to_uint, L"_uint", 1 },
		{ SArg::to_short, L"_short", 1 },
		{ SArg::to_ushort, L"_ushort", 1 },
		{ SArg::to_char, L"_char", 1 },
		{ SArg::to_byte, L"_byte", 1 },
		{ SArg::to_double, L"_double", 1 },
		{ SArg::to_float, L"_float", 1 },
		
		{ nullptr, L"", 0 }
	};

	int i;

	// add reg-exps to the numerals
	PSyntax pnum = Numerals;
	while (pnum)
	{
		std::wstring nam = pnum->name;
		if (delim_decimal != L'.')
		{
			std::wstring d;
			d.append(1, L'\\');
			d.append(1, delim_decimal);
			for (;;)
			{
				int idx = (int)nam.find(L"\\.");
				if (idx == std::wstring::npos)
					break;
				nam.replace(idx, 2, d);
			}
		}

		if (delim_digit != L' ')
		{
			std::wstring d;
			d.append(1, L'\\');
			d.append(1, delim_digit);
			for (;;)
			{
				int idx = (int)nam.find(L"\\ ");
				if (idx == std::wstring::npos)
					break;
				nam.replace(idx, 2, d);
			}
		}

		if (pnum->re)
			trex_free(pnum->re);
		const TRexChar *err = nullptr;
		if (nam[0] == '/' && nam[nam.size() - 1] == '/')
			nam = nam.substr(1, nam.size() - 2);
		pnum->re = trex_compile(nam.c_str(), &err);

		if (wcscmp(pnum->mean, L"op0"))
		{
			std::wstring tmpmean = ReplaceDelims(pnum->mean);
			delete [] pnum->mean;
			pnum->mean = new wchar_t[tmpmean.size()+1];
			wcscpy(pnum->mean, tmpmean.c_str());
		}

		pnum = pnum->next;
	}

    // add built-in funcs
	for (i = 0; builtin_funcs[i].f; i++)
	{
		switch (builtin_funcs[i].num_args)
		{
		case 0:
			allFunctions.add(builtin_funcs[i].name, (SArg (*)())builtin_funcs[i].f, L"");
			break;
		case 1:
			allFunctions.add(builtin_funcs[i].name, (SArg (*)(const SArg &))builtin_funcs[i].f, L"");
			break;
		case 2:
			allFunctions.add(builtin_funcs[i].name, (SArg (*)(const SArg &, const SArg &))builtin_funcs[i].f, L"");
			break;
		case 3:
			allFunctions.add(builtin_funcs[i].name, (SArg (*)(const SArg &, const SArg &, const SArg &))builtin_funcs[i].f, L"");
			break;
		}
	}
   
	int max_prior = 0;
	PSyntax op = Ops;
	while (op)
	{
		if (op->priority > max_prior && op->priority < 99)
			max_prior = op->priority;
		op = op->next;
	}

	for (i = 0; builtin_ops[i].priority >= 0; i++)
	{
		if (builtin_ops[i].priority > max_prior && builtin_ops[i].priority < 99)
			max_prior = builtin_ops[i].priority;
	}

	allBinaryOpTable.set_num_levels(max_prior + 1);

	for (i = 0; builtin_ops[i].priority >= 0; i++)
	{
		allBinaryOpTable[builtin_ops[i].priority]->add(builtin_ops[i].name, &builtin_binary_op);
	}

	// should be lower-case!
	allUnaryOpTable.add(L"-", &mybase::operator_unary_minus);
    allUnaryOpTable.add(L"+", &mybase::operator_unary_plus);
    allUnaryOpTable.add(L"_lnot", &mybase::operator_logical_not);
	allUnaryOpTable.add(L"_not", &builtin_unary_not);
	
	if (add_user_ops_and_funcs)
	{
		op = Ops;

		// delete all parsers
		for (const auto& it :user_bin_ops)
		{
			delete it.second;
		}
		for (const auto& it : user_un_ops)
		{
      delete it.second;
		}
		user_bin_ops.clear();
		user_un_ops.clear();
		user_bin_ops.reserve(100);
		user_un_ops.reserve(10);
		while (op)
		{
			// check for num. args
			if (wcsstr(op->mean, L"op1"))
			{	
				CalcParser *opparser = new CalcParser();
				if (wcsstr(op->mean, L"op0"))
					opparser->add_argument(L"op0", 0);
				if (wcsstr(op->mean, L"op1"))
					opparser->add_argument(L"op1", 1);

				std::wstring mean = ReplaceDelims(op->mean);

				if (opparser->parse(mean.c_str()))
				{
					if (wcsstr(op->mean, L"op0"))	// binary
					{
						allBinaryOpTable[op->priority]->add(op->name, &binary_op);
						user_bin_ops.insert(std::make_pair(op->name, opparser));
					}
					else							// unary
					{
						allUnaryOpTable.add(op->name, &unary_op);
						user_un_ops.insert(std::make_pair(op->name, opparser));
					}
				} else
					delete opparser;
			}
			op = op->next;
		}
	
		// add named constants
		PSyntax consts = Consts;
		CalcParser parser;
		while (consts)
		{
			std::wstring mean = ReplaceDelims(consts->mean);
			if (parser.parse(mean.c_str()))
			{
				SArg val = parser.eval();
				allNamedConstants.add(consts->name, val, L"");
			}
			consts = consts->next;
		}

    delete user_funcs;
		user_funcs = new UserFunctionList();

		PSyntax func = Functs;
		while (func)
		{
			CalcParser *p = new CalcParser();
      // check for num. args
      int num_args;
      p->func_name = func->name;
      if (func->mean != nullptr)
      {
        for (num_args = 0; ; num_args++)
        {
          wchar_t arg[5];
          swprintf(arg, L"op%d", num_args);
          if (wcsstr(func->mean, arg) == nullptr)
            break;
          p->add_argument(arg, num_args);
        }

        std::wstring mean = ReplaceDelims(func->mean);
        if (p->parse(mean.c_str()))
        {
          allFunctions.add(p->func_name, (const MathExpressionBase<SArg> *)p, L"");
          user_funcs->push_back(p);
        } else
        {
          delete p;
        }
      } else {
        delete p;
      }
			
			
			func = func->next;
		}
	}

	return true;
}

std::wstring CalcParser::ReplaceDelims(const wchar_t *str)
{
	std::wstring s = str;
	if (delim_args != L',')
	{
		std::wstring d;
		d.append(1, delim_args);
		for (;;)
		{
			int idx = (int)s.find(L',');
			if (idx == std::wstring::npos)
				break;
			s.replace(idx, 1, d);
		}
	}
	if (delim_decimal != L'.')
	{
		std::wstring d;
		d.append(1, delim_decimal);
		for (;;)
		{
			int idx = (int)s.find(L'.');
			if (idx == std::wstring::npos)
				break;
			s.replace(idx, 1, d);
		}
	}
	return s;
}

SArg CalcParser::Parse(const wchar_t* str, bool case_sensitive)
{
	wchar_t tmp[1025];

	math_error = ERR_OK;
	try
	{
		if (wcslen(str) > 1024)
			throw ERR_TOOLONG;
		if (!case_sensitive)
		{
			wcscpy(tmp, str);
			_wcslwr(tmp);
			str = tmp;
		}

		if (!parse(str))
		{ 
			// error
			SArg res = 0;
			set_error_id(PARSER_ERROR_UNEXPECTED);
			return res;
		}
		SArg res = eval();
		return res;
	} 
	catch(CALC_ERROR exc)
	{
		math_error = exc;
	}
	return 0;
}

bool CalcParser::parse_number(SArg *value, const wchar_t *curpos, wchar_t **endptr)
{
	static Big Big_0(0);
	int sm_begin, sm_end;
	bool sm_res;
	int sm_num;

	TRexMatch sm;

	bool res = false;

	if (*curpos == '\0')
	{
		*value = Big_0;
		*endptr = (wchar_t *)curpos;
		return true;
	}

	PSyntax tmp = Numerals;
	int max_match = 0;
	Big tmpbig;
	wchar_t *tmpend = nullptr;
	while (tmp)
	{
		res = false;
		if (tmp->re == nullptr)
		{
			tmp = tmp->next;
			continue;
		}
		
		const wchar_t *sm_b, *sm_e;
		sm_res = trex_search(tmp->re, curpos, &sm_b, &sm_e) != 0;
		sm_begin = (int)(sm_b - curpos);
		sm_end   = (int)(sm_e - curpos);
		sm_num = trex_getsubexpcount(tmp->re);

		if (sm_res && sm_num >= 0 && sm_begin == 0 && sm_end > 0 && (sm_end - sm_begin) > max_match)
		{
			int base = tmp->radix;
			if (tmp->radix < 0)
				base = 10;
			if (base >= 2 && base <= 16)
			{
				const wchar_t *start = curpos;
				int sm_begini, sm_leni;
				wchar_t arg[10];
				
				SArg r = 0;
				if (sm_num > 1)
				{
					trex_getsubexp(tmp->re, 1, &sm);
					sm_begini = (int)(sm.begin - curpos);
					sm_leni = sm.len;

					if (!_wcsicmp(tmp->mean, L"op0"))	// simple case goes first...
					{
						if (sm_begini >= 0 && sm_begini + sm_leni <= (int)wcslen(curpos))
						{
							start = curpos + sm_begini;
							if (tmp->radix < 0)
							{
								if (tmp->radix == CALC_RADIX_REPEATING)
								{
									const wchar_t *start_p = wcschr(start, delim_decimal);
									const wchar_t *start_r1 = wcschr(start, L'(');
									const wchar_t *start_r2 = wcschr(start, L')');
									if (start_p && start_r1 && start_r2)
									{
										res = tmpbig.FromString(start, from_convs[base], (const wchar_t**)&tmpend) == 0;
										Big tmpbig2;
										res = tmpbig2.FromString(start_r1 + 1, from_convs[base], (const wchar_t**)&tmpend) == 0;
										// don't use multiply because (a * (1/b)) is less accurate than (a/b)
										tmpbig += tmpbig2 / (rep_mul1[start_r1 - start_p] * rep_mul2[start_r2 - start_r1]);
									}
								}
								else if (tmp->radix == CALC_RADIX_CONTINUED)
								{
									const wchar_t *start_r1 = wcschr(start, L'[');
									const wchar_t *start_r2 = wcschr(start, L']');
									if (start_r1 && start_r2)
									{
										const wchar_t *nxt = start_r2;
										BigInt tmpi;
										Big tmpone;
										tmpone.SetOne();
										tmpbig.SetZero();
										for (;;)
										{
											for (nxt = nxt - 1; nxt > start_r1; nxt--)
											{
												if (*nxt == delim_args)
													break;
											}

											res = tmpi.FromString(nxt + 1, base, (const wchar_t**)&tmpend) == 0;
											tmpbig += tmpi;
											if (nxt <= start_r1 || tmpbig.IsZero())
											{
												if (nxt == start_r2 - 1)
													continue;
												break;
											}
											tmpbig = tmpone / tmpbig;
										}
									}
								}
							}
							else
								res = tmpbig.FromString(start, from_convs[base], (const wchar_t**)&tmpend) == 0;
							r = SArg(tmpbig);
						}
					} else
					{
						SArg *args = new SArg [sm_num];
						CalcParser parser;
						int cm;
						
						for (cm = 1; cm < sm_num; cm++)
						{
							trex_getsubexp(tmp->re, cm, &sm);
							sm_begini = (int)(sm.begin - curpos);
							sm_leni = sm.len;

							if (sm_begini >= 0 && sm_begini + sm_leni <= (int)wcslen(curpos))
							{
								start = curpos + sm_begini;
								
								wchar_t saved_end = start[sm_leni];
								*((wchar_t *)start + sm_leni) = '\0';

								res = tmpbig.FromString(start, from_convs[base], (const wchar_t**)&tmpend) == 0;
								
								*((wchar_t *)start + sm_leni) = saved_end;

								args[cm - 1] = SArg(tmpbig);

								swprintf(arg, L"op%d", cm - 1);
								parser.add_argument(arg, cm - 1);
							} else
							{
								args[cm - 1] = SArg(0);
								swprintf(arg, L"op%d", cm - 1);
								parser.add_argument(arg, cm - 1);
							}
						}

						if (cm > 1 && parser.parse(tmp->mean))
							r = parser.eval(args);

						delete [] args;
					}
				}
				else
				{
					res = tmpbig.FromString(start, from_convs[base], (const wchar_t**)&tmpend) == 0;
					r = SArg(tmpbig);
				}

				if (res)
				{
					*value = r;
					*endptr = (wchar_t *)curpos + sm_end;
					max_match = sm_end - sm_begin;
					
#if 0				
					// don't solve regexp collisions?
					if (max_match > 0)
						break;
#endif
				}
			}
		}
		tmp = tmp->next;
	}

	// decimal?
	if (max_match == 0)
	{
		res = tmpbig.FromString(curpos, from_convs[10], (const wchar_t**)endptr) == 0;
		if (res && *endptr - curpos > 0)
		{
			*value = SArg(tmpbig);
			max_match = 1;
		}
	}

	return max_match != 0;
}

CALC_ERROR CalcParser::GetError()
{
	if (math_error)
		return math_error;
	PARSER_ERROR err = get_error_id();
	switch (err)
	{
	case PARSER_ERROR_OK:
		return ERR_OK;
	case PARSER_ERROR_UNEXPECTED:
	case PARSER_ERROR_UNKNOWN_ID:
		return ERR_EXPR;
	case PARSER_ERROR_MISSING_O_BRACE:
	case PARSER_ERROR_MISSING_C_BRACE:
		return ERR_BRACK;
	case PARSER_ERROR_NO_ARGUMENTS:
	case PARSER_ERROR_ARGUMENTS_MISMATCH:
		return ERR_EXPR;
	}

	return ERR_EXPR;
}

void CalcAddonPart::Parse(bool no_args)
{
	parser = new CalcParser();
	parser->math_error = ERR_OK;
	if (!no_args)
		parser->add_argument(L"op0", 0);
	bool ret;

	try
	{
		ret = parser->parse(expr.c_str());
	}
	catch(CALC_ERROR e)
	{
		parser->math_error = e;
	}

	if (!ret || parser->math_error != ERR_OK)
	{
		delete parser;
		parser = nullptr;
	}
}

bool CalcParser::ProcessAddons()
{
	PSyntax tmp = Addons;
	int num = 0;
	main_addons_num = 0;
	while (tmp)
	{
		if (!(tmp->flags & CALC_ADDON_FLAG_DIALOG))
			main_addons_num++;
		num++;
		tmp = tmp->next;
	}

	const wchar_t *submask = L"\\{([^\\{\\}]+?)\\}";
	const TRexChar *err = nullptr;
	TRex *re = trex_compile(submask, &err);
	TRexMatch sm;

	// just in case there's no declared addons...
	if (main_addons_num < 1)
	{
		num++;
		addons.resize(num);
		addons[0].name = L"&dec";
		addons[0].radix = 10;
	}
	else
		addons.resize(num);
	
	tmp = Addons;
	for (num = 0; tmp; num++)
	{
		CalcAddon &a = addons[num];
		a.name = tmp->name ? tmp->name : L"?";
		if (tmp->radix != 0)
			a.radix = tmp->radix;
		a.flags = tmp->flags;
		a.expr = tmp->mean ? tmp->mean : L"";
		a.parts.clear();

		// now let's analyze addon pattern
		while (true)
		{
			int sm_start[2], sm_len[2];

			const wchar_t *str = a.expr.c_str(), *str_s, *str_e;

			if (!trex_search(re, str, &str_s, &str_e))
				break;
			if (trex_getsubexpcount(re) < 1)
				break;

			sm_start[0] = (int)(str_s - str);
			sm_len[0] = (int)(str_e - str_s);

			trex_getsubexp(re, 1, &sm);
			sm_start[1] = (int)(sm.begin - str);
			sm_len[1] = sm.len;

			a.parts.resize(a.parts.size() + 1);
			CalcAddonPart &part = a.parts[a.parts.size() - 1];
			
			part.expr = a.expr.substr(sm_start[1], sm_len[1]);
			part.expr = ReplaceDelims(part.expr.c_str());
			a.expr.erase(sm_start[0], sm_len[0]);
			part.str_pos = sm_start[0];
		}

		for (unsigned i = 0; i < a.parts.size(); i++)
		{
			CalcAddonPart &part = a.parts[i];
			part.Parse();
		}
		
		tmp = tmp->next;
	}

	trex_free(re);

	return true;
}

bool CalcParser::ProcessDialogData()
{
	int i;
	PDialogData dd;
	for (i = 1, dd = CalcParser::DialogData; dd; dd = dd->Next, i++)
	{
		for (PDialogElem de = dd->Elem; de; de = de->Next)
		{
			if (de->input)
				de->input->Parse();
			else if (de->scale_expr)
			{
				de->scale_expr->Parse(true);
				if (de->scale_expr->parser)
					de->scale = de->scale_expr->parser->eval().GetBig();
				delete de->scale_expr;
				de->scale_expr = nullptr;
			}

		}
	}
	return true;
}

int CalcParser::GetNumDialogs()
{
	return DialogsNum;
}

bool CalcParser::InitTables(int rep_fraction_max_start, int rep_fraction_max_period, int cont_fraction_max)
{
	int i;
	delete Consts;
	delete Ops;
	delete Functs;
	delete Addons;
	delete Numerals;
	Consts = Ops = Functs = Addons = Numerals = nullptr;

	allFunctions.remove_all();
	allNamedConstants.remove_all();
	allUnaryOpTable.remove_all();
	allBinaryOpTable.remove_all();

	rep_fraction_coefs.resize(rep_fraction_max_period);
	rep_mul1.resize(1024);
	rep_mul2.resize(1024);
	BigInt m;
	m = 10;
	m.Pow(rep_fraction_max_start);
	for (i = 0; i < rep_fraction_max_period; i++)
	{
		BigInt n = 10;
		n.Pow(i + 1);
		n -= 1;
		rep_fraction_coefs[i] = n * m;
	}
	for (i = 1; i < 1024; i++)
	{
		BigInt n = 10;
		n.Pow(i - 1);
		rep_mul1[i] = n;
		n -= 1;
		rep_mul2[i].SetOne();
		rep_mul2[i] = n;
	}
	rep_fraction_thr = Big(L"1e-150");
	rep_fraction_thr2 = Big(L"1e-200");
	
	CalcParser::rep_fraction_max_start = rep_fraction_max_start;
	CalcParser::rep_fraction_max_period = rep_fraction_max_period;
	CalcParser::cont_fraction_max = cont_fraction_max;
	return true;
}

bool CalcParser::ProcessData(PSgmlEl BaseRc, bool case_sensitive)
{
	wchar_t *par1;
	PSgmlEl Base, El, Set;
	wchar_t lang_name[32];

  delete DialogData;
	DialogData = 0;
	DialogsNum = 0;

	// XXX: set default lang name
	swprintf(lang_name, 32, L"en:name");
	if (api->GetMsg(mLang))
		swprintf(lang_name, 32, L"%s:name", api->GetMsg(mLang));

	Base = BaseRc;
	while(Base = Base->next())
	{
		if (!Base || Base->getname() && !_wcsicmp(Base->getname(),L"Calc")) 
			break;
	}
  
	if (!Base)
	    return false;
  
	for (int pass = 1; pass <= 2; pass++)
	{
		El = Base->child();
		while(El)
		{
			if ( El->getname())
			{
				if (!_wcsicmp(El->getname(), L"lang") && El->GetChrParam(L"id"))
					swprintf(lang_name, 32, L"%s:name", El->GetChrParam(L"id"));
				else if (!_wcsicmp(El->getname(), L"Use"))
				{
					par1 = El->GetChrParam(L"Set");
					Set = Base->child();
					PSgmlEl FoundSet = nullptr;
					while(Set)
					{
						if (Set->getname() && !_wcsicmp(Set->getname(),L"Set") &&
							!_wcsicmp(par1,Set->GetChrParam(L"Name")))
						{
							FoundSet = Set;
						}
						Set = Set->next();
					}
					if (FoundSet)
					{
						if (pass == 1)
							FillSet(FoundSet, case_sensitive);
						if (pass == 2)
							FillDialogData(FoundSet, case_sensitive, lang_name);
					}
				}
			}
			El = El->next();
		}
	}

	return true;
}

void CalcParser::FillDialogData(PSgmlEl Base, bool case_sensitive, const wchar_t *lang_name)
{
	wchar_t *param;
	PSgmlEl El = Base, tmp;
	PDialogData dd = DialogData, dd1;
	PDialogElem de = 0, de1;

	El = El->child();

	while (dd)
	{
		if (!dd->Next)
			break;
		dd = dd->Next;
	}

	for (; El; El = El->next())
	{
		param = El->getname();
		if (!param) 
			continue;
		if (_wcsicmp(param, L"dialog") || !El->GetChrParam(lang_name))
			continue;
		dd1 = new SDialogData;
		if (dd)
		{
			dd->Next = dd1;
			dd = dd1;
		}

		if (!dd) 
			DialogData = dd = dd1;
		wcscpy(dd->Name, El->GetChrParam(lang_name));
		DialogsNum++;
		de = 0;
		dd->num = 0;
		for (tmp = El->child(); tmp; tmp = tmp->next())
		{
			if (!tmp->getname() || (_wcsicmp(tmp->getname(), L"text") && _wcsicmp(tmp->getname(), L"field")) ||
				!tmp->GetChrParam(lang_name)) 
				continue;
			de1 = new SDialogElem;
			if (de)
			{
				de->Next = de1;
				de = de1;
			}
			if (!de) 
				dd->Elem = de = de1;

			wcscpy(de->Name, tmp->GetChrParam(lang_name));
			if (!_wcsicmp(tmp->getname(), L"text")) 
				de->Type = 0;
			else 
				de->Type = 1;
			de->input = nullptr;
			de->scale = 1;
			de->addon_idx = -1;

			wchar_t *scale = tmp->GetChrParam(L"scale");
			wchar_t *input = tmp->GetChrParam(L"input");
			wchar_t *output = tmp->GetChrParam(L"output");
			
			if (input != nullptr && output != nullptr)
			{
				PSyntax nxt = new SSyntax;
				nxt->name = nullptr;
				nxt->name_set = nullptr;
				nxt->mean = new wchar_t[wcslen(output)+1];
				nxt->flags = CALC_ADDON_FLAG_DIALOG;
				wcscpy(nxt->mean, output);
				if (!case_sensitive)
					_wcslwr(nxt->mean);
				
				PSyntax adn = Addons;
				int idx = 0;
				while (adn)
				{
					idx++;
					if (!adn->next) 
						break;
					adn = adn->next;
				}
				if (!Addons) 
					Addons = nxt;
				else 
					adn->next = nxt;
				de->addon_idx = idx;

				de->input = new CalcAddonPart();
				de->input->expr = input;
			}
			else if (scale != nullptr)
			{
				//  check if constant expression is used
				std::wstring tmpscale = ReplaceDelims(scale);
				const wchar_t *s = tmpscale.c_str();
				size_t st = wcsspn(s, L" \t+-");
				size_t l = wcslen(s);
				if (st < l)
				{
					l -= st;
					s += st;
					size_t mantis = wcsspn(s, L"0123456789.,");
					bool expr = false;
					if (mantis < l)	// expression
					{
						l -= mantis;
						s += mantis;
						size_t pt = wcsspn(s, L"Ee");
						if (pt < l)	// expression
						{
							l -= pt;
							s += pt;
							if (*s == '-' || *s == '+')
							{
								l--;
								s++;
							}
							size_t exp = wcsspn(s, L"0123456789");
							if (exp < l)	// expression
							{
								de->scale_expr = new CalcAddonPart();
								de->scale_expr->expr = tmpscale;
								expr = true;
							}
						}
					}
					if (!expr)
					{
						if (de->scale.FromString(tmpscale.c_str(), from_convs[10]) != 0)	// number
							de->scale = 1;
					}

				}
			}
				
			dd->num++;
		}
	}
}

bool CalcParser::FillSet(PSgmlEl set, bool case_sensitive)
{
	PSgmlEl Ch = set->child();
	while(Ch)
	{
		if (Ch->getname() && !_wcsicmp(Ch->getname(), L"Const"))
			AddLexem(Consts, Ch, set, case_sensitive);
		if (Ch->getname() && !_wcsicmp(Ch->getname(), L"Op"))
			AddLexem(Ops, Ch, set, case_sensitive);
		if (Ch->getname() && !_wcsicmp(Ch->getname(), L"Func"))
			AddLexem(Functs, Ch, set, case_sensitive);
		if (Ch->getname() && !_wcsicmp(Ch->getname(), L"Addon"))
			AddLexem(Addons, Ch, set, case_sensitive);
		if (Ch->getname() && !_wcsicmp(Ch->getname(), L"Numeral"))
			AddLexem(Numerals, Ch, set, case_sensitive);
		Ch = Ch->next();
	}
	return true;
}

bool CalcParser::AddLexem(PSyntax &syntax, PSgmlEl Ch, PSgmlEl set, bool case_sensitive)
{
	PSyntax snt,nxt;
	SArg r;

	wchar_t *name = Ch->GetChrParam(L"Syntax");
	wchar_t *val = Ch->GetChrParam(L"Mean");
	wchar_t *name_set = set->GetChrParam(L"name");
	
	snt = syntax;
	while(snt)
	{
		if (!snt->next) break;
		snt = snt->next;
	}
  
	nxt = new SSyntax;
	nxt->name = name ? new wchar_t[wcslen(name)+1] : nullptr;
	nxt->name_set = name_set ? new wchar_t[wcslen(name_set)+1] : nullptr;
	nxt->mean = val ? new wchar_t[wcslen(val)+1] : nullptr;

	if (!_wcsicmp(Ch->getname(), L"op"))
	{
		wchar_t *val = Ch->GetChrParam(L"priority");
		nxt->priority = val != nullptr ? wcstol(val, nullptr, 10) : 0;
	}
	else if (!_wcsicmp(Ch->getname(), L"numeral") || !_wcsicmp(Ch->getname(), L"addon"))
	{
		nxt->radix = 0;
		nxt->flags = CALC_ADDON_FLAG_NONE;
		wchar_t *val = Ch->GetChrParam(L"format");
		if (val == nullptr)
			val = Ch->GetChrParam(L"radix");
		while (val != nullptr)
		{
			static const struct
			{
				const wchar_t *fmt;
				int radix;
				int flags;
			} addon_formats[] = 
			{
				{ L"dec", 10, 0 },
				{ L"hex", 16, 0 },
				{ L"oct",  8, 0 },
				{ L"bin",  2, 0 },
				{ L"exp", CALC_RADIX_EXPONENTIAL, 0 },			// exponential
				{ L"rep", CALC_RADIX_REPEATING, 0 },	// repeating decimal
				{ L"con", CALC_RADIX_CONTINUED, 0 },	// continued fraction
				{ L"delim", 0, CALC_ADDON_FLAG_DELIM },	// continued fraction
				{ nullptr, 0, 0 },
			};

			for (int i = 0; addon_formats[i].fmt != nullptr; i++)
			{
				if (_wcsnicmp(val, addon_formats[i].fmt, wcslen(addon_formats[i].fmt)) == 0)
				{
					if (addon_formats[i].radix != 0)
						nxt->radix = addon_formats[i].radix;
					if (addon_formats[i].flags > 0)
						nxt->flags |= addon_formats[i].flags;
					break;
				}
			}
			if (nxt->radix == 0)
			{
				nxt->radix = wcstol(val, nullptr, 10);
			}
			val = wcschr(val, ',');
			if (val)
				val++;
		}

	}
	
	if (name != nullptr)
	{
		wcscpy(nxt->name, name);
		if (!case_sensitive && _wcsicmp(Ch->getname(), L"addon"))	// don't make lowercase addon names
			_wcslwr(nxt->name);
	}
	if (name_set != nullptr)
		wcscpy(nxt->name_set, name_set);
	if (val != nullptr)
	{
		wcscpy(nxt->mean, val);
		if (!case_sensitive)
			_wcslwr(nxt->mean);
	}
  
	//DelSpaces(nxt->mean);

	if (!syntax) 
		syntax = nxt;
	else 
		snt->next = nxt;
  
	return true;
}

bool CalcParser::SetVar(wchar_t *name, SArg value)
{
	PVars snt, newv;
	bool fnd = false;
	snt = Vars;
	
	while(snt)
	{
		if (!_wcsicmp(name,snt->name))
		{
			fnd = true;
			break;
		}
    
		if (!snt->next) 
			break;
		snt = (PVars)snt->next;
	}
  
	if (!fnd)
	{
		newv = new SVars();
		newv->name = new wchar_t[wcslen(name)+1];
		newv->value = value;
		wcscpy(newv->name,name);
		if (!Vars) 
			Vars = newv;
		else 
			snt->next = newv;
    
		return true;
	}
  
	snt->value = value;
  
	return true;
}

bool CalcParser::SetDelims(wchar_t decimal, wchar_t args, wchar_t digit)
{
	static const int group_nums[17] = { 0,0,4,0,0,0,0,0,3,0,3,0,0,0,0,0,4 };
	delim_decimal = decimal;
	delim_args = args;
	delim_digit = digit;

	for (int i = 2; i <= 16; i++)
	{
		from_convs[i].base   = i;
		from_convs[i].comma  = delim_decimal;
		from_convs[i].comma2 = 0;
		from_convs[i].group  = delim_digit;

		to_convs[i].base         = i;
		to_convs[i].scient       = false;
		to_convs[i].scient_from  = -1;
		to_convs[i].round        = -1;
		to_convs[i].trim_zeroes  = true;
		to_convs[i].comma        = CalcParser::delim_decimal;
		to_convs[i].group        = delim_digit;
		to_convs[i].group_digits = group_nums[i];
	}

	return true;
}

int limit_number(int num_lim, const SArg & val, int radix)
{
	// XXX: This is ugly, I know...
	static const Big e10[8] = { Big(L"1e10"), Big(L"1e100"), Big(L"1e1000"), Big(L"1e10000"), Big(L"1e100000"), 
								Big(L"1e1000000"), Big(L"1e10000000"), Big(L"1e100000000") };
	static const Big e_10[8] = { Big(L"1e-10"), Big(L"1e-100"), Big(L"1e-1000"), Big(L"1e-10000"), Big(L"1e-100000"), 
								Big(L"1e-1000000"), Big(L"1e-10000000"), Big(L"1e-100000000") };
	Big b = val.GetBig();
	if (b < 0)
		num_lim--;
	b.Abs();
	if (b <= e_10[0] || b >= e10[0])
	{
		for (int i = 1; i < 9 && (b >= e10[i] || b <= e_10[i]); i++)
			num_lim--;
	} 
	else if (radix != CALC_RADIX_EXPONENTIAL)
		num_lim += 10;
	return num_lim;
}

/// \WARNING: This is a HIGHLY EXPERIMENTAL code!
void print_repeating_decimal(std::wstring & s, SArg val, int num_lim, bool group_delim)
{
	Big b = val.GetBig();
	Big period;
	BigInt bi;
	
	unsigned i, j;
	// calculate number of digits in a period (i)

	for (i = 0; i < CalcParser::rep_fraction_coefs.size(); i++)
	{
		period = b * CalcParser::rep_fraction_coefs[i];
		
		CalcParser::RoundUp(period);
		period.ToInt(bi);

#if 0
		{
			std::wstring ss;
			period.ToString(ss, 10, false, 250, -1);
			bi.ToString(ss);
			(period - bi).ToString(ss, 10, false, 250, -1);
		}
#endif
		
		if (ttmath::Abs(period - bi) < CalcParser::rep_fraction_thr)
			break;
	}

	// calculate number of digits BEFORE period (j)
	for (j = 0; j < CalcParser::rep_fraction_coefs.size(); j++)
	{
		period.ToInt(bi);
		if (ttmath::Abs(period - bi) > CalcParser::rep_fraction_thr)
			break;
		period /= 10;
	}
	i++;
	j--;
	
	if ((int)i <= CalcParser::rep_fraction_max_period && ttmath::Abs(b) > CalcParser::rep_fraction_thr)
	{
		ttmath::Conv conv = CalcParser::to_convs[10];
		conv.scient_from  = CalcParser::rep_fraction_coefs.size();
		conv.round        = -1;
		if (!group_delim)
			conv.group = 0;
		b.ToString(s, conv);
		std::basic_string <wchar_t>::size_type pt = s.find(CalcParser::delim_decimal);
		if (pt != std::wstring::npos)
		{
			int start = std::max(CalcParser::rep_fraction_max_start - (int)j, 0);
			int st = (int)pt + 1 + start;
			// process degenerated cases
			if (i == 1)
			{
				// don't show (0) periodic
				int idx = (int)s.find_first_not_of(L'0', st);
				if (idx < st || idx > st + (int)i)
				{
					s.replace((start < 1) ? pt : st, s.size(), L"");
					i = 0;
				}
				// process .(9)
				idx = (int)s.find_first_not_of(L'9', st);
				if (idx < st || idx > st + (int)i)
				{
					// round-up
					ttmath::Conv conv = CalcParser::to_convs[10];
					conv.scient_from  = CalcParser::rep_fraction_coefs.size();
					conv.round        = st - pt;
					if (!group_delim)
						conv.group = 0;
					b.ToString(s, conv);
					i = 0;
				}
			}
			if (i > 0)
			{
				if (st + i + 1 < s.size())
				{
					s.insert(st, L"(");
					s.replace(st + i + 1, s.size(), L")");
				}
			}
		} else
		{
			ttmath::Conv conv = CalcParser::to_convs[10];
			conv.scient_from  = num_lim;
			conv.round        = num_lim;
			if (!group_delim)
				conv.group = 0;
			b.ToString(s, conv);
		}
	}
	else
	{
		ttmath::Conv conv = CalcParser::to_convs[10];
		conv.scient_from  = num_lim;
		conv.round        = num_lim;
		if (!group_delim)
			conv.group = 0;
		b.ToString(s, conv);
	}
}

/// \WARNING: This is a HIGHLY EXPERIMENTAL code!
void print_continued_decimal(std::wstring & s, SArg val, int num_lim, bool group_delim)
{
	static const Big b1 = 1;

	Big b = val.GetBig();
	if (b < 0)
	{
		s += L"-";
		b.Abs();
	}
	Big bi;

	if (b.IsNan() || b < CalcParser::rep_fraction_thr)
	{
		ttmath::Conv conv = CalcParser::to_convs[10];
		conv.scient_from  = num_lim;
		conv.round        = num_lim;
		if (!group_delim)
			conv.group = 0;
		b.ToString(s, conv);
		return;
	}

	s += L"[";

#if 0
	// a little round-up
	if (b.mantissa.IsTheLowestBitSet())
	{
		if (b.mantissa.AddOne())
		{
			b.mantissa.Rcr(1, 1);
			b.exponent.AddOne();
		}
	}
#endif	

	for (int i = 0; i < CalcParser::cont_fraction_max; i++)
	{
		bi = b;
		bi.SkipFraction();
		b -= bi;

		// a little precision correction
		if (ttmath::Abs(b - b1) < CalcParser::rep_fraction_thr)
		{
			++bi;
			b = 0;
		}

		if (i > 0)
			s += CalcParser::delim_args;

		std::wstring tmps;
		ttmath::Conv conv = CalcParser::to_convs[10];
		conv.scient_from  = num_lim;
		conv.round        = num_lim;
		if (!group_delim)
			conv.group = 0;
		bi.ToString(tmps, conv);

		s += tmps;

		if (b < CalcParser::rep_fraction_thr)
			break;
		
		b = b1 / b;
		
	}

	s += L"]";
}

void CalcParser::RoundUp(Big &b)
{
	Big m;

	m.mantissa.SetOne();
	m.exponent.FromInt(b.exponent.ToInt() - m.mantissa.CompensationToLeft());
	m.info = 0;
	m.Standardizing();

	// add even more tolerance
	if (m < rep_fraction_thr2)
		m = rep_fraction_thr2;

	if (b.IsSign())
		m.SetSign();

	b.Add(m);
}

void CalcParser::GetFraction(Big b, BigInt *numer, BigInt *denom)
{
	static std::vector<BigInt> cont_series;
	static Big stored_b;
	static BigInt num, den;
	static const Big b1 = 1;

	if (b != stored_b)
	{
		stored_b = b;
		cont_series.clear();
		if (b < 0)
			b.Abs();
		BigInt bi;

		if (!b.IsNan())
		{
#if 0
			// a little round-up
			if (b.mantissa.IsTheLowestBitSet())
			{
				if (b.mantissa.AddOne())
				{
					b.mantissa.Rcr(1, 1);
					b.exponent.AddOne();
				}
			}
#endif

			for (int i = 0; i < CalcParser::cont_fraction_max; i++)
			{
				b.ToInt(bi);
				b -= bi;

				// a little precision correction
				if (ttmath::Abs(b - b1) < CalcParser::rep_fraction_thr)
				{
					++bi;
					b = 0;
				}

				cont_series.insert(cont_series.begin(), bi);

				if (b < CalcParser::rep_fraction_thr)
					break;

				b = b1 / b;
			}
		}

		den.SetOne();
		num = cont_series[0];
		BigInt nnum;
		for (unsigned i = 1; i < cont_series.size(); i++)
		{
			nnum = num * cont_series[i] + den;
			den = num;
			num = nnum;
		}
	}
	
	*numer = num;
	*denom = den;
}

void print_string(std::wstring & s, SArg val, int radix, int num_lim, bool append_suffix, bool pad_zeroes, bool group_delim)
{
	static const wchar_t *str_suffixes[17] = { 0, 0, L"b", 0, 0, 0, 0, 0, L"o", 0, 0, 0, 0, 0, 0, 0, L"h" };

	num_lim = limit_number(num_lim, val, radix);
	
	if (radix == CALC_RADIX_EXPONENTIAL)			// decimal-exponential
		val.GetBig().ToString(s, 10, true, 0, num_lim, true, CalcParser::delim_decimal);
	else if (radix == CALC_RADIX_REPEATING)		//  repeating decimal (decimal periodic fractions)
	{
		print_repeating_decimal(s, val, num_lim, group_delim);
	}
	else if (radix == CALC_RADIX_CONTINUED)		//  repeating decimal (decimal periodic fractions)
	{
		print_continued_decimal(s, val, num_lim, group_delim);
	}
	else if (radix == 10)
	{
		if (val.gettype() == SA_FLOAT || val.gettype() == SA_DOUBLE)
		{
			int cl = _fpclass((double)val);
			switch (cl)
			{
			case _FPCLASS_QNAN:
			case _FPCLASS_SNAN:
				s = L"NaN";
				return;
			case _FPCLASS_NINF:
				s = L"-Inf";
				return;
			case _FPCLASS_PINF:
				s = L"+Inf";
				return;
			}
		}
		
		ttmath::Conv conv = CalcParser::to_convs[10];
		conv.scient_from  = num_lim;
		conv.round        = num_lim;
		if (!group_delim)
			conv.group = 0;
		val.GetBig().ToString(s, conv);
	}
	else
	{
		if (!val.IsFixedLength() || !pad_zeroes || !val.Print(s, radix, 
					(wchar_t)CalcParser::to_convs[radix].group_digits, 
					group_delim ? (wchar_t)CalcParser::to_convs[radix].group : 0))
		{
			ttmath::Conv conv = CalcParser::to_convs[radix];
			conv.scient_from  = 1023;
			conv.round        = 0;
			if (!group_delim)
				conv.group = 0;
			val.GetBig().ToString(s, conv);
		}
		if (append_suffix && radix >= 0 && radix <= 16 && str_suffixes[radix] != nullptr)
			s += str_suffixes[radix];
	}
}

// XXX:
wchar_t *convertToString(const SArg & val, int type_idx, int num_lim, bool append_suffix, bool pad_zeroes, bool group_delim, CALC_ERROR *error_code)
{
	static const wchar_t *pad_string = L"0000000000000000000000000000000000000000000000000000000000000000";
	wchar_t *str = nullptr;
	std::wstring s;
	
	if (num_lim == 0)
		num_lim = calc_edit_length;
	
    if (type_idx == CALC_CONV_ENTER)
	{
		val.GetBig().ToString(s, 10, false, num_lim, num_lim, true, CalcParser::delim_decimal);
		str = _wcsdup(s.c_str());
		return str;
	}
	else if (type_idx == CALC_CONV_UNITS)
	{
		static const Big e10[5] =  { Big(L"1e10"),  Big(L"1e20"),  Big(L"1e30"),  Big(L"1e40"),  Big(L"1e50"), };
		static const Big e_10[5] = { Big(L"1e-10"), Big(L"1e-20"), Big(L"1e-30"), Big(L"1e-40"), Big(L"1e-50"), };

		Big b = val.GetBig();
		b.Abs();
		int e10_idx = num_lim / 10 - 1;
		if (e10_idx < 0) e10_idx = 0;
		if (e10_idx > 4) e10_idx = 4;
		if (b >= e10[e10_idx] || b <= e_10[e10_idx])
		{
			num_lim = limit_number(num_lim, val, CALC_RADIX_EXPONENTIAL);
			val.GetBig().ToString(s, 10, true, 0, num_lim, true, CalcParser::delim_decimal);
		}
		else
		{
			num_lim = limit_number(num_lim, val, 10);
			val.GetBig().ToString(s, 10, false, 255, num_lim, true, CalcParser::delim_decimal);
		}
		str = _wcsdup(s.c_str());
		return str;
	}
	else if (type_idx >= 0 && type_idx < (int)CalcParser::addons.size())			// custom addons
	{
		CalcAddon &addon = CalcParser::addons[type_idx];

		std::wstring out_expr = addon.expr;
		int sum_len = 0;

		bool addon_delim = (addon.flags & CALC_ADDON_FLAG_DELIM) ? group_delim : false;

		if (addon.parts.size() == 0)
		{
			print_string(out_expr, val, addon.radix, num_lim, append_suffix, pad_zeroes, addon_delim);
		}

		for (unsigned i = 0; i < addon.parts.size(); i++)
		{
			SArg Res;
			if (error_code)
				*error_code = ERR_OK;
			try
			{
				if (addon.parts[i].parser != nullptr)
					Res = addon.parts[i].parser->eval(&val);
				else
					Res = 0;
			}
			catch(CALC_ERROR ec)
			{
				if (error_code)
				{
					*error_code = ec;
					return nullptr;
				}
				Res = 0;
			}
			
			print_string(s, Res, addon.radix, num_lim, append_suffix, pad_zeroes, addon_delim);

			out_expr.insert(addon.parts[i].str_pos + sum_len, s);
			sum_len += (int)s.length();
		}
		if (out_expr.length() > 0)
			str = _wcsdup(out_expr.c_str());
	}

	return str;
}

int CalcParser::DelSpaces(wchar_t *str)
{
	int i, len = (int)wcslen(str);
	for (i = 0; i < len; i++)
	{
		if (str[i] == ' ')
		{
			wcscpy(&str[i],&str[i+1]);
			len--;
			i--;
		}
	}
	return len;
}

SArg CalcParser::builtin_unary_not(const SArg & op)
{
	return ~op;
}

SArg CalcParser::builtin_binary_op(const SArg & op0, const SArg & op1)
{
	if (cur_op_name == L"_lor")
		return (bool)op0 || (bool)op1;
	else if (cur_op_name == L"_land")
		return (bool)op0 && (bool)op1;
	else if (cur_op_name == L"_or")
		return op0 | op1;
	else if (cur_op_name == L"_and")
		return op0 & op1;
	else if (cur_op_name == L"_xor")
		return op0 ^ op1;
	else if (cur_op_name == L"_neq")
		return op0 != op1;
	else if (cur_op_name == L"_eq")
		return op0 == op1;
	else if (cur_op_name == L"_gt")
		return op0 > op1;
	else if (cur_op_name == L"_lt")
		return op0 < op1;
	else if (cur_op_name == L"_shr")
		return op0 >> op1;
	else if (cur_op_name == L"_shl")
		return op0 << op1;
	else if (cur_op_name == L"_ror")
		return op0.Ror(op1);
	else if (cur_op_name == L"_rol")
		return op0.Rol(op1);
	else if (cur_op_name == L"_plus")
		return op0 + op1;
	else if (cur_op_name == L"_minus")
		return op0 - op1;
	else if (cur_op_name == L"_mod")
		return op0 % op1;
	else if (cur_op_name == L"_div")
		return op0 / op1;
	else if (cur_op_name == L"_mul")
		return op0 * op1;
	else if (cur_op_name == L"_pow")
		return op0.Pow(op1);

	return SArg();
}

SArg CalcParser::binary_op(const SArg & op0, const SArg & op1)
{
	/// TODO: optimise!
	auto op = user_bin_ops.find(cur_op_name);
	auto opend = user_bin_ops.end();
	if (op != opend)
	{
		// check for num. args
		SArg args[2];
		args[0] = op0;
		args[1] = op1;
		SArg res = op->second->eval(args);
		return res;
	}
	return 0;
}

SArg CalcParser::unary_op(const SArg & op0)
{
	auto op = user_un_ops.find(cur_op_name);
	auto opend = user_un_ops.end();
	if (op != opend)
	{
		// check for num. args
		SArg args[2];
		args[1] = op0;
		SArg res = op->second->eval(args);
		return res;
	}
	return 0;
}

////////////////////////////////////////////////

CalcParser::UserFunctionList::~UserFunctionList()
{
	for (iterator ii = begin(); ii != end(); ++ii)
		delete *ii;
}
