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
#include "/usr/local/opt/gumbo-parser/include/gumbo.h"


struct href_t {
    std::string href;
    int nested_level;
};

template<typename T>
class threadsafe_queue {
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue(){};
    threadsafe_queue(threadsafe_queue const&);
    void push(T);
    void wait_and_pop(T&);
    std::shared_ptr<T> wait_and_pop();
    bool try_pop(T&);
    std::shared_ptr<T> try_pop();
    bool empty() const;
    bool try_pop_2(T& , T&);
    const size_t size();

};

template<typename T>
threadsafe_queue<T>::threadsafe_queue(threadsafe_queue const& other)
{
    std::lock_guard<std::mutex> lk(other.mut);
    data_queue=other.data_queue;
}

template<typename T>
void threadsafe_queue<T>::push(T new_value)
{
    std::lock_guard<std::mutex> lk(mut);
    data_queue.push(new_value);
    data_cond.notify_one();
}

template<typename T>
void threadsafe_queue<T>::wait_and_pop(T& value)
{
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk,[this]{return !data_queue.empty();});
    value=data_queue.front();
    data_queue.pop();
}

template<typename T>
std::shared_ptr<T> threadsafe_queue<T>::wait_and_pop()
{
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk,[this]{return !data_queue.empty();});
    std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
    data_queue.pop();
    return res;
}


template<typename T>
bool threadsafe_queue<T>::try_pop(T& value)
{
    std::lock_guard<std::mutex> lk(mut);
    if(data_queue.empty())
        return false;
    value=data_queue.front();
    data_queue.pop();
    return true;
}

template<typename T>
bool threadsafe_queue<T>::try_pop_2(T& value_1, T& value_2)
{
    std::lock_guard<std::mutex> lk(mut);
    if(data_queue.size() < 2)
        return false;

    value_1 =data_queue.front();
    data_queue.pop();

    value_2 =data_queue.front();
    data_queue.pop();
    return true;
}

template<typename T>
std::shared_ptr<T> threadsafe_queue<T>::try_pop()
{
    std::lock_guard<std::mutex> lk(mut);
    if(data_queue.empty())
        return std::shared_ptr<T>();
    std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
    data_queue.pop();
    return res;
}

template<typename T>
bool threadsafe_queue<T>::empty() const
{
    std::lock_guard<std::mutex> lk(mut);
    return data_queue.empty();
}

template<typename T>
const size_t threadsafe_queue<T>::size() {
    std :: lock_guard <std :: mutex> lock (this->mut);
    return data_queue.size();
}


struct configuration_t
{
    std::string web_page, result;
    size_t number_of_pages, number_of_threads;
};

class main_task {
private:
    std::string page_entry;   //web-page where scanning starts
    size_t number_of_pages, number_of_threads;


    std::atomic_int parcing_threads_working;
    std::atomic_int reading_threads_working;

    std::atomic_int nested_level_count {0};


    std::string result;
    href_t temporary_href;

    //std::chrono::steady_clock::time_point start_time;
    //std::chrono::steady_clock::time_point end_time;


    threadsafe_queue<href_t> structed_queue;


    std::vector<std::thread> threads;

    void read_configuration(const std::string & conf_file_name);

    void search_for_links(GumboNode* node, int nested_level);

    void do_all_routine();


public:

    main_task(const std::string &);
    void do_main_routine();
    void writing_results();
};

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
        cf >> res.result;
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
    result = res.result;
    number_of_pages = res.number_of_pages;
    number_of_threads = res.number_of_threads;

}

void PrintInFile(std :: fstream & file, const std::string & web_tittle ){
    if(!file.is_open()){
        std :: cout << "Problem with file" << std :: endl;
        return;
    }

    file << web_tittle << std::endl;
}

void main_task::writing_results() {
    std::fstream fs_out_by_a(result);

    //for (size_t i = 0; i < number_of_pages; i++) {
    while(structed_queue.try_pop(temporary_href)){
        PrintInFile(fs_out_by_a, temporary_href.href);
    }
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void main_task::search_for_links(GumboNode* node, int nested_level) {

    if (node->type != GUMBO_NODE_ELEMENT) {
        return;
    }
    GumboAttribute* href;
    if (node->v.element.tag == GUMBO_TAG_A && (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
        if((std::string(href->value)).substr(0, 3) == "htt" || std::string(href->value).substr(0, 3) == "www") {

            temporary_href.href = href->value;
            temporary_href.nested_level = nested_level;

            structed_queue.push(temporary_href);
            //std::cout << temporary_href.href << " " << temporary_href.nested_level << std::endl;
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        search_for_links(static_cast<GumboNode*>(children->data[i]), nested_level);
    }
}


void main_task::do_all_routine() {

    std::string readBuffer;
    CURL *curl_handle;


    while(structed_queue.try_pop(temporary_href)) {

        std::cout << temporary_href.href << " " << temporary_href.nested_level << std::endl;

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
            search_for_links(output->root, temporary_href.nested_level+1);
            gumbo_destroy_output(&kGumboDefaultOptions, output);
        }
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

    structed_queue.push(temporary_href);

    for (int i = 0; i < number_of_threads; i++)
        threads.emplace_back(&main_task::do_all_routine, this);

    for (auto & th : threads)
        th.join();
}



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
    tsk.writing_results();
}
