#!/usr/bin/python2.7

import argparse
import random
import pprint

class task():
    """A class representing a computing job.
    This class has minimal features to format a job record.
    
    For example:

    task3:
        cores_required: 4
        execution_time: 50
        parent_tasks: "task1, task2"
    
    """
    def __init__(self, name, exec_time, cores, deps = []):
        self.name = name
        self.exec_time = exec_time
        self.cores = cores
        self.deps=[]
        self.add_deps(deps)
    def add_deps(self, deps):
        for d in deps:
            if d not in self.deps:
                self.deps.append(d)
    def __str__(self):
        ts="%s:\n    cores_required: %d\n    execution_time: %d" % (self.name, self.cores, self.exec_time)
        if len(self.deps):
            ts="%s\n    parent_tasks: \"%s\"" % (ts, ", ".join(self.deps))
        return ts

class comp():
    """A class representing a compute resource and formatting its yaml record.

    For example:

        compute1: 4 
    """
    def __init__(self, name, cores):
        self.name = name
        self.cores = cores

    def __str__(self):
        return "%s: %d" % (self.name, self.cores)


parser = argparse.ArgumentParser(description = "Task Generator Input")

# task options
parser.add_argument('--num-tasks', type = int, dest = "num_tasks", default = 10,
                    help = "number of tasks to generate")
parser.add_argument('--task-file', dest="task_file", default="tasks.yaml",
                    help="tasks file name")
parser.add_argument('--min-time', type = int, dest = "min_time", default = 100,
                    help = "minimum task time")
parser.add_argument('--max-time', type=int, dest="max_time", default=1000,
                    help="maximum task time")
parser.add_argument('--prob-deps', type=int, dest="prob_deps", default=80,
                    help="create dependencies for each task with this probability (0-100)")

# compute options
parser.add_argument('--num-compute', type = int, dest = "num_compute", default = 6,
                    help = "number of compute nodes to generate")
parser.add_argument('--compute-file', dest="compute_file", default="compute.yaml",
                    help="compute file name")
parser.add_argument('--min-cores', type = int, dest = "min_cores", default = 1,
                    help = "minimum task cores")
parser.add_argument('--max-cores', type=int, dest="max_cores", default=8,
                    help="maximum task cores")

args = parser.parse_args()

task_file=open(args.task_file, 'w')
compute_file=open(args.compute_file, 'w')

tasks = []
for i in range(args.num_tasks):
    dep_names = []
    while random.randint(0, 100) < args.prob_deps and len(tasks) > 1:
        dep_names.append(tasks[random.randint(0, len(tasks)-1)].name)

    t = task("task_%03d" % i,
        random.randint(args.min_time, args.max_time),
        random.randint(args.min_cores, args.max_cores),
        dep_names)

    tasks.append(t)

compute = []
# make sure at least one max-core compute node exists
compute.append(comp("compute_%03d" % 1, args.max_cores))
compute.append
for i in range(2, args.num_compute):
    c = comp("compute_%03d" % i, random.randint(args.min_cores, args.max_cores))
    compute.append(c)

task_file.write("\n".join(str(x) for x in tasks))
task_file.write("\n")
task_file.close()

compute_file.write("\n".join(str(x) for x in compute))
compute_file.write("\n")
compute_file.close()

# pp = pprint.PrettyPrinter(indent=4)
# pp.pprint(tasks)

