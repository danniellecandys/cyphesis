#This file is distributed under the terms of the GNU General Public license.
#Copyright (C) 1999 Al Riddoch (See the file COPYING for details).

from atlas import *
from world.objects.Thing import Thing
from common import log,const
from common.misc import set_kw
from whrandom import *

class Weather(Thing):
    def __init__(self, **kw):
        self.base_init(kw)
        set_kw(self,kw,"rain",1.0)
    def tick_operation(self, op):
        res = Message()
        optick = Operation("tick", to=self)
        res = res + optick
        if self.rain<0.5:
            optick.time.sadd=randint(120,300)
            self.rain=1.0
        else:
            optick.time.sadd=randint(600,1200)
            self.rain=0.0
        res = res+Operation("set", Entity(self.id,rain=self.rain), to=self)
        return res
