#include "tasksys.h"

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
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->num_threads_ = num_threads;
    threads_pool_ = new std::thread[num_threads];
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {
    delete[] threads_pool_;
}

void TaskSystemParallelSpawn::threadRun(IRunnable* runnable, int num_total_tasks, std::mutex* mutex, int* curr_task) {
    int turn = -1;
    while (turn < num_total_tasks) {
        mutex->lock();
        turn = *curr_task;
        *curr_task += 1;
        mutex->unlock();
        if (turn >= num_total_tasks) {
            break;
        }
        runnable->runTask(turn, num_total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::mutex* mutex = new std::mutex();
    int* curr_task = new int;
    *curr_task = 0;
    for (int i = 0; i < num_threads_; i++) {
        threads_pool_[i] = std::thread(&TaskSystemParallelSpawn::threadRun, this, runnable, num_total_tasks, mutex, curr_task);
    }
    for (int i = 0; i < num_threads_; i++) {
        threads_pool_[i].join();
    }
    delete curr_task;
    delete mutex;
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

TaskWrapper::TaskWrapper(IRunnable* runnable, const TaskID id, const int num_total_tasks, std::condition_variable* cv, std::mutex* mutex, int* left_tasks)
                        :runnable_(runnable), id_(id), num_total_tasks_(num_total_tasks), condition_variable_(cv), mutex_(mutex), left_tasks_(left_tasks){}
TaskWrapper::~TaskWrapper() {}

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    killed = false;
    threads_pool_ = new std::thread[num_threads];
    queueMutex = new std::mutex();
    num_threads_ = num_threads;
    for (int i = 0; i < num_threads; i++) {
        threads_pool_[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::spinningThread, this);
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    killed = true;
    for (int i = 0; i < num_threads_; i++) {
        threads_pool_[i].join();
    }
    delete[] threads_pool_;
    delete queueMutex;
}

void TaskSystemParallelThreadPoolSpinning::spinningThread() {
    TaskWrapper* task;
    while (true)
    {
        if (killed) break;
        task = nullptr;
        queueMutex->lock();
        if (!taskQueue.empty()){
            task = taskQueue.front();
            taskQueue.pop();
        }
        queueMutex->unlock();
        if (task != nullptr) {
            (task->runnable_)->runTask(task->id_, task->num_total_tasks_);
            task->mutex_->lock(); 
            *(task->left_tasks_) = (*(task->left_tasks_)) - 1;
            if ((*task->left_tasks_) == 0) {
                task->mutex_->unlock();
                task->condition_variable_->notify_one();
                delete task;
                continue;
            }
            task->mutex_->unlock();
            delete task;
        }
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::condition_variable* cv = new std::condition_variable();
    std::mutex* mutex = new std::mutex();
    std::unique_lock<std::mutex> lk(*mutex);
    int* left_tasks = new int;
    *left_tasks = num_total_tasks;
    for (int i = 0; i < num_total_tasks; i++) {
        TaskWrapper* task = new TaskWrapper(runnable, i, num_total_tasks, cv, mutex, left_tasks);
        queueMutex->lock();
        taskQueue.push(task);
        queueMutex->unlock();
    }
    // go to sleep, wait for all the tasks to finish
    cv->wait(lk);
    delete mutex;
    //delete cv;
    delete left_tasks;
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
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
    killed = false;
    threads_pool_ = new std::thread[num_threads];
    queueMutex = new std::mutex();
    hasTasks = new std::condition_variable();
    hasTasksMutex = new std::mutex();
    num_threads_ = num_threads;
    for (int i = 0; i < num_threads; i++) {
        threads_pool_[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::sleepingThread, this);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    killed = true;
    for (int i = 0; i < num_threads_; i++) {
        hasTasks->notify_all();
    }
    for (int i = 0; i < num_threads_; i++) {
        threads_pool_[i].join();
    }
    delete[] threads_pool_;
    delete queueMutex;
    delete hasTasks;
    delete hasTasksMutex;
}

void TaskSystemParallelThreadPoolSleeping::sleepingThread() {
    TaskWrapper* task;
    while (true)
    {
        if (killed) break;
        task = nullptr;
        queueMutex->lock();
        while (true) {
            if (killed) break;
            if(!taskQueue.empty()){
                task = taskQueue.front();
                taskQueue.pop();
                break;
            } else {
                queueMutex->unlock();
                std::unique_lock<std::mutex> lk(*hasTasksMutex);
                hasTasks->wait(lk);
                queueMutex->lock();
            }
        }
        queueMutex->unlock();
        if (task != nullptr) {
            (task->runnable_)->runTask(task->id_, task->num_total_tasks_);
            task->mutex_->lock(); 
            *(task->left_tasks_) = (*(task->left_tasks_)) - 1;
            if ((*task->left_tasks_) == 0) {
                task->mutex_->unlock();
                task->condition_variable_->notify_one();
                delete task;
                continue;
            }
            task->mutex_->unlock();
            delete task;
        }
    }
}
void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::condition_variable* cv = new std::condition_variable();
    std::mutex* mutex = new std::mutex();
    std::unique_lock<std::mutex> lk(*mutex);
    int* left_tasks = new int;
    *left_tasks = num_total_tasks;
    for (int i = 0; i < num_total_tasks; i++) {
        TaskWrapper* task = new TaskWrapper(runnable, i, num_total_tasks, cv, mutex, left_tasks);
        queueMutex->lock();
        taskQueue.push(task);
        queueMutex->unlock();
        hasTasks->notify_all();
    }
    // go to sleep, wait for all the tasks to finish
    cv->wait(lk);
    delete mutex;
    //delete cv;
    delete left_tasks;
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
