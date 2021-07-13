#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <iostream>
#include <map>
#include <vector>
#include <thread>

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
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
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
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * WaitingTask
 */
struct WaitingTask {
    TaskID _id; // the TaskID of this task
    TaskID _depend_TaskID; // the maximum TaskID of this task's dependencies
    IRunnable* _runnable; // the bulk task
    int _num_total_tasks;
    WaitingTask(TaskID id, TaskID dependency, IRunnable* runnable, int num_total_tasks):_id{id}, _depend_TaskID{dependency}, _runnable{runnable}, _num_total_tasks{num_total_tasks}{};
    bool operator<(const WaitingTask& other) const {
        return _depend_TaskID > other._depend_TaskID;
    }
};

/*
 * Ready Task
 */
struct ReadyTask {
    TaskID _id;
    IRunnable* _runnable;
    int _current_task;
    int _num_total_tasks;
    ReadyTask(TaskID id, IRunnable* runnable, int num_total_tasks):_id(id), _runnable(runnable), _current_task{0}, _num_total_tasks(num_total_tasks) {}
    ReadyTask(){}
};


/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */

class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    private:
        // the maximum number of workerThread this TaskSystem can use
        int _num_Threads;

        // record the process of tasks which are in executing
        std::map<TaskID, std::pair<int, int> > _task_Process;
        std::mutex* _meta_data_mutex; // protect _task_Process and _finished_TaskID

        // task in the waiting queue with _depend_TaskID <= _finished_TaskID can be pushed into ready queue
        TaskID _finished_TaskID;         
        // wait/notify syn() thread
        std::condition_variable* _finished;



        // the next call to run or runAsyncWithDeps with get this TaskID back
        TaskID _next_TaskID;         

        // notify workerThreads to kill themselves
        bool _killed;         
        
        // worker thread pool
        std::thread* _threads_pool;

        // waiting task queue
        std::priority_queue<WaitingTask, std::vector<WaitingTask> > _waiting_queue;
        std::mutex* _waiting_queue_mutex;

        // ready task queue
        std::queue<ReadyTask> _ready_queue;
        std::mutex* _ready_queue_mutex;


    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void workerThread();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

#endif
