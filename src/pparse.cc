
#include <errno.h>
#include <assert.h>
#include "pparse.h"
#include "planner.h"
#include <stdio.h>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <boost/shared_ptr.hpp>

int
pparse::read_compute_file(compute::list* comp, const std::string& filename)
{
    try {
        YAML::Node comp_base = YAML::LoadFile(filename.c_str());
        for (YAML::Node::const_iterator itr=comp_base.begin();
                itr != comp_base.end();
                ++itr) {
            boost::shared_ptr<compute>
                c(new compute(itr->first.Scalar(), itr->second.as<uint64_t>()));
            comp->push_back(c);
        }
    } catch (const YAML::Exception& e) {
        std::cout << "Parse of compute file " << filename << " failed: " << e.msg << "\n"; 
        return 1;
    }
   return 0;
}

namespace {
    std::string parent_tasks_label("parent_tasks");
    std::string execution_time_label("execution_time");
    std::string cores_required_label("cores_required");
}

int
pparse::read_tasks_file(task::list* tasks, const std::string& filename)
{
    try {
        YAML::Node task_base = YAML::LoadFile(filename.c_str());
        for (YAML::Node::const_iterator itr=task_base.begin();
                itr != task_base.end();
                ++itr) {
            std::string taskname(itr->first.Scalar());
            std::string parent_tasks;
            uint64_t cores = 0;
            uint64_t exec_time = 0;

            YAML::Node detail = itr->second;
            for (YAML::Node::const_iterator ditr = detail.begin();
                    ditr != detail.end();
                    ++ditr) {
                YAML::Node key = ditr->first;
                YAML::Node val = ditr->second;
                std::string key_str = key.as<std::string>();
                if (key_str.compare(execution_time_label) == 0) {
                    exec_time = val.as<uint64_t>();        
                    continue;
                }
                if (key_str.compare(cores_required_label) == 0) {
                    cores = val.as<uint64_t>();        
                    continue;
                }
                if (key_str.compare(parent_tasks_label) == 0) {
                    parent_tasks = val.as<std::string>();        
                    continue;
                }
            }
            boost::shared_ptr<task> t(new task(taskname.c_str(), cores, exec_time));
            if (!parent_tasks.empty()) {
                t->set_dep_str(parent_tasks.c_str());
            }
            tasks->push_back(t);
        }
    } catch (const YAML::Exception& e) {
        std::cout << "Parse of task file " << filename << " failed: " << e.msg << "\n"; 
        return 1;
    }
    return 0;
}

