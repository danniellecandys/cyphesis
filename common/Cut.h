// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2000,2001 Alistair Riddoch

#ifndef COMMON_CUT_H
#define COMMON_CUT_H

#include <Atlas/Objects/Operation/Action.h>

namespace Atlas { namespace Objects { namespace Operation {

class Cut : public Action {
  public:
    Cut();
    virtual ~Cut();
    static Cut Instantiate();
};

} } }

#endif // COMMON_CUT_H
