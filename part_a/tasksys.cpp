#include "tasksys.h"

#include <atomic>

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
TasksState::TasksState() {
    mutex_ = new std::mutex();
    finished_ = new std::condition_variable();
    finishedMutex_ = new std::mutex();
    runnable_ = nullptr;
    finished_tasks_ = -1;
    left_tasks_ = -1;
    num_total_tasks_ = -1;
}

TasksState::~TasksState() {
    delete mutex_;
    delete finished_;
    delete finishedMutex_;
}

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
    killed_ = false;
    num_threads_ = num_threads;
    state_ = new TasksState;
    threads_pool_ = new std::thread[num_threads];
    for (int i = 0; i < num_threads; i++) {
        threads_pool_[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::spinningThread, this);
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    killed_ = true;
    for (int i = 0; i < num_threads_; i++) {
		if (threads_pool_[i].joinable()) {
			threads_pool_[i].join();
		}
    }
    delete[] threads_pool_;
    delete state_;
}

void TaskSystemParallelThreadPoolSpinning::spinningThread() {
    while (true) {
        if (killed_) {
			break;
		}

        state_->mutex_->lock();
        int total = state_->num_total_tasks_;
        int id = total - state_->left_tasks_;
        if (id < total) state_->left_tasks_--;
        state_->mutex_->unlock();

        if (id < total) {
            state_->runnable_->runTask(id, total);
            state_->mutex_->lock();
            state_->finished_tasks_++;
            if (state_->finished_tasks_ == total) {
                state_->mutex_->unlock();
                state_->finished_->notify_all();
            }else {
                state_->mutex_->unlock();
            }
        }
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> lk(*(state_->finishedMutex_));
    state_->mutex_->lock();
    state_->finished_tasks_ = 0;
    state_->left_tasks_ = num_total_tasks;
    state_->num_total_tasks_ = num_total_tasks;
    state_->runnable_ = runnable;
    state_->mutex_->unlock();
    // go to sleep, wait for all the tasks to finish
    //std::cerr << "go to sleep" << std::endl;
    state_->finished_->wait(lk); 
    lk.unlock();
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
    state_ = new TasksState;
    killed_ = false;
    threads_pool_ = new std::thread[num_threads];
    num_threads_ = num_threads;
    has_tasks_ = new std::condition_variable();
    has_tasks_mutex_ = new std::mutex();
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
    killed_ = true;
    for (int i = 0; i < num_threads_; i++) {
        has_tasks_->notify_all();
    }
    for (int i = 0; i < num_threads_; i++) {
		if (threads_pool_[i].joinable()) {
			threads_pool_[i].join();
		}
    }
    delete[] threads_pool_;
    delete state_;
    delete has_tasks_;
    delete has_tasks_mutex_;
}

void TaskSystemParallelThreadPoolSleeping::sleepingThread() {
    while (true)
    {
        if (killed_) {
			break;
		}

        state_->mutex_->lock();
        int total = state_->num_total_tasks_;
        int id = total - state_->left_tasks_;
        if (id < total) {
			state_->left_tasks_--;
		}
        state_->mutex_->unlock();

        if (id < total) {
            state_->runnable_->runTask(id, total);
            state_->mutex_->lock();
            state_->finished_tasks_++;
            if (state_->finished_tasks_ == total) {
                state_->mutex_->unlock();

                state_->finished_->notify_all();
            }else {
                state_->mutex_->unlock();
            }
        } else {
            std::unique_lock<std::mutex> lk(*has_tasks_mutex_);
            has_tasks_->wait(lk);
            lk.unlock();
        }
    }
}
void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> lk(*(state_->finishedMutex_));
    state_->mutex_->lock();
    state_->finished_tasks_ = 0;
    state_->left_tasks_ = num_total_tasks;
    state_->num_total_tasks_ = num_total_tasks;
    state_->runnable_ = runnable;
    state_->mutex_->unlock();

    for (int i = 0; i < num_total_tasks; i++) {
        has_tasks_->notify_all();
	}
    // go to sleep, wait for all the tasks to finish
    //std::cerr << "go to sleep" << std::endl;
    state_->finished_->wait(lk); 
    lk.unlock();

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
