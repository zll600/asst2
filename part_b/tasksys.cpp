#include "tasksys.h"
#include <algorithm>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    _num_Threads = num_threads;
    _killed = false;
    _finished_TaskID = -1;
    _finished = new std::condition_variable();
    _next_TaskID = 0;
    _waiting_queue_mutex = new std::mutex();
    _ready_queue_mutex = new std::mutex();
    _meta_data_mutex = new std::mutex();
    _threads_pool = new std::thread[num_threads];
    // TODO support multiple threads
    for (int i = 0; i < _num_Threads; i++) {
        _threads_pool[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::workerThread, this);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    _killed = true;
    for (int i = 0; i < _num_Threads; i++) {
        _threads_pool[i].join();
    }

    delete _waiting_queue_mutex;
    delete _ready_queue_mutex;
    delete _meta_data_mutex;
    delete _finished;
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    std::vector<TaskID> noDeps;
    runAsyncWithDeps(runnable, num_total_tasks, noDeps);
    sync();
}

void TaskSystemParallelThreadPoolSleeping::workerThread() {
    while (!_killed) {
        ReadyTask task;
        bool runTask = false;
        _ready_queue_mutex->lock();        
        if (_ready_queue.empty()) {
            // pop ready job from waiting queue
            _waiting_queue_mutex->lock();
            while (!_waiting_queue.empty()) {
                auto& nextTask = _waiting_queue.top();
                if (nextTask._depend_TaskID > _finished_TaskID) break;
                _ready_queue.push(ReadyTask(nextTask._id, nextTask._runnable, nextTask._num_total_tasks));
                _task_Process.insert({nextTask._id, {0, nextTask._num_total_tasks}});
                _waiting_queue.pop();
            }
            _waiting_queue_mutex->unlock();
        } else {
            // process ready job in the ready queue
            task = _ready_queue.front();
            if (task._current_task >= task._num_total_tasks) _ready_queue.pop();
            else {
                _ready_queue.front()._current_task++;
                runTask = true;
            }
        }
        _ready_queue_mutex->unlock();

        if (runTask) {
            task._runnable->runTask(task._current_task, task._num_total_tasks);
        
            // update the metadata
            _meta_data_mutex->lock();
            auto& [finished, total] = _task_Process[task._id];
            finished++;
            if (finished == total) {
                _task_Process.erase(task._id);
                _finished_TaskID = std::max(task._id, _finished_TaskID);
            }
            _meta_data_mutex->unlock();
        }
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {
    TaskID dependency = -1;
    if (!deps.empty()) {
        dependency = *std::max(deps.begin(), deps.end());
    }
    WaitingTask task(_next_TaskID, dependency, runnable, num_total_tasks);
    _waiting_queue_mutex->lock();
    _waiting_queue.push(std::move(task));
    _waiting_queue_mutex->unlock();

    return _next_TaskID++;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    // std::lock_guard<std::mutex> lock(*_meta_data_mutex);
    // if (_finished_TaskID + 1 == _next_TaskID) return;

    while (true) {
        std::lock_guard<std::mutex> lock(*_meta_data_mutex);
        if (_finished_TaskID + 1 == _next_TaskID) break;
   }
    return;
}
