//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  Copyright (c) uncle-vunkis 2009-2011 <uncle-vunkis@yandex.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

#include "syntax.h"
#include "sarg.h"

SSyntax::SSyntax()
{
	next = nullptr;
	name_set = nullptr;
	name = mean = nullptr;
	priority = 0;
	flags = 0;

	re = nullptr;
}

// XXX:
SSyntax::~SSyntax()
{
	delete next;
	delete [] name;
	delete [] name_set;
	delete [] mean;
	if (re) trex_free(re);
}

SVars::SVars()= default;

SVars::~SVars()= default;

