// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef __SHARED_DATEFORMAT_H__
#define __SHARED_DATEFORMAT_H__

#include "unicode/utypes.h"
#include "sharedobject.h"

U_NAMESPACE_BEGIN

class DateFormat;

class U_I18N_API SharedDateFormat : public SharedObject {
public:
    SharedDateFormat(DateFormat *nfToAdopt) : ptr(nfToAdopt) { }
    ~SharedDateFormat();
    const DateFormat *get() const { return ptr; }
    const DateFormat *operator->() const { return ptr; }
    const DateFormat &operator*() const { return *ptr; }
private:
    DateFormat *ptr;
    SharedDateFormat(const SharedDateFormat &);
    SharedDateFormat &operator=(const SharedDateFormat &);
};

U_NAMESPACE_END

#endif
