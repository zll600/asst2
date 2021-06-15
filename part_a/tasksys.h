#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <iostream>


/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    private:
        std::thread* threads_pool_;
        int num_threads_;

    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void threadRun(IRunnable* runnable, int num_total_tasks, std::mutex* mutex, int* curr_task);
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TasksState {
    public:
        std::mutex* mutex_;
        std::condition_variable* finished_;
        std::mutex* finishedMutex_;
        IRunnable* runnable_;
        int finished_tasks_;
        int left_tasks_;
        int num_total_tasks_;
        TasksState();
        ~TasksState();
};

class TaskWrapper {
    public:
        IRunnable* runnable_;
        TaskID id_;
        int num_total_tasks_;
        std::condition_variable* condition_variable_;
        std::mutex* mutex_;
        int* left_tasks_;
        TaskWrapper(IRunnable* runnable, const TaskID id, const int num_total_tasks, std::condition_variable* cv, std::mutex* mutex, int* left_tasks);
        ~TaskWrapper();
};

class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    private:
        TasksState* state_;
        std::thread* threads_pool_;
        bool killed;
        int num_threads_;
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void spinningThread();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};


/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    private:
        std::queue<TaskWrapper*>taskQueue;
        std::mutex* queueMutex;
        std::thread* threads_pool_;
        std::condition_variable* hasTasks; 
        std::mutex* hasTasksMutex;
        bool killed;
        int num_threads_;
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void sleepingThread();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

#endif
