/*
 * opencog/cogserver/modules/commands/ListRequest.cc
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "ListRequest.h"

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/atom_types/NameServer.h>
#include <opencog/atoms/atom_types/types.h>
#include <opencog/cogserver/server/CogServer.h>

using namespace opencog;

ListRequest::ListRequest(CogServer& cs) : Request(cs)
{
}

ListRequest::~ListRequest()
{
    logger().debug("[ListRequest] destructor");
}

bool ListRequest::syntaxError()
{
    _error << "invalid syntax" << std::endl;
    sendError();
    return false;
}

bool ListRequest::execute()
{
    std::string name = "";
    Type type = NOTYPE;
    long max_size = -1;
    Handle handle = Handle::UNDEFINED;
    bool subtypes = false;
    AtomSpace& as = _cogserver.getAtomSpace();

    if (_parameters.empty()) {
        _error << "Error: option required" << std::endl;
        sendError();
        return false;
    }

    std::list<std::string>::const_iterator it;
    for (it = _parameters.begin(); it != _parameters.end(); ++it) {
        if (*it == "-a") { // list everything
            type = NOTYPE;
            handle = Handle::UNDEFINED;
            subtypes = false;
            break;
        } else if (*it == "-n")  { // filter by name
            ++it;
            if (it == _parameters.end()) return syntaxError();
            name.assign(*it);
        } else if (*it == "-t") { // filter by type, excluding subtypes
            ++it;
            if (it == _parameters.end()) return syntaxError();
            type = nameserver().getType(it->c_str());
            if (type == NOTYPE) {
                _error << "Error: Invalid type" << std::endl;
                sendError();
                return false;
            }
        } else if (*it == "-T") { // filter by type, including subtypes
            ++it;
            if (it == _parameters.end()) return syntaxError();
            type = nameserver().getType(it->c_str());
            if (type == NOTYPE) {
                _error << "invalid type" << std::endl;
                sendError();
                return false;
            }
            subtypes = true;
        } else if (*it == "-m") { // list only the N first atoms
            ++it;
            if (it == _parameters.end()) return syntaxError();
            max_size = atol(it->c_str());
        } else {
            _error << "Error: unknown option \"" << *it <<"\"" << std::endl;
            sendError();
            return false;
        }
    }
    if (name != "" && type != NOTYPE) { // filter by name & type
        nameserver().foreachRecursive(
            [&](Type t)->void {
                 try {
                     Handle h(as.get_handle(t, name));
                     if (h) _handles.push_back(h);
                 } catch (const std::exception& e) {}
            }, type);

    } else if (name != "") {     // filter by name
        nameserver().foreachRecursive(
            [&](Type t)->void {
                 try {
                     Handle h(as.get_handle(t, name));
                     if (h) _handles.push_back(h);
                 } catch (const std::exception& e) {}
            }, NODE);

    } else if (type != NOTYPE) { // filter by type
        as.get_handles_by_type(_handles, type, subtypes);
    } else {
        as.get_handles_by_type(_handles, ATOM, true);
    }

    // Remove the bottom handles
    if (max_size > 0 && max_size < (long)_handles.size())
        _handles.resize(max_size);

    sendOutput();
    return true;
}

void ListRequest::sendOutput()
{
    std::ostringstream oss;
    for (const Handle& h : _handles) {
        oss << h->to_string() << std::endl;
    }
    send(oss.str());
}

void ListRequest::sendError()
{
    _error << "Supported options:" << std::endl;
    _error << "-a          List all atoms" << std::endl;
    _error << "-n name     List all atoms with name" << std::endl;
    _error << "-t type     List all atoms of type" << std::endl;
    _error << "-T type     List all atoms with type or subtype" << std::endl;
    _error << "-m type     List all atoms up to a specified size" << std::endl;
    _error << "Options may be combined" << std::endl;
    send(_error.str());
}
