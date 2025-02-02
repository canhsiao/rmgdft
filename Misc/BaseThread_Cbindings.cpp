/*
 *
 * Copyright 2014 The RMG Project Developers. See the COPYRIGHT file 
 * at the top-level directory of this distribution or in the current
 * directory.
 * 
 * This file is part of RMG. 
 * RMG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * any later version.
 *
 * RMG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/


#include "BaseThread.h"
#include <mpi.h>
#include "rmg_error.h"
#include "const.h"
#include "transition.h"


void *run_threads(void *v);

// Non member functions used for handling thread specific data
void run_thread_tasks(int jobs)
{
    BaseThread *B = BaseThread::getBaseThread(0);
    B->run_thread_tasks(jobs);
}

void thread_barrier_wait(void) 
{
    BaseThread *B = BaseThread::getBaseThread(0);
    B->thread_barrier_wait(true);
}

int get_thread_basetag(void)
{
    BaseThread *B = BaseThread::getBaseThread(0);
    return B->get_thread_basetag();
}

void set_thread_basetag(int tid, int tag)
{
    BaseThread *B = BaseThread::getBaseThread(0);
    B->set_thread_basetag(tid, tag);
}

int get_thread_tid(void)
{
    BaseThread *B = BaseThread::getBaseThread(0);
    return B->get_thread_tid();
}

void init_HYBRID_MODEL(int npes, int thispe, int nthreads, MPI_Comm comm)
{

    InitHybridModel(nthreads, nthreads, npes, thispe, comm);

}

void RMG_MPI_lock(void)
{
    BaseThread *B = BaseThread::getBaseThread(0);
    B->RMG_MPI_lock();
}

void RMG_MPI_unlock(void)
{
    BaseThread *B = BaseThread::getBaseThread(0);
    B->RMG_MPI_unlock();
}

int is_loop_over_states(void) 
{
    BaseThread *B = BaseThread::getBaseThread(0);
    return B->is_loop_over_states();
}

void set_pptr(int tid, void *p)
{
    BaseThread *B = BaseThread::getBaseThread(0);
    B->set_pptr(tid, p);
}

