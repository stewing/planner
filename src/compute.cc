
#include "compute.h"
#include <yaml.h>
#include <iostream>

compute::compute(const std::string& name, const uint64_t& cores) 
    : _name(name), _cores_total(cores),
    _cores_available(cores), _cumulative_busy_ticks(0),
    _cumulative_idle_ticks(0), _completed_tasks(0), _state(free), _assign_count(0)
{
} 

// assign_task -- take ownership of this task, allocate resources
void
compute::assign_task(task* t)
{
    assert(t->get_state() == task::not_started);
    t->set_state(task::running);
    _current_tasks.push_back(t);
    _cores_available -= t->get_cores_required();
    assert(_cores_available >= 0);
    ++_assign_count;
}

uint64_t
compute::get_assign_count() const
{
    return _assign_count;
}

//
// tick -- run the tasks associated with this compute resource
//
// Since this is a simulation, we run many ticks at once keep
// the numbers in order.
//
// For each task:
//    1. "Run" the task
//    2. record how time was spent
//    3. clean up tasks that have completed
//    4. when done with tasks, check timings for consistency
//
int64_t
compute::tick(uint64_t ticks)
{
    assert(ticks>0);
    int64_t cores_used(0);
    int64_t tasks_completed(0);
    uint64_t this_run_idle_ticks(0);
    uint64_t this_run_busy_ticks(0);
    for (task::ptr_llist::iterator task_itr(_current_tasks.begin());
                task_itr != _current_tasks.end();
                ) {
        // run the task
        task::tick_stat ts((*task_itr)->run_for(ticks));

        // account for idle time and core use
        cores_used += (*task_itr)->get_cores_required();
        this_run_idle_ticks += ts.idle_ticks * (*task_itr)->get_cores_required();
        this_run_busy_ticks += ts.busy_ticks * (*task_itr)->get_cores_required();

        // task completed:
        //   put the cores back in service
        //   remove the task from our current execution list
        //   increment the completed counter
        if (ts.remaining_ticks == 0) {
            task::ptr_llist::iterator rem(task_itr);
            _cores_available += (*task_itr)->get_cores_required();
            assert(_cores_available <= _cores_total);
            ++task_itr;
            _current_tasks.erase(rem);
            ++tasks_completed;
            continue;
        }
        // non-completed task
        ++task_itr;
    }
    // account for totally idle cores
    assert(_cores_total >= cores_used);
    this_run_idle_ticks = (_cores_total - cores_used) * ticks;

    // verify that no ticks are missing
    assert((_cores_total * ticks) == (this_run_idle_ticks + this_run_busy_ticks));

    _cumulative_idle_ticks += this_run_idle_ticks;
    _cumulative_busy_ticks += this_run_busy_ticks;
    _completed_tasks += tasks_completed;
    return tasks_completed;
}

// getters

std::string
compute::get_name() const
{
    return _name;
}

compute::state
compute::get_state() const
{
    return _state;
}

int64_t
compute::get_cores_available() const
{
    return _cores_available;
}

uint64_t
compute::get_cores() const
{
    return _cores_total;
}

uint64_t
compute::get_busy_ticks() const
{
    return _cumulative_busy_ticks;
}
uint64_t
compute::get_idle_ticks() const
{
    return _cumulative_idle_ticks;
}
uint64_t
compute::get_total_ticks() const
{
    return _cumulative_idle_ticks + _cumulative_busy_ticks;
}

// friend ostream operator

std::ostream&
operator<<(std::ostream& os, const compute& comp)
{
    os << "name: " << comp._name << "; cores: " << comp._cores_available << "/"
        << comp._cores_total << "; state: " << comp._state;
    for (task::ptr_llist::const_iterator itr(comp._current_tasks.begin());
            itr != comp._current_tasks.end();
            ++itr) {
        os << "\n\t" << *itr;
    }
    return os;
}

