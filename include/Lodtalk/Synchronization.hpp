#ifndef LODTALK_SYNCHRONIZATION_HPP
#define LODTALK_SYNCHRONIZATION_HPP

#include <mutex>
#include <condition_variable>

namespace Lodtalk
{
/**
 * Shared mutex
 */
class SharedMutex
{
public:
    SharedMutex()
        : readCount(0), writeCount(0) {}

    void read_lock()
    {
        std::unique_lock<std::mutex> l(mutex);
        while(writeCount != 0)
            noReadWriteCondition.wait(l);
        assert(writeCount == 0);
        ++readCount;
    }

    void read_unlock()
    {
        std::unique_lock<std::mutex> l(mutex);
        assert(writeCount == 0);
        assert(readCount > 0);
        --readCount;
        if(!readCount)
            noReadCondition.notify_one();
    }

    void write_lock()
    {
        std::unique_lock<std::mutex> l(mutex);
        while(writeCount != 0 && readCount != 0)
            noReadCondition.wait(l);
        assert(readCount == 0);
        assert(writeCount == 0);
        writeCount = 1;
    }

    void write_unlock()
    {
        std::unique_lock<std::mutex> l(mutex);
        assert(writeCount == 1);
        assert(readCount == 0);
        writeCount = 0;
        noReadCondition.notify_one();
        noReadWriteCondition.notify_all();
    }

    void lock()
    {
        write_lock();
    }

    void unlock()
    {
        write_unlock();
    }

private:
    std::mutex mutex;
    std::condition_variable noReadCondition;
    std::condition_variable noReadWriteCondition;

    int readCount;
    int writeCount;
};

/**
 * Read lock
 */
template<typename T>
class ReadLock
{
public:
    ReadLock(T &mutex)
        : mutex(mutex)
    {
        mutex.lock();
    }

    ~ReadLock()
    {
        mutex.unlock();
    }

private:
    T &mutex;
};

/**
 * Write lock
 */
template<typename T>
class WriteLock
{
public:
    WriteLock(T &mutex)
        : mutex(mutex)
    {
        mutex.lock();
    }

    ~WriteLock()
    {
        mutex.unlock();
    }

private:
    T &mutex;
};

template<>
class ReadLock<SharedMutex>
{
public:
    ReadLock(SharedMutex &mutex)
        : mutex(mutex)
    {
        mutex.read_lock();
    }

    ~ReadLock()
    {
        mutex.read_unlock();
    }

private:
    SharedMutex &mutex;
};

};

#endif //LODTALK_SYNCHRONIZATION_HPP
