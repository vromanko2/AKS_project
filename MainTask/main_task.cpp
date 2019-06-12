//
// Created by Veronika Romanko on 2019-06-11.
//

#include "main_task.h"
#include "../Configuration/configuration.h"
#include "../ThreadsafeQueue/threadsafe_queue.h"
#include "../../../../../usr/local/opt/gumbo-parser/include/gumbo.h"
#include "../TimeMeasurement/time_measurement.h"
#include <chrono>
#include <atomic>

inline std::chrono::high_resolution_clock::time_point get_current_time_fenced(){
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto res_time = std::chrono::high_resolution_clock::now();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return res_time;
}

template<class D>
inline long long to_us(const D& d)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}


void main_task::read_configuration(const std::string & conf_file_name) {
    std::ifstream cf(conf_file_name);
    if (!cf.is_open()) {
        std::cerr << "Failed to open configuration file " << conf_file_name << std::endl;
        return;
    }

    std::ios::fmtflags flags(cf.flags());
    cf.exceptions(std::ifstream::failbit);

    configuration_t res;
    std::string temp;

    try {
        cf >> res.web_page;
        getline(cf, temp);
        cf >> res.link_result;
        getline(cf, temp);
        cf >> res.image_result;
        getline(cf, temp);
        cf >> res.number_of_pages;
        getline(cf, temp);
        cf >> res.number_of_threads;
        getline(cf, temp);


    } catch (std::ios_base::failure &fail) {
        cf.flags(flags);
        throw;
    }
    cf.flags(flags);

    page_entry = res.web_page;
    link_result = res.link_result;
    image_result = res.image_result;
    number_of_pages = res.number_of_pages;
    number_of_threads = res.number_of_threads;

}


void main_task::search(GumboNode* node, int nested_level) {

    if (node->type != GUMBO_NODE_ELEMENT) {
        return;
    }
    GumboAttribute* href;

    if (node->v.element.tag == GUMBO_TAG_IMG && (href = gumbo_get_attribute(&node->v.element.attributes, "src"))){
        temporary_img.href = href->value;
        temporary_img.nested_level = nested_level;

        img_queue.push(temporary_img);
    }



    if (node->v.element.tag == GUMBO_TAG_A && (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
        if((std::string(href->value)).substr(0, 3) == "htt" || std::string(href->value).substr(0, 3) == "www") {

            if(std::string(href->value).find(to_find) != std::string::npos) {

                temporary_href.href = href->value;
                temporary_href.nested_level = nested_level;

                structed_queue.push(temporary_href);
            }
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        search(static_cast<GumboNode*>(children->data[i]), nested_level);
    }
}


void main_task::writing_results(const std::string & file_name, const std::map<std::string, int>::const_iterator & it) {
    std::ofstream fs_out_by_a(file_name,  std::ios_base::app);
    if(!fs_out_by_a.is_open()){
        std :: cout << "Problem with file" << std :: endl;
        return;
    }
    fs_out_by_a << it->first << std::endl;
}



static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void save_html (std::atomic_int & i, const char* s){

    boost::filesystem::path folder = "Html";
    boost::filesystem::create_directory(folder);

    auto path = boost::filesystem::absolute(folder);


    std::string file_name = path.string() + "/file" + std::to_string(i) + ".html";
    std::ofstream fs_out_by_a(file_name,  std::ios_base::app);
    if(!fs_out_by_a.is_open()){
        std :: cout << "Problem with file" << std :: endl;
        return;
    }
    fs_out_by_a << s << std::endl;
}


void main_task::do_all_routine() {

    std::string readBuffer;
    CURL *curl_handle;
    std::atomic_int i;
    i = 1;

    while (structed_queue.try_pop(temporary_href)) {
        clear_from_same_links(temporary_href);
        if (temporary_href.nested_level < number_of_pages) {
            curl_global_init(CURL_GLOBAL_ALL);
            curl_handle = curl_easy_init();

            curl_easy_setopt(curl_handle, CURLOPT_URL, (temporary_href.href).c_str());
            curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_perform(curl_handle);
            curl_easy_cleanup(curl_handle);

            GumboOutput *output = gumbo_parse(readBuffer.c_str());
                save_html(i, readBuffer.c_str());
            search(output->root, temporary_href.nested_level + 1);
            gumbo_destroy_output(&kGumboDefaultOptions, output);
        }
        i = i+1;
    }

    while(img_queue.try_pop(temporary_img)) {
        clear_from_same_images(temporary_img);
    }
}



main_task::main_task(const std::string & conf_file_name)
{
    read_configuration(conf_file_name);
}

void main_task::do_main_routine()
{
    temporary_href.href = page_entry;
    temporary_href.nested_level = 0;

    int position = temporary_href.href.find(".com");
    to_find = temporary_href.href.substr(0, position);

    start_point = get_current_time_fenced();
    structed_queue.push(temporary_href);

    for (int i = 0; i < number_of_threads; i++)
        threads.emplace_back(&main_task::do_all_routine, this);

    for (auto & th : threads)
        th.join();

    finish_point = get_current_time_fenced();

    auto total_time = finish_point - start_point;

    for (std::map<std::string, int>::const_iterator it = map_link.begin(); it != map_link.end(); ++it)
        writing_results(link_result, it);

    for (std::map<std::string, int>::const_iterator it = map_image.begin(); it != map_image.end(); ++it)
        writing_results(image_result, it);

    std::cout << to_us(total_time) / 1000000 << std::endl;
}



void main_task::clear_from_same_links(const href_t & temporary_href){
    std::lock_guard <std::mutex> lock(mtx);
    ++map_link[temporary_href.href];
}


void main_task::clear_from_same_images(const href_t & temporary_image){
    std::lock_guard <std::mutex> lock(mtx);
    ++map_image[temporary_image.href];
}
