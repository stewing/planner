
#ifndef _planner_h_
#define _planner_h_

#include "compute.h"
#include "task.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <vector>

/*
 * @class planner
 *
 * generates an execution plan based on a set of compute resources and tasks
 */
class planner
{
public:
    
    /*
     * @class schedule_entry
     *
     * class binding compute and task objects for execution plan
     */
    class schedule_entry {
    public:
        schedule_entry(task*, compute*);
        task* get_task();
        compute* get_compute();
    private:
        task* _task;
        compute* _compute;
    };

    /* Status codes that the planner can return and string mapping
     */
    enum _status {
        ok,
        compute_exceeded,
        missing_dependency,
        circular_dependency
    };
    static const char* status_str[];
    typedef planner::_status status;
    typedef std::vector<schedule_entry> schedule_list;

    /*
     *  planner
     *  Constructor for planner class.
     *  This class references the provided compute and task structures.
     *
     *  @param[in] comp compute::list structure containing compute nodes
     *  @param[in] task task::list structure containing task list
     */ 
    planner(compute::list* comp, task::list* task);

    /*
     * validate_tasks
     *
     * Validates compute and task lists structures.  This method
     * builds the task graph necessary to perform planning and 
     * returns diagnostic status messages if certain problems are
     * detected within the provided compute and task data.
     *
     * @return status of validation and graph construction
     */
    status validate_tasks();

    /*
     * schedule_tasks
     *
     * Uses the graph created by validate_tasks() and the compute
     * nodes definitions' to simulate scheduling of tasks and build
     * an execution plan.
     *
     * The returned list of scheduled work items and compute is the
     * execution plan.
     *
     * @return schedule_list showing execution plan
     */
    schedule_list schedule_tasks();


    /*
     * get_required_ticks
     *
     * @return ticks required to execute the provided tasks
     */
    uint64_t get_required_ticks() const;

    /*
     * get_count_dependency_wait
     *
     * @return count of times tasks waited due to unmet dependencies
     */
    uint64_t get_count_dependency_wait() const;

    /*
     * get_count_compute_wait
     *
     * Returns count of times tasks waited due to lack of compute
     */
    uint64_t get_count_compute_wait() const;

    /*
     * get_count_all_cores_busy
     *
     * Returns count of times all cores were busy when scheduling.
     */
    uint64_t get_count_all_cores_busy() const;

    /*
     * get_last_task
     *
     * Returns the last task that the planner considered.  This is
     * useful when a planning error occurs.
     */
    task* get_last_task() const;


private:

    typedef boost::adjacency_list<
        boost::vecS, boost::vecS, boost::directedS> task_graph;
    typedef boost::graph_traits<task_graph>::vertex_descriptor vertex;
    typedef std::vector<vertex> sched_container;
    typedef std::pair<uint64_t, uint64_t> graph_edge;
    typedef std::vector<graph_edge> graph_edge_list;

    compute::list* _comp;
    task::list* _tasks;
    bool _tasks_validated;
    task_graph _tg;
    sched_container _job_sequence; 
    graph_edge_list _edge;
    schedule_list _schedule;
    uint64_t _required_ticks;
    uint64_t _count_dep_wait;
    uint64_t _count_comp_unavail;
    uint64_t _all_cores_busy;
    task* _last_task;
};

#endif // _planner_h_
