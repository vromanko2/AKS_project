#include "MainTask/main_task.h"
#include "Configuration/configuration.h"
#include <iostream>



int main(int argc, char* argv[])
{

    std::string conf_file_name ("conf.txt");
    if(argc == 2)
        conf_file_name = argv[1];
    if(argc > 2) {
        std::cerr << "Too many arguments. Usage: \n"
                     "<program>\n"
                     "or\n"
                     "<program> <config-filename>\n" << std::endl;
        return 1;
    }

    main_task tsk(conf_file_name);

    tsk.do_main_routine();
}
