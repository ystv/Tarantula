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
 *   File Name   : AsyncJobSystem.cpp
 *   Version     : 1.0
 *   Description : 
 *
 *****************************************************************************/

#include "AsyncJobSystem.h"
#include "TarantulaCore.h"
#include "Log.h"

extern std::timed_mutex g_core_lock;

/**
 * Constructor. Launches the job running thread
 */
AsyncJobSystem::AsyncJobSystem ():
        m_halt(false),
        m_async_thread(std::bind(&AsyncJobSystem::asyncJobRunner, this))
{

}

/**
 * Destructor. Stops the thread
 */
AsyncJobSystem::~AsyncJobSystem ()
{
    // Set the kill flag
    m_halt = true;

    // Add a new empty job to force the wait to exit
    m_jobs.insert(std::make_shared<AsyncJobData>());
    m_async_cv.notify_one();

    m_async_thread.join();
}

/**
 * Add a new job to the queue
 *
 * @param func      Function to call to run the job
 * @param cb        Function to call synchronously after job has completed
 * @param data      Data to pass to the job (void pointer)
 * @param priority  Numerical priority of the job, higher priorities will run first
 * @param repeat    Should the job repeat indefinitely?
 * @return          Pointer to the new job to allow later updates
 */
std::shared_ptr<AsyncJobData> AsyncJobSystem::newAsyncJob (AsyncJobFunction func, AsyncJobCallback cb,
        const std::shared_ptr<void> data, int priority, bool repeat)
{
    std::shared_ptr<AsyncJobData> newjob = std::make_shared<AsyncJobData>();

    newjob->m_jobfunction = func;
    newjob->m_completecallback = cb;
    newjob->m_data = data;
    newjob->m_priority = priority;
    newjob->m_repeat = repeat;
    newjob->m_state = JOB_READY;

    // Lock the job queue and add the new job
    {
    	std::lock_guard<std::mutex> lock(m_jobqueue_mutex);
    	m_jobs.insert(newjob);
    }

    // Wake the job runner
    m_async_cv.notify_one();

    return newjob;

}

/**
 * Locate completed jobs in the queue, execute their callbacks and remove if necessary
 */
void AsyncJobSystem::completeAsyncJobs ()
{
    // Run callbacks and update job states
    for (std::set<std::shared_ptr<AsyncJobData>>::iterator thisjob = m_jobs.begin(); thisjob != m_jobs.end(); )
    {
        if (JOB_COMPLETE == (*thisjob)->m_state)
        {
            if ((*thisjob)->m_completecallback)
            {
                (*thisjob)->m_completecallback((*thisjob)->m_data);
            }

            if (true == (*thisjob)->m_repeat)
            {
                std::lock_guard<std::mutex> lock(m_jobqueue_mutex);
                (*thisjob)->m_state = JOB_READY;
            }
            else
            {
                std::lock_guard<std::mutex> lock(m_jobqueue_mutex);
                // Job does not repeat and should be erased. Loop will be advanced
                thisjob = m_jobs.erase(thisjob);
                continue;
            }
        }
        else if (JOB_FAILED == (*thisjob)->m_state)
        {
            g_logger.warn("Job Runner" + ERROR_LOC, "A job threw an unhandled exception");
            (*thisjob)->m_state = JOB_ERASE;
        }

        ++thisjob;
    }
}

/**
 * Run asynchronous tasks in a separate thread
 */
void AsyncJobSystem::asyncJobRunner ()
{
    // Template job for comparison
    std::shared_ptr<AsyncJobData> template_job = std::make_shared<AsyncJobData>();
    template_job->m_state = JOB_READY;

    while (!m_halt)
    {
        if (m_jobs.size() > 0)
        {
            // Get a reference to the job in question
            std::shared_ptr<AsyncJobData> runjob;

            // Lock the mutex to allow searching the job list
            {
            	std::lock_guard<std::mutex> lock(m_jobqueue_mutex);

				// Find a runnable job
				for (std::shared_ptr<AsyncJobData> thisjob : m_jobs)
				{
					if (JOB_READY == thisjob->m_state)
					{
						runjob = thisjob;
						break;
					}
				}
            }

            if (runjob)
            {
                // Run the job
                runjob->m_state = JOB_RUNNING;

                try
                {
                    runjob->m_jobfunction(runjob->m_data, g_core_lock);
                    runjob->m_state = JOB_COMPLETE;
                }
                catch (...)
                {
                    runjob->m_state = JOB_FAILED;
                }
            }
        }
        else
        {
            // Go to sleep until notified of new job by newAsyncJob()
            std::unique_lock<std::mutex> lk(m_async_cv_mx);
            m_async_cv.wait(lk, [this, template_job]{return m_jobs.size() > 0;});
        }
    }
}

