#include <vector>
#include <deque>
#include <atomic>
#include <memory>
#include <condition_variable>

template<class PL>
struct Subscriber {
    virtual ~Subscriber() = default;
    virtual void operator()(const PL& pl) = 0;
};

template<class PL>
struct Publisher {
    virtual ~Publisher() = default;
    virtual std::vector<PL> operator()() = 0;
};

template<class PL>
using  Subscribers = std::vector< std::unique_ptr<Subscriber<PL>>>;

template<class PL>
using  Publishers = std::vector< std::unique_ptr<Publisher<PL>>>;

template<class PL>
class TaskQueue {
  public:
    TaskQueue(std::atomic_bool& interrupt) : m_interrupt(interrupt), m_complete(false) {}
    ~TaskQueue() = default;
    void run(const Subscribers<PL>& subscribers, const Publishers<PL>& publishers);
  private:
    void postTask(Publisher<PL>& pub);
    void fetchTask(Subscriber<PL>& sub);

    const std::atomic_bool& m_interrupt;
    std::atomic_bool m_complete;

    std::deque<PL> m_q;
    std::mutex m_mutex;
    std::condition_variable empty_q;
    std::condition_variable task_complete;
};
