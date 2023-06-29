#ifndef THREADPOOLING_THREADPOOL_H
#define THREADPOOLING_THREADPOOL_H

#include <atomic>
#include <functional>
#include <thread>
#include <algorithm>

#include "ThreadSafeTaskQueue.h"
#include <iostream>
#include <shared_mutex>
#include <condition_variable>

using GeneraterTask = std::function<void()>;

/**
 * This class represents a basic thread pool following ideas presented in
 * "C++ concurrency in action" (2nd ed) by Anthony Williams (p.302).
 * Use of jthreads for simpler thread-lifetime management and a simple queue construct and loop structure.
 * The condition variable is similar to the one used for a simple task scheduler.
 * The threads sleep-cycle is controlled using a shared mutex and a condition variable.
 *
 * An improved version would manage local queues and fix uneven load balance (if possible).
 */
using namespace std::chrono_literals;

class ThreadPool {
protected:
    const unsigned int minimumWorker{1};
    std::vector<std::jthread> workerPool;
    ThreadSafeTaskQueue taskQueue;
    std::atomic_bool isDone;
    std::atomic_bool newTasksExpected;

    std::shared_mutex taskMutex;
    std::condition_variable_any taskConditionVariable;

    virtual void consumerThreadLoop(const std::stop_token &stopToken) {
        std::shared_lock<std::shared_mutex> sharedLock(taskMutex);
        while (!isDone) {
            if (stopToken.stop_requested()) break;
            auto [success, task] = taskQueue.tryPop();
            if (success) {
                std::invoke(task);
            } else {
                if (!newTasksExpected) break;
                taskConditionVariable.wait(sharedLock);
                //std::this_thread::yield();
            }
        }
    }

    void setupThreadWorkerPool(unsigned int numberWorkerThreads, bool verbose) {
        const unsigned int maximumSupportedWorkers{std::thread::hardware_concurrency()};
        numberWorkerThreads = std::min(numberWorkerThreads, maximumSupportedWorkers);
        numberWorkerThreads = std::max(numberWorkerThreads, minimumWorker);

        newTasksExpected = true;

        if (verbose) {
            std::cout << "I will use " << numberWorkerThreads << " thread(s) for this run.\n";
        }

        try { // starting threads may throw
            for (unsigned int i{0}; i < numberWorkerThreads; ++i) {
                auto thread = [this](std::stop_token &&stopToken) {
                    consumerThreadLoop(stopToken);
                };
                workerPool.emplace_back(std::move(thread));
            }
        } catch (...) {
            isDone = true;
            throw;
        }
    }

public:
    /**
     * Ctor for ThreadPool with a set number of consumer threads in the range of [1,std::thread::hardware_concurrency].
     * As such the number cannot exceed the hardware provided threads. If invalid input is given,
     * the closest valid number is taken instead. Note the number of threads only refers to the consumer threads.
     * @param numberConsumerThreads is the number of consumer threads.
     */
    explicit ThreadPool(unsigned int numberConsumerThreads, bool verbose = false) : isDone{false} {
        setupThreadWorkerPool(numberConsumerThreads, verbose);
    }

    /**
     * Wait for tasks to finish.
     */
    // Pseudo future
    virtual void waitForTasks() {
        newTasksExpected = false;
        taskConditionVariable.notify_all();
        for (auto &thread: workerPool) {
            thread.join();
        }
    }

    ~ThreadPool() {
        isDone = true;
    }

    /**
     * Submit a task to the queue
     * @param task the task to be submitted.
     */
    virtual void submitTask(GeneraterTask &&task) {
        taskQueue.emplace(std::move(task));
        taskConditionVariable.notify_all();
    }
};


#endif //THREADPOOLING_THREADPOOL_H
