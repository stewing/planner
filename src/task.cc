
#include "task.h"
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <algorithm>
#include <assert.h>

task::name_lookup task::by_name;
task::id_lookup task::by_id;

const char* task::_state_str[] = {
    "not started",
    "running",
    "complete",
    "no_resource"
};

using namespace boost;

namespace {
    bool
    task_incomplete(task* t)
    {
        return t->get_state() != task::complete;
    }
}

task::task(const char* name, const uint64_t& reqd_cores, const uint64_t& reqd_ticks)
    : _name(name), _reqd_cores(reqd_cores), _reqd_ticks(reqd_ticks),
    _ticks_remaining(reqd_ticks), _id(next_id()), _state(not_started), _mapped_deps(false), _waiters(0)
{
    _register_task(this);
}

task::~task()
{
    _deregister_task(this); 
}

task::tick_stat
task::run_for(const uint64_t& ticks)
{
    task::tick_stat t;

    if (_ticks_remaining == ticks) {
        // completed
        t.remaining_ticks = 0; 
        t.busy_ticks = ticks;
        t.idle_ticks = 0;
        _state = complete;
        _ticks_remaining = 0;
    } else if (_ticks_remaining < ticks) {
        // completed with extra, idle ticks
        t.remaining_ticks = 0; 
        t.busy_ticks = ticks;
        t.idle_ticks = ticks - _ticks_remaining;
        _state = complete;
        _ticks_remaining = 0;
    } else if (_ticks_remaining > ticks) {
        // not complete
        t.remaining_ticks = _ticks_remaining - ticks;
        t.busy_ticks = ticks;
        t.idle_ticks = 0;
        _ticks_remaining -= ticks;
    }
    return t;
}

uint64_t
task::get_ticks_remaining() const
{
    return _ticks_remaining;
}

bool
task::dependencies_met() const
{
    task::ptr_list::const_iterator itr(
            std::find_if(_deps.begin(), _deps.end(), task_incomplete));
    return itr == _deps.end();
}

// Parse the dependency string for dependency mapping.
std::vector<std::string>
task::_get_dep_str() const
{
    std::vector<std::string> deps;
    if (_dep_str.empty()) {
        return deps;
    }
    split(deps, _dep_str, is_any_of(","));
    std::for_each(deps.begin(), deps.end(),
            bind(&trim<std::string>, _1, std::locale()));
    return deps;
}

// 
// Dependency mapping - map depencies at the depencency level  
//
// 1. get a list of dependency names
// 2. look up that dependency by name
// 3. inform the other task that we're waiting for it
// 4. store the pointer in this class
//
bool
task::map_dependencies()
{
    bool found_all = true;
    std::vector<std::string> deps = _get_dep_str();
    for (std::vector<std::string>::iterator itr(deps.begin());
            itr != deps.end();
            ++itr) {
        task* t(task::lookup_task(*itr));
        if (t) {
            // tell the other task that we're waiting on it
            t->_incr_waiters(this);
            _deps.push_back(t);
        } else {
            found_all = false;
        }
    }
    _mapped_deps = true;
    return found_all;
}

uint64_t
task::get_cores_required() const
{
    return _reqd_cores;
}

uint64_t
task::get_waiter_count() const
{
    return _waiters;
}

uint64_t
task::_incr_waiters(task* w)
{
    _waiter_list.push_back(w);
    return _waiters++;
}

task::ptr_list
task::get_waiter_list() const
{
    return _waiter_list;
}

uint64_t
task::get_dependency_count() const
{
    if (!_mapped_deps) {
        return -1;
    }
    return _deps.size();
}

task::ptr_list
task::get_dependencies() const
{
    return _deps;
}

uint64_t
task::get_id() const
{
    return _id;
}

task::state
task::get_state() const
{
    return _state;
}

void
task::set_state(task::state s)
{
    _state = s;
    assert(_state != task::not_started);
}

std::string
task::get_name() const
{
    return _name;
}

void
task::set_dep_str(const char* deps)
{
    if (deps) {
        _dep_str = deps;
    } else {
        _dep_str = "";
    }
}

// statics 

task*
task::lookup_task(id_t id)
{
    return by_id[id];
}

task*
task::lookup_task(const std::string& name)
{
    name_lookup::iterator itr = by_name.find(name);
    if (itr == by_name.end()) {
        return NULL;
    }
    return itr->second;
}

void
task::_register_task(task* t)
{
    if (by_id.size() <= t->get_id())
    {
        by_id.resize(t->get_id()+1);
    }
    by_id[t->get_id()] = t;
    std::pair<name_lookup::iterator, bool> rc
        (by_name.insert(std::make_pair(t->get_name(), t)));
    assert(rc.second);
}

void
task::_deregister_task(task* t)
{
    by_id[t->get_id()] = NULL;
    size_t erased(by_name.erase(t->get_name()));
    assert(erased == 1);
}

// friend ostream operator

std::ostream&
operator<<(std::ostream& os, const task& tsk)
{
    os << "name: " << tsk._name << "; cores_required: " << tsk._reqd_cores <<
        "; exec_time: " << tsk._ticks_remaining << "/" << tsk._reqd_ticks <<
        "; _id: " << tsk._id << "; state: " << task::_state_str[tsk._state] <<
        "(" << tsk._state << "); dependency count: " << tsk._deps.size() <<
        "; waiters: " << tsk._waiters;
    if (!tsk._dep_str.empty()) {
        os << "; parent tasks: " << tsk._dep_str;
    }
    return os;
}
