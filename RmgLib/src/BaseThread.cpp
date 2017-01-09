/*
 *
 * Copyright (c) 2011, 2014, Emil Briggs
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
*/

#include "BaseThread.h"
#include "rmg_error.h"
#include "BaseThreadControl.h"


// Init flag
int BaseThread::init_flag=0;

// Exit flag
bool BaseThread::exit_flag=false;

// Main thread control structure
BaseThreadControl thread_controls[MAX_RMG_THREADS];

// Condition variable and mutex for threads
std::mutex BaseThread::thread_mutex;
std::condition_variable BaseThread::thread_cv;

// Condition variable and mutex for main
std::mutex BaseThread::main_mutex;
std::condition_variable BaseThread::main_cv;

std::mutex BaseThread::mpi_mutex;
std::mutex BaseThread::thread_order_mutex;

// Pointer to the singleton
BaseThread *BaseThread::instance = NULL;

// Pointer to the thread function
void *(*BaseThread::funcptr)(void *s) = NULL;

boost::thread_group threadgroup;

// Constructor
BaseThread::BaseThread(int nthreads)
{

    int ncpus;

    if(nthreads > MAX_RMG_THREADS)
        rmg_error_handler (__FILE__, __LINE__, "Too many threads requested. Change MAX_RMG_THREADS and recompile if needed.");

    if(!BaseThread::init_flag) {

        BaseThread::THREADS_PER_NODE = nthreads;
        BaseThread::in_threaded_region = false;

        BaseThread::init_flag = 1;

    }

}

BaseThread *BaseThread::getBaseThread(int nthreads)
{
    if(!BaseThread::init_flag) {
        BaseThread::instance = new BaseThread(nthreads);
    }
    return BaseThread::instance;

}

void BaseThread::RegisterThreadFunction(void *(*funcptr)(void *s))
{
    int thread;

    BaseThread::funcptr = funcptr;

    // Create a set of long lived threads
    for(thread = 0;thread < BaseThread::THREADS_PER_NODE;thread++) 
    {
        thread_controls[thread].tid = thread;
        threadgroup.create_thread(boost::bind(BaseThread::funcptr, (void *)&thread_controls[thread]));
    }
    
}

// Wakes jobs sleeping threads starting from tid=0 and counting up
// jobs must be less than THREADS_PER_NODE
void BaseThread::run_thread_tasks(int jobs) {

    if(jobs > BaseThread::THREADS_PER_NODE) {
        // If this happens it is a bug
        rmg_error_handler (__FILE__, __LINE__, "More jobs than available threads scheduled\n");
    }

    BaseThread::scf_barrier = new boost::barrier(jobs);
    std::unique_lock<std::mutex> lk(BaseThread::thread_mutex);
    BaseThread::in_threaded_region = true;
    BaseThread::jobs = jobs;
    BaseThread::thread_cv.notify_all();
    BaseThread::main_cv.wait(lk, [&] {return BaseThread::jobs == 0;});
    BaseThread::in_threaded_region = false;
    delete(BaseThread::scf_barrier);
 
}

// Called from main when we terminate all threads
void BaseThread::wake_all_threads(void)
{
    BaseThread::thread_cv.notify_all();
}

void BaseThread::thread_sleep(void)
{
    std::unique_lock<std::mutex> lk(BaseThread::thread_mutex);
    if(BaseThread::jobs > 0) {
        BaseThread::jobs--;
        BaseThread::main_cv.notify_one();
    }
    BaseThread::thread_cv.wait(lk);
}

void BaseThread::thread_joinall(void)
{
    threadgroup.join_all();
}

bool BaseThread::get_exitflag(void)
{
    return BaseThread::exit_flag;
}

void BaseThread::set_exitflag(bool flag)
{
    BaseThread::exit_flag = flag;
}

// Blocks all threads until nthreads specified in the init call have reached this point
void BaseThread::thread_barrier_wait(void) {
    if(!BaseThread::in_threaded_region) return;
    BaseThread::scf_barrier->wait();
}

// Reads the basetag from the thread specific data. Returns 0 if we are not in
// a parallel region. Meant to be called from a thread.
int BaseThread::get_thread_basetag(void) {

    BaseThreadControl *ss;
    if(!BaseThread::in_threaded_region) return 0;
    ss = rmg_get_tsd();
    if(!ss) return 0;

    return ss->basetag;

}

// Sets a basetag value. Meant to be called from the main program.
void BaseThread::set_thread_basetag(int tid, int tag)
{
    thread_controls[tid].basetag = tag;
}

// Gets the threads control structure pointer
BaseThreadControl *BaseThread::get_thread_control(void) {
    BaseThreadControl *ss;
    if(!BaseThread::in_threaded_region) return NULL;
    ss = rmg_get_tsd();
    if(!ss) return NULL;
    return ss;
}

// Reads the tid from the thread specific data. Returns -1 if we are not in 
// a parallel region
int BaseThread::get_thread_tid(void) {

    BaseThreadControl *ss;

    if(!BaseThread::in_threaded_region) return -1;
    ss = rmg_get_tsd();
    if(!ss) return -1;

    return ss->tid;
}


// Used for positioning and setting processor affinity. For now assumes that
// THREADS_PER_NODE is an even multiple of ct.ncpus. If this is not true it
// does not attemp to schedule
void BaseThread::set_cpu_affinity(int tid, int procs_per_node, int local_rank)
{
#if __linux__
    int s;
    cpu_set_t cpuset;
    pthread_t thread;
    int cores = sysconf( _SC_NPROCESSORS_ONLN );
    if(procs_per_node > cores) return;

    if(BaseThread::THREADS_PER_NODE % (cores / procs_per_node)) return;

    s = tid % BaseThread::THREADS_PER_NODE + local_rank * BaseThread::THREADS_PER_NODE;
//printf("THREAD1  %d  %d  %d\n",tid, s, local_rank);

    // Set affinity mask
    CPU_ZERO(&cpuset);
    CPU_SET(s, &cpuset);

    thread = pthread_self();
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#endif
}

void BaseThread::RMG_MPI_lock(void) {
    BaseThread::mpi_mutex.lock();
}

void BaseThread::RMG_MPI_unlock(void) {
    BaseThread::mpi_mutex.unlock();
}

int BaseThread::is_loop_over_states(void)
{
    BaseThreadControl *ss;
    if(!BaseThread::in_threaded_region) return 0;
    ss = rmg_get_tsd();
    if(!ss) return 0;
    return 1;
}

int BaseThread::get_threads_per_node(void)
{
    if(BaseThread::THREADS_PER_NODE == 0)
        rmg_error_handler (__FILE__, __LINE__, "Threads not initialized yet");
    return BaseThread::THREADS_PER_NODE;
}

void BaseThread::set_pptr(int tid, void *p)
{
    thread_controls[tid].pptr = p;
}


// Non member functions used for handling thread specific data
void tsd_cleanup(BaseThreadControl *T)
{
}
static boost::thread_specific_ptr<BaseThreadControl> my_ptr(tsd_cleanup);
void rmg_set_tsd(BaseThreadControl *p)
{
    if(!my_ptr.get()) {
       my_ptr.reset(p);
    }
}
BaseThreadControl *rmg_get_tsd(void) {
    return my_ptr.get();
}
void rmg_release_tsd(void)
{
    my_ptr.release();
}
