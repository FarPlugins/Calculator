//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

// creates object tree structure on html/xml files

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>

#ifdef USE_CREGEXP
// you can disable clocale class - but import string services
#include<regexp/clocale.h>
#endif
#include<sgml/sgml.h>
#include<sgml/tools.cpp>

CSgmlEl::CSgmlEl()
{
  eparent= 0;
  enext  = 0;
  eprev  = 0;
  echild = 0;
  chnum  = 0;
  type   = EBASEEL;

  name[0] = 0;
  content= 0;
  contentsz = 0;
  parnum = 0;
}

CSgmlEl::~CSgmlEl()
{
	if (type == EBASEEL && enext) 
		enext->destroylevel();
	if (echild) 
		echild->destroylevel();
	// if (name) delete[] name;
	if (content) 
		delete[] content;
	for (int i = 0; i < parnum; i++)
	{
		if (params[i][0]) 
			delete params[i][0];
		if (params[i][1]) 
			delete params[i][1];
	}
}

PSgmlEl CSgmlEl::createnew(ElType type, PSgmlEl parent, PSgmlEl after)
{
	PSgmlEl El = new CSgmlEl;
	El->type = type;
	if (parent)
	{
		El->enext = parent->echild;
		El->eparent = parent;
		if (parent->echild) 
			parent->echild->eprev = El;
		parent->echild = El;
		parent->chnum++;
		parent->type = EBLOCKEDEL;
	} 
	else if (after) 
		after->insert(El);
	return El;
}

bool CSgmlEl::init()
{
	return true;
}

wchar_t *CSgmlEl::readfile(const wchar_t *basename, const wchar_t *fname, const wchar_t *buf, int pos, int *bufsize)
{
	wchar_t tmpfname[4097];
	wcscpy(tmpfname, basename);
	int i;
	for (i = (int)wcslen(tmpfname); i; i--)
	{
		if (tmpfname[i] == '/' || tmpfname[i] == '\\') 
			break;
	}
	wcscpy(tmpfname+i+1, fname);
#if 0
	for (i = 0; tmpfname[i]; i++)
	{
		if (tmpfname[i] == '\\')
			tmpfname[i] = '/';
	}
#endif
	FILE *fp = _wfopen(tmpfname, L"rb");
	if (fp == NULL)
		return NULL;
	unsigned len, num_read = 0;

	unsigned char bom[2];
	bool ansi = false;
	fread(bom, 2, 1, fp);

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	rewind(fp);

	if (len < 2)
	{
		fclose(fp);
		return NULL;
	}

	if (bom[0] != 0xff && bom[1] != 0xfe)
	{
		ansi = true;
		len *= 2;
	}

	unsigned wl = (len + 1) / 2;
	wchar_t *src;
	
	if (buf == NULL)
	{
		src = (wchar_t *)malloc(len + 4);
		if (bufsize)
			*bufsize = wl;
		pos = 0;
	}
	else
	{
		if (!bufsize || !pos)
			return NULL;
		src = (wchar_t *)realloc((void *)buf, *bufsize * 2 + len + 4);
		memmove(src + pos + wl, src + pos, (*bufsize - pos) * 2);
		*bufsize += wl;
	}
	if (ansi)
	{
		num_read = (unsigned)fread(src + pos, 1, wl, fp);
		char *asrc = (char *)(src + pos) + wl - 1;
		wchar_t *dst = src + pos + wl - 1;
		for (int i = len / 2; i > 0; i--, dst--, asrc--)
		{
			mbtowc(dst, asrc, 1);
		}
		num_read *= 2;
	} else
	{
		num_read = (unsigned)fread(src + pos, 1, len, fp);
	}
	fclose(fp);

	if (num_read != len)
	{
		free(src);
		return NULL;
	}

	return src;
}

bool CSgmlEl::parse(const wchar_t *basename, const wchar_t *fname)
{
	PSgmlEl Child, Parent, Next = 0;
	int i, j, lins, line;
	int ls, le, rs, re, empty;

	int sz = 0;
	wchar_t *src = readfile(basename, fname, NULL, 0, &sz);
	if (src == NULL)
		return false;

	// start object - base
	type = EBASEEL;
	Next = this;

	lins = line = 0;
	for (i = 0; i < sz; i++)
	{
		if (i >= sz) 
			continue;

		// comments
		if (src[i] == '<' && src[i+1] == '!' && src[i+2] == '-' && src[i+3] == '-' && i+4 < sz)
		{
			i += 4;
			while((src[i] != '-' || src[i+1] != '-' || src[i+2] != '>') && i+3 < sz) 
				i++;
			i+=3;
		}

		line = i;

		if (src[i] == '<' || i >= sz-1)
		{
			while (line > lins)
			{
				// linear
				j = lins;
				while (iswspace(src[j]) && j < i)
					j++;
				if (j == i) 
					break; // empty text
				Child = createnew(EPLAINEL,0,Next);
				Child->init();
				Child->setcontent(src + lins, i - lins);
				Next = Child;
				break;
			}
			if (i == sz-1) 
				continue;
			// start or single tag
			if (src[i+1] != '/')
			{
				Child = createnew(ESINGLEEL,NULL,Next);
				Child->init();
				Next  = Child;
				j = i+1;
				while (src[i] != '>' && !iswspace(src[i]) && i < sz) 
					i++;
				// Child->name = new wchar_t[i-j+1];
				if (i-j > MAXTAG) 
					i = j + MAXTAG - 1;
				wcsncpy(Child->name, src+j, i-j);
				Child->name[i-j] = 0;
				// parameters
				Child->parnum = 0;
				while (src[i] != '>' && Child->parnum < MAXPARAMS && i < sz)
				{
					ls = i;
					while (iswspace(src[ls]) && ls < sz) 
						ls++;
					le = ls;
					while (!iswspace(src[le]) && src[le]!='>' && src[le]!='=' && le < sz) 
						le++;
					rs = le;
					while (iswspace(src[rs]) && rs < sz) 
						rs++;
					empty = 1;
					if (src[rs] == '=')
					{
						empty = 0;
						rs++;
						while (iswspace(src[rs]) && rs < sz) 
							rs++;
						re = rs;
						if (src[re] == '"')
						{
							while (src[++re] != '"' && re < sz)
								;
							rs++;
							i = re+1;
						}
						else if (src[re] == '\'')
						{
							while (src[++re] != '\'' && re < sz)
								;
							rs++;
							i = re+1;
						} else
						{
							while (!iswspace(src[re]) && src[re] != '>' && re < sz) 
								re++;
							i = re;
						}
					} else
						i = re = rs;
					
					if (ls == le) 
						continue;
					if (rs == re && empty)
					{
						rs = ls;
						re = le;
					}
					
					int pn = Child->parnum;
					Child->params[pn][0] = new wchar_t[le-ls+1];
					wcsncpy(Child->params[pn][0], src+ls, le-ls);
					Child->params[pn][0][le-ls] = 0;
					
					Child->params[pn][1] = new wchar_t[re-rs+1];
					wcsncpy(Child->params[pn][1], src+rs, re-rs);
					Child->params[pn][1][re-rs] = 0;
					
					Child->parnum++;
					
					substquote(Child->params[pn], L"&lt;", '<');
					substquote(Child->params[pn], L"&gt;", '>');
					substquote(Child->params[pn], L"&amp;", '&');
					substquote(Child->params[pn], L"&quot;", '"');
				}
				lins = i+1;

				// include
				if (_wcsicmp(Child->name, L"xi:include") == 0)
				{
					wchar_t *fn = Child->GetChrParam(L"href");
					if (fn)
					{
						wchar_t *new_src = readfile(basename, fn, src, i + 1, &sz);
						if (new_src)
							src = new_src;
					}
				}
			} else 
			{  // end tag
				j = i+2;
				i += 2;
				while (src[i] != '>' && !iswspace(src[i]) && i < sz) 
					i++;
				int cn = 0;
				for (Parent = Next; Parent->eprev; Parent = Parent->eprev, cn++)
				{
					if (!*Parent->name) 
						continue;
					int len = (int)wcslen(Parent->name);
					if (len != i-j) 
						continue;
					if (Parent->type != ESINGLEEL || _wcsnicmp((wchar_t*)src+j, Parent->name, len)) 
						continue;
					break;
				}
				
				if (Parent && Parent->eprev)
				{
					Parent->echild = Parent->enext;
					Parent->chnum = cn;
					Parent->type = EBLOCKEDEL;
					Child = Parent->echild;
					if (Child) 
						Child->eprev = 0;
					while(Child)
					{
						Child->eparent = Parent;
						Child = Child->enext;
					}
					Parent->enext = 0;
					Next = Parent;
				}
				while (src[i] != '>' && i < sz) 
					i++;
				lins = i+1;
			}
		}
	}
	
	free(src);
	return true;
}

void CSgmlEl::substquote(SParams par, wchar_t *sstr, wchar_t c)
{
	int len = (int)wcslen(sstr);
	int plen = (int)wcslen(par[1]);

	for (int i = 0; i <= plen-len; i++)
	{
		if (!wcsncmp(par[1]+i, sstr, len))
		{
			par[1][i] = c;
			for(int j = i+1; j <= plen-len+1; j++)
				par[1][j] = par[1][j+len-1];
			plen -= len-1;
		}
	}
}

bool CSgmlEl::setcontent(const wchar_t *src, int sz)
{
	content = new wchar_t[sz+1];
	memmove(content, src, sz);
	content[sz] = 0;
	contentsz = sz;
  return true;
}

void CSgmlEl::insert(PSgmlEl El)
{
	El->eprev = this;
	El->enext = this->enext;
	El->eparent = this->eparent;
	if (this->enext) 
		this->enext->eprev = El;
	this->enext = El;
}

// recursive deletion
void CSgmlEl::destroylevel()
{
	if (enext) 
		enext->destroylevel();
	delete this;
}

PSgmlEl CSgmlEl::parent()
{
	return eparent;
}

PSgmlEl CSgmlEl::next()
{
	return enext;
}

PSgmlEl CSgmlEl::prev()
{
	return eprev;
}

PSgmlEl CSgmlEl::child()
{
	return echild;
}

ElType  CSgmlEl::gettype()
{
	return type;
}

wchar_t *CSgmlEl::getname()
{
	if (!*name) 
		return NULL;
	return name;
}

wchar_t *CSgmlEl::getcontent()
{
	return content;
}

int CSgmlEl::getcontentsize()
{
	return contentsz;
}

wchar_t* CSgmlEl::GetParam(int no)
{
	if (no >= parnum) 
		return NULL;
	return params[no][0];
}

wchar_t* CSgmlEl::GetChrParam(const wchar_t *par)
{
	for (int i = 0; i < parnum; i++)
	{
		if (!_wcsicmp(par,params[i][0]))
			return params[i][1];
    }
	return NULL;
}

bool CSgmlEl::GetIntParam(const wchar_t *par, int *result)
{
	double res = 0;
	for (int i=0; i < parnum; i++)
	{
		if (!_wcsicmp(par,params[i][0]))
		{
			bool b = get_number(params[i][1],&res);
			*result = b ? (int)res : 0;
			return b;
		}
	}
	return false;
}

bool CSgmlEl::GetFloatParam(const wchar_t *par, double *result)
{
	double res;
	for (int i = 0; i < parnum; i++)
	{
		if (!_wcsicmp(par,params[i][0]))
		{
			bool b = get_number(params[i][1],&res);
			*result = b ? (double)res : 0;
			return b;
		}
	}
	return false;
}

PSgmlEl CSgmlEl::search(const wchar_t *TagName)
{
	PSgmlEl Next = this->enext;
	while(Next)
	{
		if (!_wcsicmp(TagName,Next->name)) 
			return Next;
		Next = Next->enext;
	}
	return Next;
}

PSgmlEl CSgmlEl::enumchilds(int no)
{
	PSgmlEl El = this->echild;
	while(no && El)
	{
		El = El->enext;
		no--;
	}
	return El;
}

PSgmlEl CSgmlEl::fprev()
{
	PSgmlEl El = this;
	if (!El->eprev) 
		return El->eparent;
	if (El->eprev->echild)
		return El->eprev->echild->flast();
	return El->eprev;
}

PSgmlEl CSgmlEl::fnext()
{
	PSgmlEl El = this;
	if (El->echild) 
		return El->echild;
	while(!El->enext)
	{
		El = El->eparent;
		if (!El) 
			return 0;
	}
	return El->enext;
}

PSgmlEl CSgmlEl::ffirst()
{
	PSgmlEl Prev = this;
	while(Prev)
	{
		if (!Prev->eprev) 
			return Prev;
		Prev = Prev->eprev;
	}
	return Prev;
}

PSgmlEl CSgmlEl::flast()
{
	PSgmlEl Nxt = this;
	while(Nxt->enext || Nxt->echild)
	{
		if (Nxt->enext)
		{
			Nxt = Nxt->enext;
			continue;
		}
		if (Nxt->echild)
		{
			Nxt = Nxt->echild;
			continue;
		}
	}
	return Nxt;
}
