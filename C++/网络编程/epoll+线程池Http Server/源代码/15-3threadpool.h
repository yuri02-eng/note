#ifndef THREADPOOL_H
#define THREADPOOL_H

/*
 * 线程池模板类
 * 这是一个半同步/半异步的线程池实现，采用生产者-消费者模型
 * 模板参数T：表示任务类，必须实现process()方法
 * 设计模式：工作线程模型 + 任务队列
 */

#include <list>         // 使用双向链表实现任务队列
#include <cstdio>       // 标准输入输出，用于调试打印
#include <exception>    // 异常处理
#include <pthread.h>    // POSIX线程库
#include "14-2locker.h" // 线程同步工具（互斥锁、信号量、条件变量）

template <typename T>
class threadpool
{
public:
    /* 构造函数
     * 参数：
     *   thread_number: 线程池中的线程数量，默认8个
     *   max_requests: 请求队列的最大容量，默认10000
     * 注意：创建线程池时立即启动所有工作线程
     */
    threadpool(int thread_number = 8, int max_requests = 10000);

    /* 析构函数
     * 功能：释放线程资源，设置停止标志
     * 注意：不会等待线程执行完所有任务，立即停止
     */
    ~threadpool();

    /* 向任务队列添加任务
     * 参数：
     *   request: 指向任务对象的指针
     * 返回值：
     *   true: 添加成功
     *   false: 队列已满，添加失败
     * 线程安全：是，通过互斥锁保护队列
     */
    bool append(T *request);

private:
    /* 工作线程的入口函数（静态成员函数）
     * 由于pthread_create只能接受C风格函数指针，所以必须是静态函数
     * 参数：
     *   arg: 指向线程池对象的指针
     * 返回值：
     *   线程退出时返回的指针，这里返回线程池对象指针
     * 工作原理：
     *   1. 将void*参数转换为threadpool*指针
     *   2. 调用线程池的run()方法执行真正的任务处理
     */
    static void *worker(void *arg);

    /* 工作线程的实际执行函数
     * 功能：不断从任务队列中取出任务并执行
     * 执行流程：
     *   1. 等待信号量（表示有任务可处理）
     *   2. 获取互斥锁
     *   3. 从队列中取出一个任务
     *   4. 释放互斥锁
     *   5. 执行任务
     *   6. 重复1-5，直到线程池停止
     */
    void run();

private:
    // 线程池配置参数
    int m_thread_number; // 线程池中的线程数量
    int m_max_requests;  // 请求队列的最大容量

    // 线程管理
    pthread_t *m_threads; // 线程ID数组，用于管理所有工作线程

    // 任务队列
    std::list<T *> m_workqueue; // 任务队列，存储待处理的任务指针

    // 线程同步工具
    locker m_queuelocker; // 互斥锁，保护任务队列的访问
    sem m_queuestat;      // 信号量，表示队列中的任务数量，用于线程同步

    // 控制标志
    bool m_stop; // 线程池停止标志，true表示停止
};

/*
 * 构造函数实现
 * 功能：初始化线程池，创建指定数量的工作线程
 * 注意：线程创建失败会抛出异常
 */
template <typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) : m_thread_number(thread_number), // 初始化线程数量
                                                                 m_max_requests(max_requests),   // 初始化队列容量
                                                                 m_stop(false),                  // 初始化停止标志为false
                                                                 m_threads(NULL)                 // 初始化线程数组指针为NULL
{
    // 参数有效性检查
    if ((thread_number <= 0) || (max_requests <= 0))
    {
        // 线程数或队列容量必须大于0
        throw std::exception(); // 抛出异常，终止构造函数
    }

    // 动态分配线程ID数组内存
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) // 检查内存分配是否成功
    {
        throw std::exception(); // 内存分配失败，抛出异常
    }

    // 创建并启动所有工作线程
    for (int i = 0; i < thread_number; ++i)
    {
        printf("create the %dth thread\n", i); // 调试信息

        /* 创建线程
         * 参数1: m_threads + i 指向线程ID的指针
         * 参数2: NULL 使用默认线程属性
         * 参数3: worker 线程入口函数（必须是静态成员函数）
         * 参数4: this 传递给线程函数的参数，这里是线程池对象指针
         */
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            // 创建线程失败，清理已分配的内存
            delete[] m_threads;
            throw std::exception(); // 抛出异常
        }

        /* 设置线程为分离状态
         * 分离状态：线程结束时自动释放资源，无需主线程调用pthread_join等待
         * 优点：避免资源泄漏
         * 缺点：无法获取线程返回值
         */
        if (pthread_detach(m_threads[i]))
        {
            // 设置分离状态失败，清理已分配的内存
            delete[] m_threads;
            throw std::exception(); // 抛出异常
        }
    }
}

/*
 * 析构函数实现
 * 功能：清理线程池资源
 * 注意：
 *   1. 设置停止标志，让工作线程退出循环
 *   2. 释放线程ID数组内存
 *   3. 不等待工作线程结束（因为它们是分离的）
 */
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads; // 释放线程ID数组
    m_stop = true;      // 设置停止标志，让工作线程退出循环
    // 注意：这里没有等待工作线程结束，因为它们是分离状态
}

/*
 * 添加任务到任务队列
 * 参数：request - 指向任务对象的指针
 * 返回值：true表示添加成功，false表示队列已满
 * 工作原理：
 *   1. 获取互斥锁，保护队列
 *   2. 检查队列是否已满
 *   3. 将任务添加到队列尾部
 *   4. 释放互斥锁
 *   5. 增加信号量，通知工作线程有新任务
 */
template <typename T>
bool threadpool<T>::append(T *request)
{
    // 步骤1：获取互斥锁，保护任务队列
    m_queuelocker.lock();

    // 步骤2：检查队列是否已满
    if (m_workqueue.size() > m_max_requests)
    {
        // 队列已满，释放锁并返回失败
        m_queuelocker.unlock();
        return false;
    }

    // 步骤3：将任务添加到队列尾部
    m_workqueue.push_back(request);

    // 步骤4：释放互斥锁
    m_queuelocker.unlock();

    // 步骤5：增加信号量值，通知等待的工作线程有新任务
    m_queuestat.post();

    return true; // 添加成功
}

/*
 * 工作线程的静态入口函数
 * 由于pthread_create要求C风格函数，所以必须是静态函数
 * 静态函数没有this指针，通过参数传递线程池对象
 */
template <typename T>
void *threadpool<T>::worker(void *arg)
{
    // 将void*参数转换为线程池对象指针
    threadpool *pool = (threadpool *)arg;

    // 调用线程池的run()方法，执行实际的任务处理
    pool->run();

    // 返回线程池对象指针（实际上不会被使用，因为线程是分离的）
    return pool;
}

/*
 * 工作线程的实际执行函数
 * 功能：不断从任务队列中取出任务并执行
 * 执行流程：
 *   1. 等待信号量（阻塞直到有任务）
 *   2. 获取互斥锁
 *   3. 检查队列是否为空（避免虚假唤醒）
 *   4. 从队列头部取出任务
 *   5. 释放互斥锁
 *   6. 执行任务
 *   7. 重复上述过程，直到停止标志为true
 */
template <typename T>
void threadpool<T>::run()
{
    // 主循环：不断处理任务，直到线程池停止
    while (!m_stop)
    {
        // 步骤1：等待信号量
        // 如果信号量值>0，则减1并继续执行
        // 如果信号量值=0，则阻塞直到有任务被添加
        m_queuestat.wait();

        // 步骤2：获取互斥锁，保护队列访问
        m_queuelocker.lock();

        // 步骤3：再次检查队列是否为空（避免虚假唤醒）
        if (m_workqueue.empty())
        {
            // 队列为空，释放锁并继续等待
            m_queuelocker.unlock();
            continue;
        }

        // 步骤4：从队列头部取出一个任务
        T *request = m_workqueue.front(); // 获取任务指针
        m_workqueue.pop_front();          // 从队列中移除任务

        // 步骤5：释放互斥锁
        m_queuelocker.unlock();

        // 步骤6：检查任务指针是否有效
        if (!request)
        {
            continue; // 任务指针无效，跳过
        }

        // 步骤7：执行任务的处理函数
        // 注意：这里调用了任务对象的process()方法
        // 模板类T必须实现process()方法
        // 这是多态性的体现，具体处理逻辑由任务类决定
        request->process();
    }

    // 当停止标志为true时，退出循环，线程结束
}

#endif