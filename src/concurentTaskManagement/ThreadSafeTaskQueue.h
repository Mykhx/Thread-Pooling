#ifndef THREADPOOLING_THREADSAFEQUEUE_H
#define THREADPOOLING_THREADSAFEQUEUE_H

#include <functional>
#include <queue>
#include <mutex>

/**
 * A simple implementation of a thread safe queue. For more appropriate versions see e.g.
 * "C++ concurrency in action" (2nd ed) by Anthony Williams (p.190ff), where
 * non-blocking alternatives are given.
 */

using GeneralTask = std::function<void()>;

class ThreadSafeTaskQueue {
public:
    /**
     * Returns the first element in the queue and removes it from the queue.
     * If the queue is empty, an empty task is returned.
     * @return is the first element in the queue.
     */
    GeneralTask pop() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (taskQueue.empty()) return [] {}; /* avoid undefined behaviour */
        auto item{std::move(taskQueue.front())};
        taskQueue.pop();
        return item;
    }

    /**
     * Returns the first element in the queue and removes it from the queue as a tuple.
     * The first value indicates the success of this action.
     * If the queue is empty, an empty task is returned indicated by the first value of the tuple being false.
     * In this case the second element of the queue contains an anonymous empty function.
     * @return a tuple containing stating the success of the action and the popped task if successful.
     */
    std::tuple<bool, GeneralTask> tryPop() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (taskQueue.empty()) return std::make_tuple(false, [] {}); /* avoid undefined behaviour */
        auto item{std::move(taskQueue.front())};
        taskQueue.pop();
        return std::make_tuple(true, std::move(item));
    }

    /**
     * Emplace new task in the queue.
     * @param task is a r-value reference to the task to emplace.
     */
    void emplace(GeneralTask &&task) {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.emplace(std::move(task));
    }

    /**
     * Check if the queue is empty.
     */
    bool isEmpty() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return taskQueue.empty();
    }

    /**
     * Returns the number of elements currently in the queue
     */
    unsigned long long size() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return taskQueue.size();
    }

    /**
     * Clear queue
     */
    void clear() {
        taskQueue = std::queue<GeneralTask>();
    }
    
private:
    std::queue<GeneralTask> taskQueue;
    std::mutex queueMutex;
};


#endif //THREADPOOLING_THREADSAFEQUEUE_H
