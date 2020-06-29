// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
******************************************************************************
* Copyright (C) 2014, International Business Machines
* Corporation and others.  All Rights Reserved.
******************************************************************************
* sharednumberformat.h
*/

#ifndef __SHARED_NUMBERFORMAT_H__
#define __SHARED_NUMBERFORMAT_H__

#include "unicode/utypes.h"
#include "sharedobject.h"

U_NAMESPACE_BEGIN

class Format;

class U_I18N_API SharedFormat : public SharedObject {
public:
    SharedFormat(Format *nfToAdopt) : ptr(nfToAdopt) { }
    virtual ~SharedFormat() { }
    const Format *get() const { return ptr; }
    const Format *operator->() const { return ptr; }
    const Format &operator*() const { return *ptr; }
private:
    Format *ptr;
    SharedFormat(const SharedFormat &);
    SharedFormat &operator=(const SharedFormat &);
};

U_NAMESPACE_END

#endif
