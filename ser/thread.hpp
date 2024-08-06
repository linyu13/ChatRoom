#include "../include/headFile.hpp"

// 任务队列类模板
template <typename T>
class TaskQueue
{
public:
    TaskQueue() = default; // 默认构造函数
    // 移动构造函数，使用noexcept表示不会抛出异常
    TaskQueue(const TaskQueue &&other) noexcept 
    {
        std::unique_lock <std::mutex> lock(other.m_mutex); // 锁住源对象的互斥锁
        m_queue = std::move(other.m_queue); // 将队列中的数据移动到当前对象
    }
    ~TaskQueue() = default; // 默认析构函数

    // 判断队列是否为空
    bool empty()
    {
        std::unique_lock <std::mutex> lock(m_mutex); // 锁住互斥锁，确保线程安全
        return m_queue.empty(); // 返回队列是否为空
    }

    // 获取队列的大小
    int size()
    {
        std::unique_lock <std::mutex> lock(m_mutex); // 锁住互斥锁
        return m_queue.size(); // 返回队列的大小
    }

    // 入队操作
    void enqueue(T &t)
    {
        std::unique_lock <std::mutex> lock(m_mutex); // 锁住互斥锁
        m_queue.emplace(std::move(t)); // 将任务移动到队列中
    }

    // 出队操作
    bool dequeue(T &t)
    {
        std::unique_lock < std::mutex> lock(m_mutex); // 锁住互斥锁
        if(m_queue.empty()) // 如果队列为空，返回false
        {
            return false;
        }
        t = std::move(m_queue.front()); // 将队首元素移动到t
        m_queue.pop(); // 移除队首元素
        return true; // 返回true表示成功出队
    }

private:
    std::queue<T> m_queue; // 队列，用于存储任务
    std::mutex m_mutex; // 互斥锁，用于确保线程安全
};

// 线程池类
class ThreadPool
{
private:
    // 内部类，用于表示线程的工作
    class ThreadWork
    {
    private:
        int m_id; // 线程ID
        ThreadPool *m_thread_pool; // 指向所属线程池的指针
    public:
        ThreadWork(int id, ThreadPool* thread_pool) : m_id(id), m_thread_pool(thread_pool) {} // 构造函数

        // 重载()运算符，使对象可以像函数一样调用
        void operator()()
        {
            std::function<void(void)> func; // 存储任务的函数对象
            bool dequeued; // 表示是否成功从队列中取出任务
            while (!m_thread_pool->m_shutdown) // 循环直到线程池关闭
            {
                {
                    std::unique_lock<std::mutex> lock(m_thread_pool->m_mutex); // 锁住线程池的互斥锁
                    if(m_thread_pool->m_queue.empty()) // 如果任务队列为空，线程等待
                    {
                        m_thread_pool->m_conditional_lock.wait(lock); // 等待条件变量
                    }
                    dequeued = m_thread_pool->m_queue.dequeue(func); // 尝试从队列中取出任务
                }
                if(dequeued) // 如果成功取出任务
                {
                    func(); // 执行任务
                }
            }
        }
    };

public:
    // 线程池构造函数，初始化线程数
    ThreadPool(const int n_threads = 4) : m_shutdown(false), m_threads(std::vector<std::thread>(n_threads)) {}

    // 删除拷贝构造函数和拷贝赋值运算符
    ThreadPool(const ThreadPool &other) = delete;
    ThreadPool operator=(const ThreadPool &other) = delete;

    // 删除移动构造函数和移动赋值运算符
    ThreadPool(ThreadPool &&other) = delete;
    ThreadPool operator=(ThreadPool &&other) = delete;

    // 初始化线程池，创建线程
    void init()
    {
        for(size_t i = 0; i < m_threads.size(); i++) 
        {
            m_threads.at(i) = std::thread(ThreadWork(i, this)); // 创建线程并将其加入线程池
        }
    }

    // 关闭线程池，等待所有线程结束
    void shutdown()
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex); // 锁住互斥锁
            m_shutdown = true; // 设置关闭标志
        } 
        m_conditional_lock.notify_all(); // 唤醒所有等待的线程
        
        for(size_t i = 0; i < m_threads.size(); i++)
        {
            if(m_threads.at(i).joinable()) // 如果线程可加入
            {
                m_threads.at(i).join(); // 等待线程结束
            }
        }
    }

    // 提交任务到线程池
    template<typename T, typename ...Args>
    auto submit(T &&t, Args ...args) -> std::future<decltype(t(args...))>
    {
        // 使用std::bind绑定函数和参数，创建任务
        std::function<decltype(t(args...))()> func = std::bind(std::forward<T>(t), std::forward<Args>(args)...);

        // 创建packaged_task来管理任务
        auto task_ptr = std::make_shared<std::packaged_task<decltype(t(args...))()>>(func);
        
        // 将任务封装为无参函数，加入任务队列
        std::function<void()> queue_func = [task_ptr]()
        {
            (*task_ptr)();
        };
        m_queue.enqueue(queue_func); // 将任务加入队列
        m_conditional_lock.notify_one(); // 通知一个等待的线程执行任务
        m_needAdjust.store(true); // 标记需要调整线程池
        return task_ptr->get_future(); // 返回任务的future对象
    }

    // 根据当前任务数调整线程池的线程数
    void Adjust_threads()
    {
        if(m_shutdown) // 如果线程池已关闭，则不做调整
        {
            return;
        }
        size_t current_threads = m_threads.size(); // 获取当前线程数
        size_t current_tasks = m_queue.size(); // 获取当前任务数
        if(current_tasks > current_threads && current_tasks < m_Maxthreads.load()) // 如果任务数大于线程数，且不超过最大线程数
        {
            size_t new_threads = std::min(current_tasks, m_Maxthreads.load()); // 计算需要增加的线程数
            size_t add_threads = new_threads - current_threads;
            for(size_t i = 0; i < add_threads; i++)
            {
                m_threads.emplace_back(ThreadWork(current_threads + i, this)); // 创建并添加新线程
            }
        }
        else if(current_threads > m_Minthreads.load() && current_tasks < current_threads / 2) // 如果线程数过多且任务数较少
        {
            size_t new_threads = std::max(current_tasks, m_Minthreads.load());  
            size_t del_threads = current_threads - new_threads;
            for(size_t i = 0; i < del_threads; i++)
            {
                size_t last_index = m_threads.size() - 1;
                if (m_threads[last_index].joinable())
                {
                    m_threads[last_index].join(); // 等待线程结束
                }
                m_threads.pop_back(); // 移除多余的线程
            }
        }
    }

private:
    bool m_shutdown; // 标志线程池是否关闭
    TaskQueue<std::function<void(void)>> m_queue; // 任务队列
    std::vector<std::thread> m_threads; // 线程池中的线程
    std::mutex m_mutex; // 互斥锁，用于同步
    std::condition_variable m_conditional_lock; // 条件变量，用于线程间的等待和通知
    std::atomic<bool> m_needAdjust; // 标志是否需要调整线程数
    std::atomic<size_t> m_Maxthreads; // 最大线程数
    std::atomic<size_t> m_Minthreads; // 最小线程数
};
