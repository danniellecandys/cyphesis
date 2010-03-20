// Cyphesis Online RPG Server and AI Engine
// Copyright (C) 2006 Alistair Riddoch
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

// $Id$

#include "common/BaseWorld.h"

#include <Atlas/Objects/RootOperation.h>

#include <sigc++/functors/ptr_fun.h>

#include <cstdlib>

#include <cassert>

static void test_function(Atlas::Objects::Operation::RootOperation)
{
}

class LocatedEntity;

class Entity
{
  public:
    const std::string m_id;
    const long m_intId;
    explicit Entity(const std::string & id, long intId) : m_id(id), m_intId(intId) { }

    const std::string & getId() const {
        return m_id;
    }

    long getIntId() const {
        return m_intId;
    }
};

class TestWorld : public BaseWorld {
  public:
    explicit TestWorld(Entity & gw) : BaseWorld(gw) {
        m_eobjects[m_gameWorld.getIntId()] = &m_gameWorld;
    }

    virtual bool idle(int, int) { return false; }
    virtual Entity * addEntity(Entity * ent) { 
        m_eobjects[ent->getIntId()] = ent;
        return 0;
    }
    virtual Entity * addNewEntity(const std::string &,
                                  const Atlas::Objects::Entity::RootEntity &) {
        return 0;
    }
    int createSpawnPoint(const Atlas::Message::MapType & data,
                         Entity *) { return 0; }
    int getSpawnList(Atlas::Message::ListType & data) { return 0; }
    Entity * spawnNewEntity(const std::string & name,
                            const std::string & type,
                            const Atlas::Objects::Entity::RootEntity & desc) {
        return addNewEntity(type, desc);
    }
    virtual Task * newTask(const std::string &, Character &) { return 0; }
    virtual Task * activateTask(const std::string &, const std::string &,
                                const std::string &, Character &) { return 0; }
    virtual void message(const Atlas::Objects::Operation::RootOperation & op, Entity & ent) { }
    virtual Entity * findByName(const std::string & name) { return 0; }
    virtual Entity * findByType(const std::string & type) { return 0; }
    virtual void addPerceptive(Entity *) { }
};

int main()
{
    // We have to use the TestWorld class, as it implements the functions
    // missing from BaseWorld interface.

    {
        // Test constructor
        Entity wrld("1", 1);
        TestWorld tw(wrld);
    }

    {
        // Test destructor
        Entity wrld("1", 1);
        BaseWorld * tw = new TestWorld(wrld);

        delete tw;
    }

    {
        // Test constructor sets singleton pointer
        Entity wrld("1", 1);
        TestWorld tw(wrld);

        assert(&BaseWorld::instance() == &tw);
    }

    {
        // Test constructor installs reference to world entity
        Entity wrld("1", 1);
        TestWorld tw(wrld);

        assert(&tw.m_gameWorld == &wrld);
    }

    {
        // Test retrieving non existant entity by string ID is ok
        Entity wrld("1", 1);
        TestWorld tw(wrld);

        assert(tw.getEntity("2") == 0);
    }

    {
        // Test retrieving existant entity by string ID is ok
        Entity wrld("1", 1);
        TestWorld tw(wrld);

        Entity * tc = new Entity("2", 2);

        tw.addEntity(tc);

        assert(tw.getEntity("2") == tc);
    }

    {
        // Test retrieving existant entity by integer ID is ok
        Entity wrld("1", 1);
        TestWorld tw(wrld);

        Entity * tc = new Entity("2", 2);

        tw.addEntity(tc);

        assert(tw.getEntity(2) == tc);
    }

    {
        // Test retrieving non existant entity by integer ID is ok
        Entity wrld("1", 1);
        TestWorld tw(wrld);

        assert(tw.getEntity(2) == 0);
    }

    {
        // Test retrieving reference to all entities is okay and empty
        Entity wrld("1", 1);
        TestWorld tw(wrld);

        assert(tw.getEntities().size() == 1);
    }

    {
        // Test getting the time
        Entity wrld("1", 1);
        TestWorld tw(wrld);

        tw.getTime();
    }

    {
        // Test getting the uptime
        Entity wrld("1", 1);
        TestWorld tw(wrld);

        tw.upTime();
    }

    {
        // Test connecting to the dispatch signal
        Entity wrld("1", 1);
        TestWorld tw(wrld);

        tw.Dispatching.connect(sigc::ptr_fun(&test_function));
    }

    return 0;
}

long integerId(const std::string & id)
{
    long intId = strtol(id.c_str(), 0, 10);
    if (intId == 0 && id != "0") {
        intId = -1L;
    }

    return intId;
}

int timeoffset = 0;
