#ifndef THREADPOOLING_THREADLOCALQUEUETHREADPOOL_H
#define THREADPOOLING_THREADLOCALQUEUETHREADPOOL_H

#include <memory>
#include "ThreadPool.h"

#define MAX_NUM_TASKS_IN_LOCAL_QUEUE 500

/**
 * Experimental:
 * Use thread local queues to avoid (blocking) calls to underlying pool/main queue.
 */
class ThreadLocalQueueThreadPool : public ThreadPool {
public:
    explicit ThreadLocalQueueThreadPool(unsigned int numberConsumerThreads, bool verbose = false) : ThreadPool(
            numberConsumerThreads, verbose) {
    }

    /**
     * Submit a task to the queue. First try to insert into thread local queue
     * before checking the pool/main queue.
     * @param task the task to be submitted
     */
    void submitTask(GeneraterTask &&task) override {
        if (localTaskQueue != nullptr and localTaskQueue->size() < MAX_NUM_TASKS_IN_LOCAL_QUEUE) {
            localTaskQueue->emplace(std::move(task));
        } else {
            taskQueue.emplace(std::move(task));
            taskConditionVariable.notify_all();
        }
    }

    /**
      * Wait for tasks to finish.
      */
    void waitForTasks() override {
        newTasksExpected = false;
        taskConditionVariable.notify_all();
        for (auto &thread: workerPool) {
            thread.join();
        }
    }

private:
    static inline thread_local std::unique_ptr<std::queue<GeneralTask>> localTaskQueue;

    void consumerThreadLoop(const std::stop_token &stopToken) override {
        localTaskQueue = std::make_unique<std::queue<GeneralTask>>();

        std::shared_lock<std::shared_mutex> sharedLock(taskMutex);
        while (!isDone) {
            if (stopToken.stop_requested()) {
                localTaskQueue = std::make_unique<std::queue<GeneralTask>>();
                break;
            }

            if (localTaskQueue != nullptr and !localTaskQueue->empty()) {
                auto task{localTaskQueue->front()};
                localTaskQueue->pop();
                std::invoke(task);
            } else {
                auto [success, task] = taskQueue.tryPop();
                if (success) {
                    std::invoke(task);
                } else {
                    if (!newTasksExpected) break;
                    // alternative to: std::this_thread::yield();
                    taskConditionVariable.wait(sharedLock);
                }
            }
        }
    }

};

#endif //THREADPOOLING_THREADLOCALQUEUETHREADPOOL_H
