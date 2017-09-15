
#include "planner.h"

#include <algorithm>
#include <assert.h>
#include <boost/graph/topological_sort.hpp>
#include <iostream>
#include <iterator>
#include <unistd.h>
#include <utility>

const char* planner::status_str[] = {
    "Ok",
    "Core capacity exceeded by task input.",
    "Missing dependency in task description.",
    "Circular dependency in task description."
};

using namespace boost;

namespace {
    //
    // statics and predicates
    //

    // This predicate defines the priority for scheduling runnable tasks
    // on available cores.
    bool runnable_task_sort(const task* rt, const task* lt)
    {
        if (rt->get_cores_required() == lt->get_cores_required()) {
            return rt->get_waiter_count() < lt->get_waiter_count();
        }
        return rt->get_cores_required() < lt->get_cores_required();
    }

    template<typename T>
    bool sort_max_cores(T rhs, T lhs) 
    {
        return rhs->get_cores_available() < lhs->get_cores_available();
    }

    template<typename T>
    void print_list(const T& cont, const std::string& sep)
    {
        for (typename T::const_iterator itr(cont.begin());
                itr != cont.end();
                ++itr) {
            std::cout << **itr << sep;
        }
    }
}

// planner and schedule_intry constructors
planner::schedule_entry::schedule_entry(task* t, compute* c)
    : _task(t), _compute(c)
{
}

planner::planner(compute::list* comp, task::list* task)
    : _comp(comp), _tasks(task), _tasks_validated(false), _required_ticks(0),
    _count_dep_wait(0), _count_comp_unavail(0), _all_cores_busy(0), _last_task(0)
{                                                                             
}

//
// validate_tasks -- check all tasks and compute for validity, build initial schedule
//
// 1. find the max core count; we can't run tasks that exceed this number
// 2. for each task,
//   3. verify core count is ok
//   4. map the task dependencies
//   5. create graph edges representing each task and its dependencies
// 6. add all dependency edges to the graph
// 7. perform a topological sort
//
planner::status
planner::validate_tasks()
{
    // find the most compute we have on any compute resource
    compute::list::const_iterator max_comp(std::max_element(
            _comp->begin(), _comp->end(), sort_max_cores<compute::ptr>));

    task::ptr_list disconnected_nodes;
    task* artificial_dep = NULL;

    _edge.reserve(_tasks->size()*4);
    for (task::list::iterator itr(_tasks->begin()) ; 
            itr != _tasks->end();
            ++itr) {

        // validate that cores requirements aren't exceeded
        if ((*itr)->get_cores_required() > (*max_comp)->get_cores()) {
            return compute_exceeded;
        }
        
        // validate that task dependencies are met
        bool ok = (*itr)->map_dependencies();
        if (!ok) {
            _last_task = (*itr).get();
            return missing_dependency;
        }

        // add all edges to the list
        task::ptr_list all_deps((*itr)->get_dependencies()); 
        for (task::ptr_list::const_iterator dep(all_deps.begin());
                dep != all_deps.end();
                ++dep) {
            _edge.push_back(std::make_pair((*itr)->get_id(), (*dep)->get_id()));
        }

        // keep track of disconnected tasks
        if ((*itr)->get_waiter_count() == 0 &&
                    (*itr)->get_dependency_count() == 0) {
            disconnected_nodes.push_back(itr->get());
        }

        if (artificial_dep == NULL) {
            artificial_dep = itr->get();
        }
    }

    // add nodes that are disconnected from the graph back with an artificial
    // dependency
    for (task::ptr_list::iterator dis_itr(disconnected_nodes.begin());
            dis_itr != disconnected_nodes.end();
            ++dis_itr) {
        if (*dis_itr == artificial_dep) {
            continue;
       }
        _edge.push_back(std::make_pair((*dis_itr)->get_id(), artificial_dep->get_id()));
    }

    // add edges
    for (graph_edge_list::const_iterator itr(_edge.begin());
                itr != _edge.end();
                ++itr) {
        boost::add_edge(itr->first, itr->second, _tg);
        std::cout << "adding edge: " << itr->first << " -> " << itr->second << "\n";
    }

    // topological sort
    try {
        boost::topological_sort(_tg,
                std::back_inserter(_job_sequence));
    } catch (boost::not_a_dag e_dag) {
        return circular_dependency;
    }

    _tasks_validated = true;
    return ok;
}

// schedule_tasks -- take the validated set of tasks in dependency order and schedule them
// 
// Note, validate_tasks must have already been called.
planner::schedule_list
planner::schedule_tasks()
{
    assert(_tasks_validated);

    uint64_t tasks_remaining = _tasks->size();

    task::ptr_list runnable;
    task::ptr_llist running;
    compute::ptr_list comp_avail(_comp->size());

    while (tasks_remaining) {
        uint64_t skip_ticks = 0;

        // Assign tasks to compute resources.  This is roughly a best-fit bin packing
        // algorithm.  Here are the steps.
        //    1. Select only compute resources with at least one core available now.
        //    2. Sort the available compute resources ascending.
        //    3. Build a list of runnable tasks (not started, dependencies met).
        //    4. Sort the runnable tasks descending based on core requirements
        //       and waiters.
        //    5. Assign the largest tasks to the compute resources with the minimum
        //       necessary core availability.
        //    6. Find the next (minimum) time for any runnable task to complete.
        //    7. Run for the number of ticks found in step 6.
        //    8. Remove all completed tasks from the runnable list.
        //    9. Repeat until all tasks have completed.

        // find available compute resources
        comp_avail.clear();
        for (compute::list::iterator comp_itr(_comp->begin());
                comp_itr != _comp->end();
                ++comp_itr) {
            compute* c = comp_itr->get();
            if (c->get_cores_available() > 0) {
                comp_avail.push_back(c);
            }
        }
        // sort them by available cores
        std::sort(comp_avail.begin(), comp_avail.end(), sort_max_cores<compute*>);

        // build runnable list
        runnable.clear();
        for (planner::sched_container::iterator itr(_job_sequence.begin()) ; 
                itr != _job_sequence.end();
                ++itr) {
            // TODO many tasks could be skipped because we can never move to before
            // the last most recently runnable task.
            task* t(task::lookup_task(*itr));
            if (t->get_state() == task::not_started) {
                if (t->dependencies_met()) {
                    runnable.push_back(t); 
                } else {
                    ++_count_dep_wait;
                }
            }
        }

        // sort based on waiters and compute requirements
        std::sort(runnable.begin(), runnable.end(), runnable_task_sort);

        // assign each tasks to a compute node's cores, enter the decision in the plan
        int cores_available = comp_avail.size();
        for (task::ptr_list::reverse_iterator task_itr(runnable.rbegin());
                task_itr != runnable.rend();
                ++task_itr) {
            if ((*task_itr)->get_state() != task::not_started) {
                continue;
            }
            for (compute::ptr_list::iterator comp_itr(comp_avail.begin());
                    comp_itr != comp_avail.end() && (*comp_itr)->get_cores_available() > 0;
                    ++comp_itr) {
                if (static_cast<int64_t>((*task_itr)->get_cores_required()) <=
                        (*comp_itr)->get_cores_available()) {
                    _schedule.push_back(schedule_entry(*task_itr, *comp_itr));
                    (*comp_itr)->assign_task(*task_itr);
                    running.push_back(*task_itr);
                    if ((*comp_itr)->get_cores_available() == 0) {
                        --cores_available;
                    }
                    break;
                } else {
                    ++_count_comp_unavail;
                }
            }
            if (cores_available == 0) {
                ++_all_cores_busy;
                break;
            }
        }

        // find the smallest amount of time required to complete a task
        for (task::ptr_llist::iterator run_itr(running.begin());
                run_itr != running.end();
                ++run_itr) {
            skip_ticks = std::min(skip_ticks,(*run_itr)->get_ticks_remaining());
            if (skip_ticks == 0) {
                skip_ticks = (*run_itr)->get_ticks_remaining();
            } else {
                skip_ticks = std::min(skip_ticks,
                        (*run_itr)->get_ticks_remaining());
            }
        }

        // run the tasks by ticking to the next task completion time
        for (compute::list::iterator comp_itr(_comp->begin());
                    comp_itr != _comp->end();
                    ++comp_itr) {
            tasks_remaining -= (*comp_itr)->tick(skip_ticks);
        }

        _required_ticks += skip_ticks;
        
        // remove complete tasks from running list
        for (task::ptr_llist::iterator run_itr(running.begin());
                run_itr != running.end();
                ) {
            if ((*run_itr)->get_state() == task::complete) {
                task::ptr_llist::iterator rem(run_itr);
                ++run_itr; 
                running.erase(rem);
            } else {
                ++run_itr;
            }
        }
    }

    return _schedule;
}

// getters

task*
planner::schedule_entry::get_task()
{
    return _task;
}

compute*
planner::schedule_entry::get_compute()
{
    return _compute;
}

uint64_t
planner::get_required_ticks() const
{
    return _required_ticks;
}

uint64_t
planner::get_count_dependency_wait() const
{
    return _count_dep_wait;
}

uint64_t
planner::get_count_compute_wait() const
{
    return _count_comp_unavail;
}

uint64_t
planner::get_count_all_cores_busy() const
{
    return _all_cores_busy;
}

task*
planner::get_last_task() const
{
    return _last_task;
}

