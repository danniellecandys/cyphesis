// This file may be redistributed and modified only under the terms of
// the GNU Lesser General Public License (See COPYING for details).
// Copyright (C) 2000 Alistair Riddoch

#include <fstream.h>
#include <strstream>

#include <Atlas/Message/Object.h>
#include <Atlas/Objects/Root.h>
#include <Atlas/Objects/Operation/Login.h>
#include <Atlas/Codecs/XML.h>

#include <server/Admin.h>

#include <config.h>

#include "persistance.h"

Persistance * Persistance::m_instance = NULL;

Persistance * Persistance::instance()
{
    if (m_instance == NULL) {
        m_instance = new Persistance();
    }
    return m_instance;
}

#ifdef HAVE_LIBDB_CXX

// This is the version of the persistance code which is enabled if 
// there is db support.

// DB_CXX_NO_EXCEPTIONS is set for now to enable easier debugging. Later
// once the code is padded out, this should be removed to allow exceptions.

Persistance::Persistance() : account_db(NULL, DB_CXX_NO_EXCEPTIONS),
                             world_db(NULL, DB_CXX_NO_EXCEPTIONS) { }

bool Persistance::init()
{
    Persistance * p = instance();
    int i = p->account_db.open("/var/forge/cyphesis/db", "account",
                               DB_BTREE, DB_CREATE, 0600);
    int j = p->world_db.open("/var/forge/cyphesis/db", "world",
                             DB_BTREE, 0, 0600);
    return ((i == 0) && (j == 0));
}

Account * Persistance::load_admin_account()
{
    Persistance * p = instance();
    Account * adm;
    if ((adm = p->getAccount("admin")) == NULL) {
        adm = new Admin(NULL, "admin", "test");
        save_admin_account(adm);
    }
    return adm;
}

void Persistance::save_admin_account(Account * adm)
{
    Persistance * p = instance();
    p->putAccount(adm);
}

Account * Persistance::getAccount(const std::string & name)
{
    return NULL;
}

void Persistance::putAccount(const Account * ac)
{
    putObject(account_db, ac->asObject(), ac->fullid.c_str());
}

Atlas::Message::Object Persistance::getObject(Db & db, const char * key)
{
    // FIXME
    return Atlas::Message::Object();
}

bool Persistance::putObject(Db & db, const Atlas::Message::Object & o, const char * keystr)
{
    std::strstream str;

    Atlas::Codecs::XML codec(str, &m_d);
    Atlas::Message::Encoder enc(&codec);

    enc.StreamMessage(o);

    Dbt key, data;

    key.set_data((void*)keystr);
    key.set_size(strlen(keystr) + 1);

    data.set_data((void*)str.str());
    data.set_size(str.pcount() + 1);

    int err;
    if ((err = db.put(NULL, &key, &data, 0)) != 0) {
        cout << "db.put.ERROR! " << err << endl << flush;
        return false;
    }
    return true;
}

#else // HAVE_LIBDB_CXX

Persistance::Persistance() { }

Account * Persistance::load_admin_account()
{
    // Eventually this should actually load the account. For now it just
    // creates it.
    Account * adm = new Admin(NULL, "admin", "test");
    save_admin_account(adm);
    return(adm);
}

void Persistance::save_admin_account(Account * adm)
{
    std::ofstream adm_file("/tmp/admin.xml", ios::out, 0600);
    adm_file << "<atlas>" << endl << "<map>" << endl;
    adm_file << "    <string name=\"password\">" << adm->password << "</string>" << endl;
    adm_file << "    <string name=\"id\">" << adm->fullid << "</string>" << endl;
    adm_file << "    <list name=\"parents\">" << endl;
    adm_file << "    <string>admin</string>" << endl;
    adm_file << "    </list>" << endl;
    adm_file << "</map>" << endl << "</atlas>" << endl << flush;
    adm_file.close();
}

Account * Persistance::getAccount(const std::string & name)
{
    return NULL;
}

void Persistance::putAccount(const Account * ac) { }

bool Persistance::init()
{
    instance();
    return true;
}

#endif // HAVE_LIBDB_CXX
