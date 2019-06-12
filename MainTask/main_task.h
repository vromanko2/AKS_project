#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>
#include <fstream>
#include <queue>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <atomic>

#include "/usr/local/opt/gumbo-parser/include/gumbo.h"
#include <boost/filesystem.hpp>


#include "../hrefStruct/href_t.h"
#include "../ThreadsafeQueue/threadsafe_queue.h"
#include "../Configuration/configuration.h"
#include "../TimeMeasurement/time_measurement.h"

#ifndef UNTITLED_MAIN_TASK_H
#define UNTITLED_MAIN_TASK_H

class main_task {
private:
    std::string page_entry;   //web-page where scanning starts
    size_t number_of_pages, number_of_threads;


    std::atomic_int parcing_threads_working;
    std::atomic_int reading_threads_working;

    std::atomic_int nested_level_count {0};


    std::string to_find;
    std::string link_result;
    std::string image_result;


    href_t temporary_href;

    href_t temporary_img;

    std::mutex mtx;

    threadsafe_queue<href_t> structed_queue;

    threadsafe_queue<href_t> img_queue;

    std::map<std::string, int> map_link;
    std::map<std::string, int> map_image;

    std::chrono::high_resolution_clock::time_point start_point;
    std::chrono::high_resolution_clock::time_point finish_point;


    threadsafe_queue<std::map<std::string, int>> result_queue;


    std::vector<std::thread> threads;

    void read_configuration(const std::string & conf_file_name);


    void search(GumboNode* node, int nested_level);

    void clear_from_same_links(const href_t &);

    void clear_from_same_images(const href_t & temporary_image);

    void do_all_routine();


public:

    main_task(const std::string &);
    void do_main_routine();
    void writing_results(const std::string & file_name, const std::map<std::string, int>::const_iterator & it);
};


#endif //UNTITLED_MAIN_TASK_H
