#include <Atlas/Message/Object.h>
#include <Atlas/Objects/Root.h>
#include <Atlas/Objects/Operation/Login.h>
#include <Atlas/Objects/Operation/Create.h>
#include <Atlas/Objects/Operation/Sight.h>
#include <Atlas/Objects/Operation/Set.h>
#include <Atlas/Objects/Operation/Delete.h>
#include <Atlas/Objects/Operation/Move.h>
#include <Atlas/Objects/Operation/Sound.h>
#include <Atlas/Objects/Operation/Touch.h>
#include <Atlas/Objects/Operation/Look.h>

#include "Thing.h"
#include "MemMap_methods.h"
#include "Python_API.h"

#include <server/WorldRouter.h>

#include <common/const.h>

static int debug_thing = 0;

Thing::Thing() : script_object(NULL), status(1), is_character(0), type("thing")
{
    in_game = 1;
    name=string("Foo");
    attributes["age"] = 0;
    attributes["mode"] = Message::Object("birth");
    attributes["weight"] = (double)-1;
    attributes["description"] = Message::Object("Some Thing");
}

int Thing::script_Operation(const string & op_type, const RootOperation & op,
                     oplist & ret_list, RootOperation * sub_op)
{
    if (script_object != NULL) {
        debug_thing && cout << "Got script object for " << fullid << endl << flush;
        string op_name = op_type+"_operation";
        // Construct apropriate python object thingies from op
        if (!PyObject_HasAttrString(script_object, (char *)(op_name.c_str()))) {
            debug_thing && cout << "No method to be found for " << fullid
                 << "." << op_name << endl << flush;
            return(0);
        }
        RootOperationObject * py_op = newAtlasRootOperation(NULL);
        py_op->operation = new RootOperation(op);
        py_op->own = 0;
        py_op->from = map.get_add(op.GetFrom());
        py_op->to = map.get_add(op.GetTo());
        PyObject * ret;
        if (sub_op == NULL) {
            ret = PyObject_CallMethod(script_object, (char *)(op_name.c_str()),
                                             "(O)", py_op);
        } else {
            RootOperationObject * py_sub_op = newAtlasRootOperation(NULL);
            py_sub_op->operation = sub_op;
            py_sub_op->own = 0;
            py_sub_op->from = map.get_add(sub_op->GetFrom());
            py_sub_op->to = map.get_add(sub_op->GetTo());
            ret = PyObject_CallMethod(script_object, (char *)(op_name.c_str()),
                                             "(OO)", py_op, py_sub_op);
            Py_DECREF(py_sub_op);
        }
        delete py_op->operation;
        Py_DECREF(py_op);
        if (ret != NULL) {
            debug_thing && cout << "Called python method " << op_name
                                << " for object " << fullid << endl << flush;
            if ((PyTypeObject*)PyObject_Type(ret) == &RootOperation_Type) {
                RootOperationObject * op = (RootOperationObject*)ret;
                if (op->operation != NULL) {
                    ret_list.push_back(op->operation);
                    op->own = 0;
                } else {
                    debug_thing && cout << "Method returned invalid operation"
                         << endl << flush;
                }
            } else if ((PyTypeObject*)PyObject_Type(ret) == &Oplist_Type) {
                OplistObject * op = (OplistObject*)ret;
                if (op->ops != NULL) {
                    ret_list = *op->ops;
                } else {
                    debug_thing && cout << "Method returned invalid oplist"
                         << endl << flush;
                }
            } else {
                debug_thing && cout << "Method returned invalid object" << endl << flush;
            }
            
            Py_DECREF(ret);
            return(1);
        } else {
            if (PyErr_Occurred() == NULL) {
                debug_thing && cout << "No method to be found for " << fullid << endl << flush;
            } else {
                cerr << "Reporting python error for " << fullid << endl << flush;
                PyErr_Print();
            }
        }
    } else {
        debug_thing && cout << "No script object asociated" << endl << flush;
    }
    return(0);
}

#if 0

Message::Object & Thing::operator[](const string & name)
{
    if (attributes.find(name) == attributes.end()) {
        debug_thing && cout << "SETTING NEW" << name << endl << flush;
        attributes[name]=Message::Object();
    }
    return(attributes[name]);
}

oplist Thing::send_world(RootOperation * msg)
{
    return world->message(*msg, this);
}

#endif

void Thing::addObject(Message::Object * obj)
{
    Message::Object::MapType & omap = obj->AsMap();
    omap["name"] = Message::Object(name);
    omap["type"] = Message::Object(type);
    omap["parents"] = Message::Object(Message::Object::ListType(1,Message::Object(type)));
    omap.insert(attributes.begin(), attributes.end());
    location.addObject(obj);
    BaseEntity::addObject(obj);
}

void Thing::merge(const Message::Object::MapType & entmap)
{
    Message::Object::MapType::const_iterator I;
    for (I=entmap.begin(); I!=entmap.end(); I++) {
        const string & key = I->first;
        if ((key == "name") || (key == "id") || (key == "parents")) continue;
        if ((key == "pos") || (key == "loc") || (key == "velocity")) continue;
        if ((key == "face") || (key == "contains")) continue;
        attributes[key] = I->second;
    }
}

void Thing::getLocation(Message::Object::MapType & entmap, fdict_t & fobjects)
{
    debug_thing && cout << "Thing::getLocation" << endl << flush;
    if (entmap.find("loc") != entmap.end()) {
        debug_thing && cout << "Thing::getLocation, getting it" << endl << flush;
        try {
            const string & parent_id = entmap["loc"].AsString();
            BaseEntity * parent_obj;
            if (fobjects.find(parent_id) == fobjects.end()) {
                debug_thing && cout << "ERROR: Can't get parent from objects dictionary" << endl << flush;
                return;
            }
                
            parent_obj = fobjects[parent_id];
            Vector3D pos(0, 0, 0);
            Vector3D velocity(0, 0, 0);
            Vector3D face(1, 0, 0);
            if (entmap.find("pos") != entmap.end()) {
                pos = Vector3D(entmap["pos"].AsList());
            } else if (location) {
                pos = location.coords;
            }
            if (entmap.find("velocity") != entmap.end()) {
                velocity = Vector3D(entmap["velocity"].AsList());
            } else if (location) {
                velocity = location.velocity;
            }
            if (entmap.find("face") != entmap.end()) {
                face = Vector3D(entmap["face"].AsList());
            } else if (location) {
                face = location.face;
            }
            Location thing_loc(parent_obj, pos, velocity, face);
            location = thing_loc;
        }
        catch (Message::WrongTypeException) {
            cerr << "ERROR: Create operation has bad location" << endl << flush;
        }
    }

}

oplist Thing::Operation(const Setup & op)
{
    oplist res;
    if (script_Operation("setup", op, res) != 0) {
        return(res);
    }
    RootOperation * tick = new Tick;
    *tick = Tick::Instantiate();
    tick->SetTo(fullid);
    return(oplist(1,tick));
}

oplist Thing::Operation(const Tick & op)
{
    oplist res;
    if (script_Operation("tick", op, res) != 0) {
        return(res);
    }
    return(res);
}

oplist Thing::Operation(const Create & op)
{
    oplist res;
    if (script_Operation("create", op, res) != 0) {
        return(res);
    }
    const Message::Object::ListType & args=op.GetArgs();
    if (args.size() == 0) {
       return(res);
    }
    try {
        Message::Object::MapType ent = args.front().AsMap();
        if (ent.find("parents") == ent.end()) {
            return error(op, "Object to be created has no type");
        }
        Message::Object::ListType & parents = ent["parents"].AsList();
        string type;
        if (parents.size() < 1) {
            type = "thing";
        } else {
            type = parents.front().AsString();
        }
        debug_thing && cout << fullid << " creating " << type;
        Thing * obj = world->add_object(type,ent);
        if (!obj->location) {
            obj->location=location;
            obj->location.velocity=Vector3D(0,0,0);
        }
        if (obj->location.parent != NULL) {
            obj->location.parent->contains.push_back(obj);
            obj->location.parent->contains.unique();
        }
        Create c(op);
        list<Message::Object> args2(1,obj->asObject());
        c.SetArgs(args2);
        RootOperation * s = new Sight();
        *s = Sight::Instantiate();
        list<Message::Object> args3(1,c.AsObject());
        s->SetArgs(args3);
        res.push_back(s);
    }
    catch (Message::WrongTypeException) {
        cerr << "EXCEPTION: Malformed object to be created\n";
        return(error(op, "Malformed object to be created\n"));
    }
    return(res);
}

oplist Thing::Operation(const Delete & op)
{
    oplist res;
    if (script_Operation("delete", op, res) != 0) {
        return(res);
    }
    world->del_object(this);
    RootOperation * s = new Sight;
    *s = Sight::Instantiate();
    Message::Object::ListType args(1,op.AsObject());
    s->SetArgs(args);
    return(oplist(1,s));
}

oplist Thing::Operation(const Move & op)
{
    debug_thing && cout << "Thing::move_operation" << endl << flush;
    oplist res;
    if (script_Operation("move", op, res) != 0) {
        return(res);
    }
    const Message::Object::ListType & args=op.GetArgs();
    if (args.size() == 0) {
        debug_thing && cout << "ERROR: move op has no argument" << endl << flush;
        return(res);
    }
    BaseEntity * newparent;
    try {
        Message::Object::MapType ent = args.front().AsMap();
        string & oname = ent["id"].AsString();
        debug_thing && cout << "In " << fullid << " got moveop for " << oname << endl << flush;
        if (ent.find("loc") == ent.end()) {
            debug_thing && cout << "ERROR: move op arg has no parent" << endl << flush;
            return(error(op, "Move location has no parent"));
        }
#if USE_OLD_LOC
        Message::Object::MapType lmap = ent["loc"].AsMap();
        string parent = lmap["ref"].AsString();
        debug_thing && cout << "Got old style ref in move op" << endl << flush;
#else
        string parent=ent["loc"].AsString();
        if (world->fobjects.find(parent) == world->fobjects.end()) {
            debug_thing && cout << "ERROR: move op arg parent is invalid" << endl << flush;
            return(error(op, "Move location parent invalid"));
        }
        newparent = world->fobjects[parent];
        if (location.parent != newparent) {
            location.parent->contains.remove(this);
            newparent->contains.push_back(this);
        }
#endif
        location.parent=newparent;
#if USE_OLD_LOC
        if (lmap.find("coords") == lmap.end()) {
            return(error(op, "Move location has no position"));
        }
        Message::Object::ListType vector = lmap["coords"].AsList();
        if (vector.size()!=3) {
            return(error(op, "Move location pos is malformed"));
        }
        double x = vector.front().AsFloat();
        vector.pop_front();
        double y = vector.front().AsFloat();
        vector.pop_front();
        double z = vector.front().AsFloat();
        location.coords = Vector3D(x, y, z);
        if (lmap.find("velocity") == lmap.end()) {
            return(error(op, "Move location has no velocity"));
        }
        vector.clear();
        vector = lmap["velocity"].AsList();
        if (vector.size()!=3) {
            return(error(op, "Move location velocity is malformed"));
        }
        x = vector.front().AsFloat();
        vector.pop_front();
        y = vector.front().AsFloat();
        vector.pop_front();
        z = vector.front().AsFloat();
        location.velocity = Vector3D(x, y, z);
#else
        if (ent.find("pos") == ent.end()) {
            return(error(op, "Move location has no position"));
        }
        Message::Object::ListType vector = ent["pos"].AsList();
        if (vector.size()!=3) {
            return(error(op, "Move location pos is malformed"));
        }
        double x = vector.front().AsFloat();
        vector.pop_front();
        double y = vector.front().AsFloat();
        vector.pop_front();
        double z = vector.front().AsFloat();
        debug_thing && cout << "POS: " << x << " " << y << " " << z << endl << flush;
        location.coords = Vector3D(x, y, z);
        if (ent.find("velocity") != ent.end()) {
            vector.clear();
            vector = ent["velocity"].AsList();
            if (vector.size()!=3) {
                cerr << "ERROR: Move location velocity is malformed";
                return(error(op, "Move location velocity is malformed"));
            }
            x = vector.front().AsFloat();
            vector.pop_front();
            y = vector.front().AsFloat();
            vector.pop_front();
            z = vector.front().AsFloat();
            debug_thing && cout << "VEL: " << x << " " << y << " " << z << endl << flush;
            location.velocity = Vector3D(x, y, z);
        }
#endif
        debug_thing && cout << "MOVE calculate vel=" << location.velocity
             << " coord=" << location.coords;

        double speed_ratio;
        if (!(location.velocity)) {
            speed_ratio = 0.0;
        } else {
            speed_ratio = location.velocity.mag()/consts::base_velocity;
        }
        RootOperation * s = new Sight;
        *s = Sight::Instantiate();
        Message::Object::ListType args2(1,op.AsObject());
        s->SetArgs(args2);
        res.push_back(s);
        // I think it might be wise to send a set indicating we have changed
        // modes
    }
    catch (Message::WrongTypeException) {
        cerr << "EXCEPTION: Malformed object to be moved\n";
        return(error(op, "Malformed object to be moved\n"));
    }
    return(res);
}

oplist Thing::Operation(const Set & op)
{
    oplist res;
    if (script_Operation("set", op, res) != 0) {
        return(res);
    }
    const Message::Object::ListType & args=op.GetArgs();
    if (args.size() == 0) {
       return(res);
    }
    try {
        Message::Object::MapType ent = args.front().AsMap();
        Message::Object::MapType::const_iterator I;
        for (I = ent.begin(); I != ent.end(); I++) {
            if (I->first == "id") continue;
            if (I->first == "status") {
                status = I->second.AsFloat();
            } else {
                attributes[I->first] = I->second;
            }
        }
        RootOperation * s = new Sight();
        *s = Sight::Instantiate();
        Message::Object::ListType args2(1,op.AsObject());
        s->SetArgs(args2);
        res.push_back(s);
        if (status < 0) {
            RootOperation * d = new Delete();
            *d = Delete::Instantiate();
            Message::Object::ListType args3(1,this->asObject());
            d->SetArgs(args3);
            d->SetTo(fullid);
            res.push_back(d);
        }
    }
    catch (Message::WrongTypeException) {
        cerr << "EXCEPTION: Malformed set operation\n";
        return(error(op, "Malformed set operation\n"));
    }
    return(res);
}

oplist Thing::Operation(const Sight & op)
{
    oplist res;
    script_Operation("sight", op, res);
    return(res);
}

oplist Thing::Operation(const Sound & op)
{
    oplist res;
    script_Operation("sound", op, res);
    return(res);
}

oplist Thing::Operation(const Touch & op)
{
    oplist res;
    script_Operation("touch", op, res);
    return(res);
}

oplist Thing::Operation(const Look & op)
{
    oplist res;
    if (script_Operation("look", op, res) != 0) {
        return(res);
    }
    return(BaseEntity::Operation(op));
}
