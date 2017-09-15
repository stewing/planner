
#ifndef _task_h_
#define _task_h_

#include <boost/shared_ptr.hpp>
#include <ostream>
#include <stdint.h>
#include <queue>
#include <string>
#include <vector>
#include <list>
#include <map>
#include "identity.h"

/*
 * @class task
 *
 * Models a compute task.
 */
class task : public namespace_id<task>
{
public:
    /*
     * compute job states
     */
    enum _state {
        not_started,
        running,
        complete,
        no_resources
    };

    typedef boost::shared_ptr<task> ptr;
    typedef std::vector<ptr> list;
    typedef std::vector<task*> ptr_list;
    typedef std::list<task*> ptr_llist;
    typedef enum task::_state  state;
    typedef uint64_t id_t;

    /*
     * @struct _tick_stat
     *
     * A structure containing the state of this task after it runs for
     * some period of time.
     */
    struct _tick_stat {
        uint64_t remaining_ticks;
        uint64_t busy_ticks;
        uint64_t idle_ticks;
    };
    typedef _tick_stat tick_stat;

    /*
     * task
     *
     * Constructor for task class.  Creates a task object that can be used to
     * model execution and develop an execution plan.
     *
     * Tasks are registered in two static structures associated with this class
     * that are used to look up tasks quickly based either on the id number or
     * the task name.
     *
     * @param[in]  name        task name
     * @param[in]  reqd_cores  cores required for this task
     * @param[in]  reqd_ticks  ticks required to complete this task
     */
    task(const char* name, const uint64_t& reqd_cores, const uint64_t& reqd_ticks);
    virtual ~task();

    /*
     * get_id
     *
     * @return number ID of this task
     */
    id_t get_id() const;

    /* 
     * get_name 
     *
     * @return string of name
     */
    std::string get_name() const;

    /*
     * get_cores_required
     *
     * @return cores required for this task
     */
    uint64_t get_cores_required() const;

    /*
     * run_for
     *
     * Simulates running this task.
     *
     * @return tick_stat structure indicating how many ticks were used and how many more are needed
     */
    tick_stat run_for(const uint64_t& ticks);

    /*
     * get_state
     *
     * @return state enum indicating task state.
     */
    state get_state() const;

    /*
     * set_state
     *
     * Sets the tasks's state.
     *
     * @return void
     */
    void set_state(task::state);

    /*
     * set_dep_str
     *
     * Sets the dependency string for this task.
     *
     * @param[in]  deps  string with command separated dependencies
     */
    void set_dep_str(const char* deps);

    /*
     * map_dependencies
     *
     * Verifies that this task's dependencies are met.
     * Maps dependencies to this task and indicates
     * waiting status for that task.
     *
     * @return boolean indicating success or failure
     */
    bool map_dependencies();

    /* 
     * dependencies_met()
     *
     * Verfies that this task's dependencies have been satisfied.
     *
     * @return boolean indicating whether dependencies are met
     */
    bool dependencies_met() const;

    /*
     * get_dependency_count
     *
     * @return dependency count for this task
     */
    uint64_t get_dependency_count() const;

    /*
     * get_dependencies
     *
     * @return list of pointers to this tasks's dependencies
     */
    ptr_list get_dependencies() const;

    /*
     * get_waiter_count
     *
     * @returns count of tasks waiting on this task
     */
    uint64_t get_waiter_count() const;

    /*
     * get_waiter_list
     *
     * @return list of pointers to this tasks waiters
     */
    ptr_list get_waiter_list() const;


    /*
     * get_ticks_remaining
     *
     * @return ticks require for this task to complete
     */
    uint64_t get_ticks_remaining() const; 


    /*
     * lookup_task
     * @param[in]  id  integer id of task of intrest
     *
     * @return pointer to specified task or NULL if not found
     */
    static task* lookup_task(const id_t id);

    /*
     * lookup_task
     * @param[in]  name  name of task of intrest
     *
     * @return pointer to specified task or NULL if not found
     */
    static task* lookup_task(const std::string& name);

    friend std::ostream& operator<<(std::ostream& os, const task& tsk);

private:
    std::string _name;
    uint64_t _reqd_cores;
    uint64_t _reqd_ticks;
    uint64_t _ticks_remaining;
    id_t _id;
    std::string _dep_str;
    ptr_list _deps;
    state _state;
    bool _mapped_deps;

    uint64_t _waiters;         // tasks waiting on this
    ptr_list _waiter_list;     // list of the above

    void _add_task_id_mapping(id_t);
    uint64_t _incr_waiters(task*);

    std::vector<std::string> _get_dep_str() const;

    void _register_task(task*);
    void _deregister_task(task*);

    typedef std::map<std::string, task*> name_lookup;
    typedef std::vector<task*> id_lookup;

    static name_lookup by_name;
    static id_lookup by_id;

    static const char* _state_str[];
};

std::ostream& operator<<(std::ostream& os, const task& tsk);

#endif // _task_h_
