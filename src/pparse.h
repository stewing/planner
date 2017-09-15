
#include "compute.h"
#include "task.h"
#include <string>

namespace pparse {
    /*
     * read_compute_file
     *
     * Parses the provided yaml file and fills in a list of compute nodes.
     *
     * @param  comp  pointer to the output structure that stores the parsed compute entries
     *
     * @param  filename name of file to parse
     */
    int read_compute_file(compute::list* comp, const std::string& filename);

    /*
     * read_tasks_file
     *
     * Parses the provided yaml file and fills in a list of tasks.
     *
     * @param  task  pointer to the output structure that stores the parsed task entries
     *
     * @param  filename name of file to parse
     */
    int read_tasks_file(task::list* task, const std::string& filename);
}

