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
    auto pls = pub();
    if (not pls.empty()) {
        CODE(std::cerr << "  " << thId << ": Posting Task # " << pls.size() << std::endl;)
        for (const auto& pl : pls) m_q.push_back(pl);
    } else
        m_complete = true;

    empty_q.notify_all();
    while(not m_interrupt and not m_complete) {
        unique_lock<mutex> lock (m_mutex);
        task_complete.wait(lock);
        auto pls = pub();
        bool toContinue = not pls.empty();
        if (toContinue) {
            CODE(std::cerr << "  " << thId << ": Posting Task # " << pls.size() << std::endl;)
            for (const auto& pl : pls) m_q.push_back(pl);
        }
        lock.unlock();

        empty_q.notify_all();
        m_complete = not toContinue and m_q.empty();
    }
}

template<class PL>
void TaskQueue<PL>::fetchTask(Subscriber<PL>& sub) {
    auto thId = std::this_thread::get_id();
    CODE(std::cerr << thId << " - Entered Subscriber - \n";)
    while (not m_interrupt and not m_complete) {
        unique_lock<mutex> lock(m_mutex);
        empty_q.wait(lock, [this]() {return not m_q.empty() or m_complete;});
        if (not m_q.empty() and not m_interrupt and not m_complete) {
            auto pl = m_q.front();
            m_q.pop_front();
            lock.unlock();

            CODE(std::cerr << "  " << thId << ": Fetching Task\n";)
            sub(pl);
        } else
            lock.unlock();
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
