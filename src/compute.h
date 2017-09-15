
#include <boost/shared_ptr.hpp>
#include <ostream>
#include <stdint.h>
#include <string>
#include <vector>
#include "task.h"

#ifndef _compute_h_
#define _compute_h_

/*
 * @class compute
 *
 * Models a compute node.
 */
class compute
{
public:
    /*
     * compute node states
     */
    enum _state {
        busy,
        free,
        partially_available
    };
    typedef enum compute::_state  state;

    /*
     * compute
     *
     * Constructor for compute class.
     *
     * @param[in]  name  Name of compute resource.
     * @param[in]  cores  Number of cores present.
     */
    compute(const std::string& name, const uint64_t& cores);

    /* 
     * get_name 
     *
     * returns the compute resource's name
     *
     * @return string name
     */
    std::string get_name() const;

    /*
     * returns the current state of the compute
     *
     * @return state enum
     */
    state get_state() const;

    /*
     * get_cores
     *
     * @return number of cores on this node
     */
    uint64_t get_cores() const;

    /*
     * get_cores_available
     *
     * @return number of cores currently free
     */
    int64_t get_cores_available() const;

    /*
     * assign_task
     *
     * Assigns a task to this compute resource.
     * The task consumes some number of cores for
     * some period of time, based on the task.
     *
     * @param[in]  task  the task to assign to this compute node
     *
     * @return void
     */
    void assign_task(task* task);

    /*
     * get_assign_count
     *
     * @return  the number of times this compute node has been assigned a task
     */
    uint64_t get_assign_count() const;

    /*
     * tick
     *
     * param[in]  ticks  Run all assigned, tasks for this number of ticks
     *
     * @return the number of tasks completed during this call
     */
    int64_t tick(uint64_t ticks = 1);

    /*
     * get_busy_ticks
     *
     * @return the number of busy ticks on this compute node.
     */
    uint64_t get_busy_ticks() const;
    
    /*
     * get_idle_ticks
     *
     * @return the number of idle ticks on this compute node.
     */
    uint64_t get_idle_ticks() const;

    /*
     * get_total_ticks
     *
     * @return The total number of ticks on this compute node.
     */
    uint64_t get_total_ticks() const;

    friend std::ostream& operator<<(std::ostream& os, const compute& comp);

    typedef boost::shared_ptr<compute> ptr;
    typedef std::vector<ptr> list;
    typedef std::vector<compute*> ptr_list;

private:

    std::string _name;
    int64_t _cores_total;
    int64_t _cores_available;
    uint64_t _cumulative_busy_ticks; // for all cores
    uint64_t _cumulative_idle_ticks; // for all cores
    uint64_t _completed_tasks;
    task::ptr_llist _current_tasks;
    state _state;
    uint64_t _assign_count;
};

std::ostream& operator<<(std::ostream& os, const compute& comp);

#endif // _compute_h_
