#include <iostream>
#include <mutex>
#include <thread>
#include <future>

#define DEBUG false
#define CODE(s) {if (DEBUG) {s} }

using namespace std;

// Instantiate template class TaskQueue<Payload>;

template<class PL>
void TaskQueue<PL>::postTask(Publisher<PL>& pub) {
    auto thId = std::this_thread::get_id();
    CODE(std::cerr << thId << " - Entered Publisher   -\n";)

    while(not m_complete) {
        unique_lock<mutex> lock (m_mutex);
        condition.wait(lock, [this]() { return m_postTask ? true : false; });
        m_postTask = false;
        auto pls = pub();
        m_complete = pls.empty() and m_q.empty();
        
        CODE(std::cerr << "  " << thId << ": Posting Task # " << pls.size() << std::endl;)
        for (const auto& pl : pls) m_q.push_back(pl);
        
        lock.unlock();
        condition.notify_all();
    }
}

template<class PL>
void TaskQueue<PL>::fetchTask(Subscriber<PL>& sub) {
    auto thId = std::this_thread::get_id();
    CODE(std::cerr << thId << " - Entered Subscriber - \n";)
    while (not m_complete) {
        unique_lock<mutex> lock(m_mutex);
        condition.wait(lock, [this]() {return not m_q.empty() or m_complete;});
        if (not m_q.empty() and not m_complete) {
            auto pl = m_q.front(); // Copy
            m_q.pop_front();
            lock.unlock(); // Unlock for other subscribers

            CODE(std::cerr << "  " << thId << ": Fetching Task\n";)
            sub(pl);
        } else
            lock.unlock();
        m_postTask = true;
        task_complete.notify_all();
        CODE(std::cerr << "  " << thId <<": One Task Complete\n";)
    }
}


template<class PL>
void TaskQueue<PL>::run(const Subscribers<PL>& subscribers, const Publishers<PL>& publishers) {
    CODE(std::cerr << "Opening Task Queue \n";);
    vector<thread> threads;

    for (auto& pub : publishers) {
        if (pub) threads.emplace_back(&TaskQueue<PL>::postTask, this, std::ref(*pub));
    }

    for (auto& sub : subscribers) {
        if (sub) threads.emplace_back(&TaskQueue<PL>::fetchTask, this, std::ref(*sub));
    }

    for (auto& th : threads)
        th.join();

    CODE(std::cerr << "Closing Task Queue \n";);
}
