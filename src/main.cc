
#include <boost/program_options.hpp>
#include <boost/program_options/value_semantic.hpp>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <utility>
#include <queue>

#include <string.h>

#include "pparse.h"
#include "task.h"
#include "compute.h"
#include "planner.h"

#define DEFAULT_TASK_FILE    "tasks.yaml"
#define DEFAULT_COMPUTE_FILE "compute.yaml"

namespace {
    class compute_assign_count {
    public:
        bool operator()(compute* lc, compute* rc)
        {
            return rc->get_assign_count() > lc->get_assign_count();
        }
    };
    class task_waiters_sort {
    public:
        bool operator()(task* lt, task* rt)
        {
            return rt->get_waiter_count() > lt->get_waiter_count();
        }
    };
    class task_dependencies_sort {
    public:
        bool operator()(task* lt, task* rt)
        {
            return rt->get_dependency_count() > lt->get_dependency_count();
        }
    };
}

using namespace boost;

int main(int argc, char** argv)
{
    // process options using
    namespace opt = boost::program_options;

    opt::options_description opt_desc("Planner Options");

    std::string tasks_file; 
    std::string compute_file;
    bool analyze = false;
    bool verbose = false;

    opt_desc.add_options()
        ("help",     "display this message")
        ("tasks",    opt::value<std::string>()->default_value(DEFAULT_TASK_FILE),
             "name of task description file")
        ("compute",  opt::value<std::string>()->default_value(DEFAULT_COMPUTE_FILE),
             "name of compute description file (default: compute.yaml)")
        ("analyze",  opt::bool_switch(&analyze),
             "analyze compute utilization and task dependencies")
        ("verbose",  opt::bool_switch(&verbose),
             "print details of task and compute input");

    opt::variables_map vmap;

    try {
        opt::store(opt::parse_command_line(argc, argv, opt_desc), vmap);
        std::string task_file("tasks.yaml");

        if (vmap.count("help")) {
            std::cout << "Usage:" << argv[0] << " --tasks <tasks.yaml>"
                << " --compute <compute.yaml>\n" << opt_desc << "\n";
            return 1;
        }

        if (vmap.count("tasks")) {
            tasks_file = vmap["tasks"].as<std::string>();
        }

        if (vmap.count("compute")) {
            compute_file = vmap["compute"].as<std::string>();
        }

        if (vmap.count("analyze")) {
            analyze = vmap["analyze"].as<bool>(); 
        }

        if (vmap.count("verbose")) {
            verbose = vmap["verbose"].as<bool>(); 
        }

    } catch (opt::error& err) {
        std::cerr << "Error: " << err.what() << "\n";
        std::cout << "Usage:" << argv[0] << " --tasks <tasks.yaml>"
            << " --compute <compute.yaml>\n" << opt_desc << "\n";
        return 1;
    }
    
    // read compute yaml file
    compute::list comp;
    if (verbose) {
        std::cout << "Using compute file " << compute_file << ".\n";
    }
    int err = 0;

    err = pparse::read_compute_file(&comp, compute_file);
    if (err) {
        return 1;
    }

    if (verbose) {
        std::cout << "Compute Resources:\n";
        for (compute::list::iterator itr(comp.begin()) ; 
                itr != comp.end();
                ++itr) {
            std::cout << "    " << **itr << "\n";
        }
    }

    // read the task yaml file
    task::list tasks;
    if (verbose) {
        std::cout << "Using tasks file " << tasks_file << ".\n";
    }
    err = pparse::read_tasks_file(&tasks, tasks_file);
    if (err) {
        return 1;
    }

    if (verbose) {
        std::cout << "Tasks:\n";
        for (task::list::iterator itr(tasks.begin()) ; 
                itr != tasks.end();
                ++itr) {
            std::cout << "    " << **itr << "\n";
        }
    }

    // initialize planner
    planner plan(&comp, &tasks);

    // validate tasks and compute
    planner::status rc = plan.validate_tasks();

    if (rc != planner::ok) {
        std::cout << "Planner failed: " << planner::status_str[rc] << "\n";
        return 1;
    }

    // build the plan
    planner::schedule_list sched(plan.schedule_tasks());

    std::cout << "# task schedule:\n";
    for (planner::schedule_list::iterator itr(sched.begin()) ; 
            itr != sched.end();
            ++itr) {
        task* t = itr->get_task();
        compute* c = itr->get_compute();
        std::cout << t->get_name() << ": "  << c->get_name() << "\n";
    }

    // very basic analysis of tasks, compute and planning
    if (analyze) {
        const int max_show_count = 10;
        std::cout << "== Compute Analyzer ==\n";
        uint64_t total_comp_cores(0);
        uint64_t total_comp_ticks(0);
        uint64_t total_comp_busy(0);
        uint64_t total_comp_idle(0);
        uint64_t compute_nodes(0);
        typedef std::priority_queue<compute*, std::vector<compute*>, compute_assign_count> hot_compute_list;
        hot_compute_list hot_compute;
        for (compute::list::iterator comp_itr(comp.begin());
                comp_itr != comp.end();
                ++comp_itr) {
            total_comp_cores += (*comp_itr)->get_cores();
            total_comp_ticks += (*comp_itr)->get_total_ticks();
            total_comp_busy += (*comp_itr)->get_busy_ticks();
            total_comp_idle += (*comp_itr)->get_idle_ticks();
            ++compute_nodes;
            hot_compute.push(comp_itr->get());
        }
        
        float avg_cores =
            static_cast<float>(total_comp_cores) / static_cast<float>(compute_nodes);

        std::cout << "Total core count: " << total_comp_cores << "\n";
        std::cout << "Total ticks needed (across all cores):" << total_comp_ticks << "\n";
        std::cout << "    busy ticks: " << total_comp_busy << "\n";
        std::cout << "    idle ticks: " << total_comp_idle << "\n";
        std::cout.precision(4);
        std::cout << "Avg. cores per node: " << std::setw(5) << avg_cores << "\n";
        std::cout << "Hot compute nodes:\n";
        for (uint64_t ix(0); ix < max_show_count && ix < hot_compute.size(); ++ix) {
            compute* c(hot_compute.top());
            hot_compute.pop();
            if (c->get_assign_count() == 0) {
                break;
            }
            std::cout << "    node: " << c->get_name() << " (" << c->get_cores() <<
                " cores) ran " << c->get_assign_count() << " tasks\n";
        }
        std::cout << "Planner ticks: " << plan.get_required_ticks() << "\n";
        std::cout << "Task delays\n";
        std::cout << "    not runnable, unmet dependencies: " << plan.get_count_dependency_wait() << "\n";
        std::cout << "    runnable, but waited for compute: " << plan.get_count_compute_wait() << "\n";
        std::cout << "Schedulings when all cores were busy: " << plan.get_count_all_cores_busy() << "\n";

        std::cout << "== Task analysis ==\n";
        typedef std::priority_queue<task*, std::vector<task*>, task_waiters_sort> most_waited_list;
        typedef std::priority_queue<task*, std::vector<task*>, task_dependencies_sort> most_dependent_list;
        most_waited_list most_waited_on;
        most_dependent_list most_dependencies;
        for (task::list::iterator task_itr(tasks.begin());
                task_itr != tasks.end();
                ++task_itr) {
            most_waited_on.push(task_itr->get());
            most_dependencies.push(task_itr->get());
        }

        for (uint64_t ix(0); ix < max_show_count && ix < most_waited_on.size(); ++ix) {
            task* t(most_waited_on.top());
            task::ptr_list waiters(t->get_waiter_list());
            if (waiters.empty()) {
                break;
            }
            if (ix == 0) {
                std::cout << "Most waited on tasks:\n";
            }
            std::cout << "    " << t->get_name() << ": " << t->get_waiter_count() << " waiters (";
            for (task::ptr_list::iterator wait_itr(waiters.begin());
                    wait_itr != waiters.end();
                    ++wait_itr) {
                std::cout << (*wait_itr)->get_name() << ((wait_itr+1 != waiters.end()) ? ", " : ")\n");
            }
            most_waited_on.pop();
            if (most_waited_on.empty()) {
                break;
            }
        }
        for (uint64_t ix(0); ix < max_show_count && ix < most_dependencies.size(); ++ix) {
            task* t(most_dependencies.top());
            task::ptr_list deps(t->get_dependencies());
            if (deps.empty()) {
                break;
            }
            if (ix == 0) {
                std::cout << "Tasks with the most dependencies:\n";
            }
            std::cout << "    " << t->get_name() << ": " << t->get_dependency_count() << " dependencies (";
            for (task::ptr_list::iterator dep_itr(deps.begin());
                    dep_itr != deps.end();
                    ++dep_itr) {
                std::cout << (*dep_itr)->get_name() << ((dep_itr+1 != deps.end()) ? ", " : ")\n");
            }
            most_dependencies.pop();
            if (most_dependencies.empty()) {
                break;
            }
        }
        std::cout << "\n";
    }
    return 0;
}

