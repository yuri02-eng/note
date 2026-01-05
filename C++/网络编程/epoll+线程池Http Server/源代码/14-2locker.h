#ifndef LOCKER_H
#define LOCKER_H

#include <exception>   // 异常处理
#include <pthread.h>   // POSIX线程库，包含互斥锁和条件变量
#include <semaphore.h> // POSIX信号量

/*
 * 信号量封装类
 * 封装了POSIX信号量操作，提供更简单的接口
 */
class sem
{
public:
    // 构造函数：初始化信号量
    sem()
    {
        // sem_init参数说明：
        // 1. &m_sem: 信号量指针
        // 2. 0: 表示信号量在当前进程的线程间共享（非0表示进程间共享）
        // 3. 0: 信号量的初始值
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception(); // 初始化失败抛出异常
        }
    }

    // 析构函数：销毁信号量
    ~sem()
    {
        sem_destroy(&m_sem);
    }

    // 等待信号量（P操作）
    // 如果信号量的值大于0，则减1并立即返回
    // 如果信号量的值为0，则阻塞直到信号量值大于0
    bool wait()
    {
        return sem_wait(&m_sem) == 0; // 成功返回true，失败返回false
    }

    // 发布信号量（V操作）
    // 将信号量的值加1，如果有线程阻塞在此信号量上，则唤醒其中一个
    bool post()
    {
        return sem_post(&m_sem) == 0; // 成功返回true，失败返回false
    }

private:
    sem_t m_sem; // POSIX信号量对象
};

/*
 * 互斥锁封装类
 * 封装了POSIX互斥锁操作，用于保护共享资源
 */
class locker
{
public:
    // 构造函数：初始化互斥锁
    locker()
    {
        // pthread_mutex_init参数说明：
        // 1. &m_mutex: 互斥锁指针
        // 2. NULL: 使用默认属性
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception(); // 初始化失败抛出异常
        }
    }

    // 析构函数：销毁互斥锁
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    // 加锁
    // 如果互斥锁已被锁定，则阻塞直到获得锁
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0; // 成功返回true，失败返回false
    }

    // 解锁
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0; // 成功返回true，失败返回false
    }

private:
    pthread_mutex_t m_mutex; // POSIX互斥锁对象
};

/*
 * 条件变量封装类
 * 封装了POSIX条件变量操作，用于线程间同步
 * 条件变量通常与互斥锁配合使用
 */
class cond
{
public:
    // 构造函数：初始化条件变量和关联的互斥锁
    cond()
    {
        // 初始化互斥锁
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }

        // 初始化条件变量
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            // 如果条件变量初始化失败，需要销毁已创建的互斥锁
            pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }

    // 析构函数：销毁条件变量和互斥锁
    ~cond()
    {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }

    /*
     * 等待条件变量
     * 步骤：
     * 1. 获取互斥锁
     * 2. 调用pthread_cond_wait释放互斥锁并等待条件变量
     * 3. 被唤醒后自动重新获取互斥锁
     * 4. 释放互斥锁
     */
    bool wait()
    {
        int ret = 0;
        pthread_mutex_lock(&m_mutex);               // 步骤1：获取互斥锁
        ret = pthread_cond_wait(&m_cond, &m_mutex); // 步骤2-3：等待条件变量
        pthread_mutex_unlock(&m_mutex);             // 步骤4：释放互斥锁
        return ret == 0;                            // 成功返回true，失败返回false
    }

    // 唤醒一个等待该条件变量的线程
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0; // 成功返回true，失败返回false
    }

private:
    pthread_mutex_t m_mutex; // 与条件变量配合使用的互斥锁
    pthread_cond_t m_cond;   // POSIX条件变量对象
};

#endif