/******************************************************************************
 *   Copyright (C) 2011 - 2013  York Student Television
 *
 *   Tarantula is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tarantula is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tarantula.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Contact     : tarantula@ystv.co.uk
 *
 *   File Name   : AsyncJobSystem.h
 *   Version     : 1.0
 *   Description : 
 *
 *****************************************************************************/

#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>
#include <set>
#include <functional>
#include <memory>
#include <algorithm>
#include <chrono>

typedef std::function<void(std::shared_ptr<void>, std::timed_mutex&)> AsyncJobFunction;
typedef std::function<void(std::shared_ptr<void>)> AsyncJobCallback;

/**
 * Enum marking possible states of queued jobs
 */
enum AsyncJobState
{
    JOB_READY,   //!< JOB_READY     Job is ready to run
    JOB_RUNNING, //!< JOB_RUNNING   Job is currently active
    JOB_COMPLETE,//!< JOB_COMPLETE  Job has finished and is awaiting callback
    JOB_ERASE    //!< JOB_ERASE     Job has run callback and can be erased
};

/**
 * Structure describing a job to be executed asynchronously
 */
struct AsyncJobData
{
    AsyncJobFunction m_jobfunction;      //!< Function to execute to carry out the job
    AsyncJobCallback m_completecallback; //!< Function to execute synchronously when job completes
    std::shared_ptr<void> m_data;        //!< Data to pass into and out of the job
    int m_priority;                      //!< Priority (higher is better)
    AsyncJobState m_state;               //!< Flag to mark whether job has been run
    bool m_repeat;                       //!< Flag to mark whether job shall run until cancelled

    // Sort operator to keep in priority order
    bool operator< (AsyncJobData const &lhs) const
    {
        return (lhs.m_priority < m_priority);
    }
};

class AsyncJobSystem
{
public:
    AsyncJobSystem ();
    virtual ~AsyncJobSystem ();

    std::shared_ptr<AsyncJobData> newAsyncJob (AsyncJobFunction func, AsyncJobCallback cb,
            const std::shared_ptr<void> data, int priority, bool repeat);

    void completeAsyncJobs ();

private:
    void asyncJobRunner ();

    std::set<std::shared_ptr<AsyncJobData>> m_jobs; //! Async jobs queue

    // Condition variable (and mutex) for sleeping thread when no jobs are present
    std::condition_variable m_async_cv;
    std::mutex m_async_cv_mx;

    // Mutex for access to jobs queue
    std::mutex m_jobqueue_mutex;

    // Flag to stop the async thread
    bool m_halt;

    // Thread to run jobs
    std::thread m_async_thread;
};

