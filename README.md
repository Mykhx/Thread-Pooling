# Thread-Pooling
A basic thread pool implementation for educational purposes only.

A part of a small project implementing and testing out a basic thread management pool.
This was separated from the rest of the project for easier modification and possible further development.

ThreadPool.h:
A basic thread pool distributing work using a thread-safe blocking queue incorporating ideas from a TaskScheduler written for another project.

ThreadLocalQueueThreadPool.h:
A modification of ThreadPool.h complimenting the shared queue by thread-local standard queues (std::queue),
which reduces the access to the shared thread-safe blocking queue.
Tasks created by a thread during execution may either be added to its own (thread-local) queue
or the shared queue based on the current load or size of the local queue.

This was done for educational purposes and contains ideas inspired by "C++ Concurrency In Action" (2nd ed) by Anthony Williams.

As such see https://www.boost.org/LICENSE_1_0.txt (Boost Software Licence) for licensing information.
