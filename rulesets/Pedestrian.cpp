// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2000,2001 Alistair Riddoch

#include <Atlas/Objects/Operation/Move.h>

#include <common/Tick.h>

#include "Character.h"
#include "Pedestrian.h"

#include <common/const.h>
#include <common/debug.h>

using Atlas::Message::Object;

static const bool debug_flag = false;

Pedestrian::Pedestrian(Character & body) : Movement(body)
{
}

Pedestrian::~Pedestrian()
{
}

double Pedestrian::getTickAddition(const Vector3D & coordinates) const
{
    // This may seem a little weird. Everything is handled in squares to
    // reduce the number of square roots that have to be calculated. In
    // this case only one is required.
    double basic_square_distance = m_velocity.relMag()
                                   * consts::square_basic_tick;
    const Vector3D & target = m_collPos ? m_collPos : m_targetPos;
    if (target) {
        double square_distance = coordinates.relativeDistance(target);
        debug( std::cout << "basic_distance: " << basic_square_distance
                         << std::endl << std::flush;);
        debug( std::cout << "distance: " << square_distance << std::endl
                         << std::flush;);
        if (basic_square_distance > square_distance) {
            debug( std::cout << "\tshortened tick" << std::endl << std::flush;);
            return sqrt(square_distance / basic_square_distance)
                        * consts::basic_tick;
        }
    }
    return consts::basic_tick;
}

Move * Pedestrian::genFaceOperation(const Location & loc)
{
    if (m_orientation != loc.orientation) {
        m_orientation = loc.orientation;
        debug( std::cout << "Turning" << std::endl << std::flush;);
        Move * moveOp = new Move(Move::Instantiate());
        moveOp->SetTo(m_body.getId());
        Object::MapType entmap;
        entmap["id"] = m_body.getId();
        loc.addToObject(entmap);
        Object::ListType args(1,entmap);
        moveOp->SetArgs(args);
        return moveOp;
    }
    return NULL;
}

Move * Pedestrian::genMoveOperation(Location * rloc)
{
    return genMoveOperation(rloc, m_body.location);
}

Move * Pedestrian::genMoveOperation(Location * rloc, const Location & loc)
{
    debug(std::cout << "genMoveOperation: Pedestrian(" << m_serialno << ","
               << m_collPos << "," << m_targetPos << "," << m_velocity << ","
               << m_lastMovementTime << ")" << std::endl << std::flush;);
    if (!updateNeeded(loc)) {
        debug( std::cout << "No update needed" << std::endl << std::flush; );
        return NULL;
    }

    debug(std::cout << "genMoveOperation: Update needed..." << std::endl
                    << std::flush;);

    // Sort out time difference, and set updated time
    const double & current_time = m_body.world->getTime();
    double time_diff = current_time - m_lastMovementTime;
    debug( std::cout << "time_diff:" << time_diff << std::endl << std::flush;);
    m_lastMovementTime = current_time;

    m_orientation = loc.orientation;
    
    Location new_loc(loc);
    new_loc.velocity = m_velocity;

    // Create move operation
    Move * moveOp = new Move(Move::Instantiate());
    moveOp->SetTo(m_body.getId());

    // Set up argument for operation
    Object::MapType entmap;
    entmap["id"] = m_body.getId();

    // Walk out what the mode of the character should be.
    // Performed in squares to save on that critical sqrt() call
    double vel_square_mag = m_velocity.relMag();
    double square_speed_ratio;
    if (vel_square_mag == 0.0) {
        square_speed_ratio = 0.0;
    } else {
        square_speed_ratio = vel_square_mag / consts::square_base_velocity;
    }
    std::string mode;
    if (square_speed_ratio > 0.25) { // 0.5 ^ 2
        mode = std::string("running");
    } else if (square_speed_ratio > 0.0025) { // 0.05 ^ 2
        mode = std::string("walking");
    } else {
        mode = std::string("standing");
    }
    debug( std::cout << "Mode set to " << mode << std::endl << std::flush;);
    entmap["mode"] = Object(mode);

    // If velocity is not set, return this simple operation.
    if (!m_velocity) {
        debug( std::cout << "only velocity changed." << std::endl
                         << std::flush;);
        new_loc.addToObject(entmap);
        Object::ListType args(1,entmap);
        moveOp->SetArgs(args);
        if (NULL != rloc) {
        *rloc = new_loc;
        }
        return moveOp;
    }

    // Update location
    Vector3D new_coords = m_updatedPos ? m_updatedPos : loc.coords;
    new_coords += Vector3D(m_velocity) *= time_diff;
    const Vector3D & target = m_collPos ? m_collPos : m_targetPos;
    if (target) {
        Vector3D new_coords2 = new_coords;
        new_coords2 += Vector3D(m_velocity) *= (consts::basic_tick / 10.0);
        // The values returned by relativeDistance are squares, so
        // cannot be used except for comparison
        double dist = target.relativeDistance(new_coords);
        double dist2 = target.relativeDistance(new_coords2);
        debug( std::cout << "dist: " << dist << "," << dist2 << std::endl
                         << std::flush;);
        if (dist2 > dist) {
            debug( std::cout << "target achieved";);
            new_coords = target;
            if (m_collRefChange) {
                debug(std::cout << "CONTACT " << m_collEntity->getId()
                                << std::endl << std::flush;);
                if (m_collEntity == new_loc.ref->location.ref) {
                    debug(std::cout << "OUT" << target
                                    << new_loc.ref->location.coords
                                    << std::endl << std::flush;);
                    new_coords += new_loc.ref->location.coords;
                    if (m_targetPos) {
                        m_targetPos += new_loc.ref->location.coords;
                    }
                } else {
                    debug(std::cout << "IN" << std::endl << std::flush;);
                    new_coords -= m_collEntity->location.coords;
                    if (m_targetPos) {
                        m_targetPos -= m_collEntity->location.coords;
                    }
                }
                new_loc.ref = m_collEntity;
                m_collEntity = NULL;
                m_collRefChange = false;
                m_collPos = Vector3D();
            } else {
                if (m_collPos) {
                    // Generate touch ops
                    m_velocity[m_collAxis] = 0;
                    m_collPos = Vector3D();
                    if ((m_velocity.mag() / consts::base_velocity) > 0.05) {
                        new_loc.orientation = Quaternion(Vector3D(1,0,0), m_velocity.unitVector());
                    } else {
                        reset();
                        entmap["mode"] = Object("standing");
                    }
                } else {
                    reset();
                    entmap["mode"] = Object("standing");
                }
                new_loc.velocity = m_velocity;
            }
        }
    }
    new_loc.coords = new_coords;
    m_updatedPos = new_coords;

    // Check for collisions
    checkCollisions(new_loc);

    debug( std::cout << "new coordinates: " << new_coords << std::endl
                     << std::flush;);
    new_loc.addToObject(entmap);
    Object::ListType args2(1,entmap);
    moveOp->SetArgs(args2);
    if (NULL != rloc) {
        *rloc = new_loc;
    }
    return moveOp;
}
