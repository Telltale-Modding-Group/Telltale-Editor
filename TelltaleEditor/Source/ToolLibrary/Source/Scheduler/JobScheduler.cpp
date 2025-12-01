#include <Scheduler/JobScheduler.hpp>
#include <Meta/Meta.hpp> // for lua modules
#include <Scripting/ScriptManager.hpp>
#include <Resource/ResourceRegistry.hpp>

JobScheduler *JobScheduler::Instance = 0;

thread_local JobThread* MyLocalThread = nullptr;

void JobScheduler::Initialise(LuaFunctionCollection Col)
{
    if (Instance == 0)
        Instance = TTE_NEW(JobScheduler, MEMORY_TAG_SCHEDULER, std::move(Col));
}

void JobScheduler::Shutdown()
{
    if (Instance != nullptr)
        TTE_DEL(Instance);
    Instance = 0;
}

Bool JobScheduler::IsRunningFromWorker()
{
    return MyLocalThread != nullptr;
}

JobThread& JobScheduler::GetCurrentThread()
{
    TTE_ASSERT(IsRunningFromWorker(), "Not running from a worker thread!");
    return *MyLocalThread;
}

// This is the function run per job thread, which runs the jobs.
void JobScheduler::_JobThreadFn(JobScheduler &scheduler, U32 threadIndex)
{
    // Initialise.
    JobThread myself{};
    myself.FastBufferSize = FAST_BUFFER_SIZE;
    myself.FastBufferOffset = 0;
    myself.FastBuffer = TTE_ALLOC(FAST_BUFFER_SIZE, MEMORY_TAG_RUNTIME_BUFFER);
    myself.ThreadNumber = threadIndex;
    myself.ThreadName = String("Worker Thread ") + std::to_string(threadIndex);
    TTE_ATTACH_DBG_STR(myself.FastBuffer, myself.ThreadName + " Fast Buffer Memory");
    myself.L.Initialise(LuaVersion::LUA_5_2_3); // latest
    MyLocalThread = &myself;
    
    InjectFullLuaAPI(myself.L, true);
    ScriptManager::RegisterCollection(myself.L, scheduler._workerScriptCollection); // register anything else
    
    SetThreadName(myself.ThreadName); // Set name in debugger for future use
    
    // Main job thread loop.
    for (;;)
    {
        
        JobThreadWaitResult wResult = scheduler._WaitJobThread(true, myself); // Wait until kicked off.
        
        if (wResult == JOB_THREAD_RESULT_CANCEL_EXIT)
            break; // Result says to exit, so just exit and join.
        else if (myself.CurrentJob.RunnableFunction == NULL)
            continue;
        
        // We now have a job to run, so run it here.
        JobThreadWaitResult result = _JobThreadRunJob(scheduler, myself);
        if (result == JOB_THREAD_RESULT_CANCEL_EXIT)
            break; // Exit. If result was just a normal exit, wait job thread will keep returning jobs until no are left.
    }
    
    myself.FastBufferSize = myself.FastBufferOffset = 0;
    TTE_FREE(myself.FastBuffer);
    myself.FastBuffer = nullptr;
    MyLocalThread = nullptr;
    // Join with main thread.
}

JobThreadWaitResult JobScheduler::_JobThreadRunJob(JobScheduler &scheduler, JobThread &myself)
{
    // Run the job
    Bool jbResult = myself.CurrentJob.RunnableFunction(myself, myself.CurrentJob.UserArgA, myself.CurrentJob.UserArgB);
    JobResult jResult = jbResult ? JOB_RESULT_OK : JOB_RESULT_FAIL;
    
    // Check if we need to exit.
    if (scheduler._WaitJobThread(false, myself /*untouched*/) ==
        JOB_THREAD_RESULT_CANCEL_EXIT) // (Query, not wait) If we want to cancel jobs, exit quickly now
        return JOB_THREAD_RESULT_CANCEL_EXIT;
    
    // Signal its finished so the user can check.
    scheduler._GetSetCounter(myself.CurrentJob.JobID, jResult);
    
    // Cache the job ID as CurrentJob is not on the stack, and it changed with each call to this function
    U32 jobID = myself.CurrentJob.JobID;
    
    // Dequeue any jobs and keep running them until no more to run.
    Job enqueued{};
    while (scheduler._DequeueJob(jobID, enqueued))
    {
        
        myself.CurrentJob = std::move(enqueued);
        
        // Run it
        JobThreadWaitResult queuedResult = _JobThreadRunJob(scheduler, myself);
        if (queuedResult == JOB_THREAD_RESULT_CANCEL_EXIT)
            return queuedResult; // Skip any ref decrements, we are exiting anyway.
    }
    
    // Decrement the references to it, as now the scheduler doesn't need to know about it - we have gone through all enqueued ones to run here.
    scheduler._DecrementRefs(jobID, true);
    
    return JOB_THREAD_RESULT_OK; // No need to exit.
}

void JobScheduler::_SignalJobThread(U32 numSignals /*= 1*/)
{
    std::lock_guard<std::mutex> _Guard(_jobThreadNotifierLock);
    _jobThreadNotifierCount += numSignals; // Increase semaphore counter
    
    // If numSignals >= NUM_SCHEDULER_THREADS, notify all, else loop that many times and notify one.
    if (numSignals >= NUM_SCHEDULER_THREADS)
        _jobThreadNotifierCond.notify_all(); // All need to wake up
    else
    {
        while (numSignals--)
            _jobThreadNotifierCond.notify_one(); // Notify a thread to wake up
    }
}

JobThreadWaitResult JobScheduler::_WaitJobThread(Bool bWaitSemaphore, JobThread &myself)
{
    JobThreadWaitResult Result;
    Bool NeedsWait = false;
    Bool IsRunning = false;
    {
        // Check if we need to exit.
        Result = (IsRunning = _running.load(std::memory_order_acquire)) && _cancelJobs ? JOB_THREAD_RESULT_CANCEL_EXIT : JOB_THREAD_RESULT_OK;
        
        // If we are only checking running state, return now. Also if we need to exit, no need to wait on this thread so just return we need to exit.
        if (!bWaitSemaphore || Result == JOB_THREAD_RESULT_CANCEL_EXIT)
            return Result;
        
        // Here we wait.
        std::unique_lock<std::mutex> _Guard(_jobThreadNotifierLock); // Unique lock: destructor unlocks if locked
        
        // Now wait until signalled.
        
        // Use the condition variable to wait
        while (!_jobThreadNotifierCount)
            _jobThreadNotifierCond.wait(_Guard); // Try and lock the mutex, loop handles any spurious wakeups
        
        // Lock is aquired. First check if any job is available, if not, don't decrement counter and rewait for a bit.
        // It may take time for this thread to be run. One signal => one job run! So if no job is run and we decrement it
        // then we may cause all threads to be sleeping with jobs available and only run until next post job is called.
        // So rewait a bit. Deadlock won't occur, the notifier lock is only locked before here and has higher priority.
        
        {
            std::lock_guard<std::mutex> _Guard(_jobsLock);
            if (_pendingJobs.size())
            {
                Bool hasJob = false;
                typename std::vector<Job>::iterator foundIt = {};
                for(auto it = _pendingJobs.get_container().begin(); it != _pendingJobs.get_container().end(); it++)
                {
                    if(it->AffinityOverride == -1 || it->AffinityOverride == (I32)myself.ThreadNumber)
                    {
                        foundIt = it;
                        hasJob = true;
                        break;
                    }
                }
                if(hasJob)
                {
                    //  Found a job! Move highest priority job. NOTE: const cast needed (c++ spec...)
                    myself.CurrentJob = std::move(const_cast<Job &>(*foundIt));
                    _pendingJobs.get_container().erase(foundIt);
                }
                else
                {
                    NeedsWait = true;
                }
            }
            else
            {
                if (!IsRunning)
                    return JOB_THREAD_RESULT_CANCEL_EXIT; // We aren't running, and no more jobs available
                NeedsWait = true;                         // No jobs. Retry later.
            }
        }
        
        // If we need a wait then wait a small time else decrement (success, job found) - do this outside the lock!
        if (!NeedsWait)
        {
            // OK, decrement.
            _jobThreadNotifierCount--;
        }
    }
    
    if (NeedsWait)
    {
        ThreadSleep(15);                           // Wait for a bit, thread will retry after
        myself.CurrentJob.RunnableFunction = NULL; // Let thread know we have to re-wait
    }
    
    return Result; // And unlock (destructor)
}

void JobScheduler::_IncrementRefs(U32 job)
{
    // Simply lock the array and increment the references.
    std::lock_guard<std::mutex> _Guard(_aliveJobsLock);
    auto it = _jobCounters.find(job);
    if (it != _jobCounters.end())
        it->second.Refs++;
}

void JobScheduler::_DecrementRefs(U32 job, Bool bReleaseScheduler)
{
    _aliveJobsLock.lock();
    auto it = _jobCounters.find(job);
    if (it != _jobCounters.end())
    {
        if (bReleaseScheduler)
            it->second.SchedulerReleased = true;
        if ((--it->second.Refs) == 0)
        {
            // No more references, so free it.
            _jobCounters.erase(it);
            // There may still be enqueued jobs in the array. Move the array to the pending jobs array.
            auto it = _enqueuedJobs.find(job);
            if (it != _enqueuedJobs.end())
            {
                auto jobList = std::move(it->second);
                _enqueuedJobs.erase(it);
                
                // Finished with enqueued job array, so release lock and now lock jobs array so append the local jobList to it
                _aliveJobsLock.unlock();
                
                _jobsLock.lock();
                
                // Append the jobs
                if (jobList.NumQueued == 1)
                { // Only one in the array
                    _pendingJobs.push(std::move(jobList.Singular));
                    jobList.Singular.~Job();
                }
                else if (jobList.NumQueued > 1)
                { // Multiple, append all.
                    while (jobList.Queued.size())
                    {
                        _pendingJobs.push(std::move(const_cast<Job &>(jobList.Queued.top())));
                        jobList.Queued.pop();
                    }
                    jobList.Queued.~hacked_priority_queue();
                }
                jobList.NumQueued = 0;
                
                _jobsLock.unlock();
                
                return; // No more unlocks needed
            }
        }
    }
    _aliveJobsLock.unlock();
}

JobResult JobScheduler::_GetSetCounter(U32 job, JobResult newValue /*= JOB_RESULT_NONE*/)
{
    std::lock_guard<std::mutex> _Guard(_aliveJobsLock);
    auto it = _jobCounters.find(job);
    if (it != _jobCounters.end())
    {
        JobResult Result = it->second.Result;
        if (newValue != JOB_RESULT_NONE)
        {
            it->second.Result = newValue;
            
            // Here we have just finished the job and are assigning its result. If needed, signal wait we are finished
            if (it->second.PostIncrement != NULL)
                (*it->second.PostIncrement)++;
            if (it->second.PostResult != NULL && newValue == JOB_RESULT_FAIL)
                *it->second.PostResult = JOB_RESULT_FAIL;
            it->second.PostIncrement = NULL;
            it->second.PostResult = NULL;
        }
        return Result;
    }
    return JOB_RESULT_NONE; // Not found
}

U32 JobScheduler::_GenerateJobCountersUnsafe(U32 num, U32 nRefs)
{
    TTE_ASSERT(num > 0, "Invalid number of IDs to generate in scheduler");
    U32 startID = 1 + _runningID.fetch_add(num); // ID of 0 invalid, so + 1.
    
    // Create job counters and insert into map (using pair). ID => job counter
    for (U32 i = 0; i < num; i++)
        _jobCounters.insert(std::make_pair(startID + i, JobCounter{nRefs, JOB_RESULT_NONE}));
    
    return startID;
}

void JobScheduler::_EnqueueJobs(U32 jobID, JobDescriptor *pJobDescriptors, U32 nJobs, JobHandle *pOutHandles)
{
    _RegisterJobs(nJobs, pJobDescriptors, pOutHandles, jobID); // Just call register, to reduce repeated code.
}

Bool JobScheduler::_DequeueJob(U32 jobID, Job &dest)
{
    // The previous job is finised, so dispurse the rest and leave one to execute on the current thread for maximum throughput.
    U32 numDispursed = 0;
    
    {
        std::unique_lock<std::mutex> ulock(_aliveJobsLock); // Lock as accessing enqueued jobs
        
        auto it = _enqueuedJobs.find(jobID); // Find the associated job list of enqueued jobs
        if (it == _enqueuedJobs.end() || it->second.NumQueued == 0)
            return false; // No enqueued jobs
        
        // We could dispurse them *all* to any thread. We however want to slightly let enqueued ones have an advantage.
        // So we will ensure 1 gets returned. Lets keep half in the enqueued array if theres a larger amount of jobs queued.
        // Why? Say we have 32 queued jobs. We leave 16 in the queue. This function will run again after one of the run jobs
        // from the remaining 16 is finished (and its enqueued jobs if it has any!). We can then put another 8 in the normal
        // pending jobs queue and keep 7 (one just run). This not only will likely cause less memory allocations in the vectors (logarithmic-halfing)
        // but also will importantly mean lots of the jobs get run and dont 1) take over other jobs, 2) they have a slight priority
        // advantage - we know these jobs are hot in the job system and should be run. We could in future use priorities of them etc.
        
        // Lets set the large numer of threshold to the number of threads, which makes logical sense. (16 typically)
        
        // First lets take the one we are going to run by the one with the highest priority in the enqueued list.
        it->second.Dequeue(dest);
        
        Job tmp{};
        if (it->second.NumQueued >= NUM_SCHEDULER_THREADS)
        {
            
            // Keep half and dispurse the other half
            
            std::lock_guard<std::mutex> _Guard2(_jobsLock); // Don't panic! Deadlock cannot occur as this lock is inferior to the previous
            // ie, the alive jobs lock is and should *always* be locked before and unlocked after the jobs lock!
            
            // We have both locks acquired here so we can access both arrays safely.
            
            U32 toDispurseNum = (it->second.NumQueued >> 1) + (it->second.NumQueued & 1); // Half it, w/advantage for remainder.
            numDispursed = toDispurseNum;
            
            while (toDispurseNum--)
            {
                it->second.Dequeue(tmp); // Ignore smart highlight. Moved object gets assigned each dequeue.
                _pendingJobs.push(std::move(tmp));
            }
        }
        else if (it->second.NumQueued > 0)
        {
            
            // We have less, dispurse the rest
            
            JobList toDispurse = std::move(it->second);
            ulock.unlock(); // Finished with alive jobs.
            
            numDispursed = toDispurse.NumQueued;
            
            // Lock pending jobs array and dispurse these
            _jobsLock.lock();
            while (toDispurse.NumQueued)
            {
                toDispurse.Dequeue(tmp); // Ignore smart highlight. Moved object gets assigned each dequeue.
                _pendingJobs.push(std::move(tmp));
            }
            _jobsLock.unlock();
        }
        else
        {
            // Was only one queued job, ignore. Will be freed when decrement refs gets to 0, and we might still enqueue more.
        }
    }
    
    // Signal remaining worker threads (num-1). If number of dispursed < numthreads-1, signal dispursed
    if (numDispursed)
        _SignalJobThread(numDispursed);
    
    // No gauruntee lock is held here, unique lock will unlock if needed
    
    return true;
}

void JobScheduler::_MakeJob(U32 ID, Job &job, JobDescriptor &descriptor, I32 affinityOverride)
{
    TTE_ASSERT(descriptor.AsyncFunction != NULL, "Job descriptor async function is NULL!");
    job.Priority = descriptor.Priority;
    job.RunnableFunction = descriptor.AsyncFunction;
    job.UserArgA = descriptor.UserArgA;
    job.UserArgB = descriptor.UserArgB;
    job.JobID = ID;
    job.AffinityOverride = affinityOverride;
}

void JobScheduler::_RegisterJobs(U32 nJobs, JobDescriptor *pJobDescriptors, JobHandle *pOutHandles, U32 parent /*= (U32)-1*/, I32 affinityOverride)
{
    Bool bHasParent = parent != (U32)-1;
    
    // If we have a parent, we need to do everything in a different order. Enqueue jobs, add counters then done. Only need to use alive jobs lock.
    if (bHasParent)
    {
        
        std::unique_lock<std::mutex> _Guard(_aliveJobsLock); // Accessing enqueued jobs array
        
        // If we are enqueuing them after another, ensure parent exists and then allocate job array
        auto it = _enqueuedJobs.find(parent);
        if (it == _enqueuedJobs.end())
        {
            
            // Two cases. The job is alive and has no previous enqueued jobs (_enqueuedJobs doesn't contain it) or job isn't alive. Check alive, or
            // scheduler has released its ref.
            auto checkAliveIt = _jobCounters.find(parent);
            if (checkAliveIt == _jobCounters.end() || checkAliveIt->second.SchedulerReleased)
            {
                _Guard.unlock();
                return _RegisterJobs(nJobs, pJobDescriptors, pOutHandles, -1, affinityOverride); // Job not found (parent), so is finished! Run normally.
            }
            
            // Add enqueued mapping
            it = _enqueuedJobs.insert(std::make_pair(parent, JobList{})).first;
        }
        
        // Initialise job IDs and output handles. 2 references as we return an array of job handles as well as job system reference.
        U32 newIDs = _GenerateJobCountersUnsafe(nJobs, 2);
        for (U32 handle = 0; handle < nJobs; handle++)
            pOutHandles[handle]._jobID = newIDs + handle;
        
        // All to JobList
        for (U32 i = 0; i < nJobs; i++)
        {
            Job tmp{};
            _MakeJob(newIDs + i, tmp, pJobDescriptors[i], affinityOverride);
            it->second.Append(std::move(tmp));
        }
        
        // Done !
    }
    else
    {
        
        // Firsly initialise all job handles
        U32 newIDs;
        {
            // Generate job IDs, within the lock.
            std::lock_guard<std::mutex> _Guard(_aliveJobsLock);
            newIDs = _GenerateJobCountersUnsafe(nJobs, 2);
        }
        
        // Set output handles.
        for (U32 handle = 0; handle < nJobs; handle++)
            pOutHandles[handle]._jobID = newIDs + handle;
        
        {
            
            std::lock_guard<std::mutex> _Guard(_jobsLock); // Adding now to pending array, so ensure pending jobs lock array is locked.
            
            // Add new jobs to pending jobs array
            for (U32 i = 0; i < nJobs; i++)
            {
                Job localJob{}; // Store locally then push it to priority queue.
                _MakeJob(newIDs + i, localJob, pJobDescriptors[i], affinityOverride);
                _pendingJobs.push(std::move(localJob));
            }
        }
        
        // Signal threads to run.
        _SignalJobThread(nJobs);
    }
}

JobScheduler::JobScheduler(LuaFunctionCollection&& col) : _workerScriptCollection(std::move(col)),
_jobThreadNotifierCount(0), _running(true), _cancelJobs(false), _runningID(0)
{
    // Job thread notifier counter is 0 to begin with, ie threads will wait until jobs come.
    
    // Spawn threads!
    for (U32 threadIndex = 0; threadIndex < NUM_SCHEDULER_THREADS; threadIndex++)
    {
        _workers[threadIndex] = std::thread(_JobThreadFn, std::ref(*this), threadIndex);
    }
}

JobScheduler::JobScheduler() : JobScheduler(LuaFunctionCollection{}) {}

void JobScheduler::_Shutdown(Bool bKillAwaiting)
{
    TTE_ASSERT(_running, "Shutdown called twice! Internal error");
    
    _cancelJobs = bKillAwaiting;
    _running.store(false, std::memory_order_release); // release, store it. cancelJobs by definition is also synchronised.
    
    // Signal all threads, so they finish.
    _SignalJobThread(9999);
    
    // Now we wait! Join all threads
    for (U32 threadIdx = 0; threadIdx < NUM_SCHEDULER_THREADS; threadIdx++)
        _workers[threadIdx].join();
    
    // Finished. If this assert fires, another thread (not the calling (main) or any worker), has posted jobs!
    TTE_ASSERT(bKillAwaiting || _pendingJobs.size() == 0, "Jobs were posted after Shutdown called from external thread");
}

Bool JobScheduler::PostAll(JobDescriptor *pDescriptors, U32 Num, JobHandle *pOutHandles)
{
    _RegisterJobs(Num, pDescriptors, pOutHandles);
    return true;
}

JobHandle JobScheduler::Post(JobDescriptor descriptor, I32 affinityOverride)
{
    JobHandle handle{};
    // Call register job to do all the work! Ignore return, will always be true.
    _RegisterJobs(1, &descriptor, &handle, -1, affinityOverride);
    return handle;
}

void JobScheduler::_CancelRemoveJob(const JobHandle &hJob, Bool cancelQueued)
{
    // Remove counter and enqueued jobs.
    
    JobList toDispuse{};
    {
        std::lock_guard<std::mutex> _Guard(_aliveJobsLock);
        
        _jobCounters.erase(hJob._jobID); // Remove counters
        
        if (cancelQueued)
        {
            // Clear enqueued
            _enqueuedJobs.erase(hJob._jobID);
        }
        else
        {
            // Leave queued, so dispurse them all
            auto it = _enqueuedJobs.find(hJob._jobID);
            if (it != _enqueuedJobs.end())
            {
                toDispuse = std::move(it->second);
                _enqueuedJobs.erase(it);
            }
        }
    }
    
    if (toDispuse.NumQueued)
    { // Save a lock if possible
        
        std::lock_guard<std::mutex> _Guard(_jobsLock);
        
        // Dispurse if we have queued jobs
        Job tmp{};
        while (toDispuse.NumQueued)
        {
            toDispuse.Dequeue(tmp); // Ignore warning under this line
            _pendingJobs.push(std::move(tmp));
        }
    }
}

Bool JobScheduler::Cancel(const JobHandle &hJob, Bool cancelQueued)
{
    // Check the job has not started. If it has return false. If it has, remove it from the pending queue!
    Bool Started = true;
    {
        std::lock_guard<std::mutex> _Guard(_jobsLock);
        
        // Use hacked container (access internal vector) and try and find
        for (auto it = _pendingJobs.get_container().begin(); it != _pendingJobs.get_container().end(); it++)
        {
            if (it->JobID == hJob._jobID)
            {
                _pendingJobs.get_container().erase(it); // below line is needed to ensure priority queue remains ordered.
                std::make_heap(_pendingJobs.get_container().begin(), _pendingJobs.get_container().begin(), _pendingJobs.get_cmp());
                Started = false;
                break;
            }
        }
    }
    if (Started)
    {
        
        // The job may be in an enqueued jobs array
        std::unique_lock<std::mutex> _Guard(_aliveJobsLock); // Lock initially, used so we can manually unlock
        
        for (auto &it : _enqueuedJobs)
        {
            
            if (it.first == hJob._jobID)
            {
                // It has enqueued jobs (rare case), between locks. Remove it and we are done.
                _enqueuedJobs.erase(it.first);
                return false; // Return false however. The job at question still ran, queued jobs won't. Still return false
            }
            
            // Sizeof 1
            if (it.second.NumQueued == 1)
            {
                if (it.second.Singular.JobID == hJob._jobID)
                {
                    // Lucky, found it! Cancel
                    Job _{};
                    it.second.Dequeue(_); // Ignore the job
                    return true;
                }
            }
            else if (it.second.NumQueued > 1)
            {
                
                // Find it in the array of enqueued jobs. If found, cancel it.
                for (auto itB = it.second.Queued.get_container().begin(); itB != it.second.Queued.get_container().end(); itB++)
                {
                    if (itB->JobID == hJob._jobID)
                    {
                        
                        // Erase the job from
                        it.second.Queued.get_container().erase(itB);
                        // below line is needed to ensure priority queue remains ordered.
                        std::make_heap(it.second.Queued.get_container().begin(), it.second.Queued.get_container().begin(),
                                       it.second.Queued.get_cmp());
                        
                        if ((--it.second.NumQueued) == 1)
                        { // If size is now one, move to Singular
                            Job sin = std::move(const_cast<Job &>(it.second.Queued.top()));
                            it.second.Queued.pop();
                            it.second.Queued.~hacked_priority_queue();
                            new (&it.second.Singular) Job(std::move(sin));
                        }
                        
                        // Remove any counters, none will queued.
                        _Guard.unlock();
                        _CancelRemoveJob(hJob, false);
                        
                        // Done!
                        return true;
                    }
                }
            }
        }
        
        // Not found
        return false;
    }
    
    _CancelRemoveJob(hJob, cancelQueued);
    return true;
}

// Helper function used by Scheduler::Wait to lock the mutex and check the result.
void JobScheduler::_PerformWaitCheck(Bool bFirstIt, U32 jobID, JobResult &outResult)
{
    std::lock_guard<std::mutex> _Guard(_aliveJobsLock); // Lock counters array
    
    auto it = _jobCounters.find(jobID);
    if (it == _jobCounters.end())
    {
        TTE_ASSERT(bFirstIt,
                   "Internal error: job could not be found!"); // Should never happen! Job removed from counters array, but we have a handle!
        outResult = JOB_RESULT_NONE; // Job doesn't exist (bFirstIt == true), so just return that - assertion won't have fired.
        return;
    }
    
    TTE_ASSERT(it->second.Result != JOB_RESULT_RUNNING, "Internal error: invalid job result"); // Result should never be running!
    TTE_ASSERT(it->second.PostIncrement == 0, "Cannot call Wait() multiple times on a job handle from different threads");
    
    outResult = it->second.Result == JOB_RESULT_NONE ? JOB_RESULT_RUNNING : it->second.Result;
}

JobResult JobScheduler::Wait(const JobHandle &hJob)
{
    // In the future we could make a Proxy thread class and have this calling function act as a job thread and run jobs while waiting !
    
    JobResult Result = JOB_RESULT_RUNNING;
    _PerformWaitCheck(true, hJob._jobID, Result); // Initial check
    
    U32 Alternator = 1; // We use this so we pass in either 0 or WAIT_INTERNVAL_MS (alternates) to ThreadSleep.
    // Zero will just yield this thread on most platforms.
    
    while (Result == JOB_RESULT_RUNNING)
    {
        
        ThreadSleep(WAIT_INTERNVAL_MS * Alternator); // Wait initially, and then check again. This can be changed!
        
        _PerformWaitCheck(false, hJob._jobID, Result); // Perform the check
        
        Alternator = (Alternator + 1) & 1; // Alternate between 0 and 1
    }
    
    return Result;
}

JobResult JobScheduler::Wait(U32 N, const JobHandle* pJobHandles)
{
    // Sanity checks
    if (N == 0)
        return JOB_RESULT_OK; // No jobs
    else if (N == 1)
        return Wait(pJobHandles[0]); // Wait normally on the single job
    
    // Go through the array and take all valid (active jobs) under the lock. Set the counter post increment (check none are set already). Wait.
    U32 LocalWaiter = 0; // This is going to be incremented but multiple threads, UNDER the aliveJobsLock. So only access within that lock.
    U32 NumWaitingOn = 0;
    JobResult Result = JOB_RESULT_OK;
    {
        std::lock_guard<std::mutex> _Guard(_aliveJobsLock); // Lock counters array
        
        for (U32 i = 0; i < N; i++)
        {
            const JobHandle& handle = pJobHandles[i];
            
            auto it = _jobCounters.find(handle._jobID);
            if (it != _jobCounters.end())
            { // Check if valid (eg wait on cancelled job, which should succeed and ignore)
                
                // First assert
                TTE_ASSERT(it->second.PostIncrement == 0, "Wait() called multiple times on a job handle from different threads, or duplicates!");
                
                // Assign PostIncrement to local variable which will get incremented when the jobs finish.
                it->second.PostIncrement = &LocalWaiter;
                it->second.PostResult = &Result;
                if (it->second.Result == JOB_RESULT_RUNNING || it->second.Result == JOB_RESULT_NONE)
                {
                    NumWaitingOn++;
                }
            }
        }
    }
    
    // If all are finished, we are done
    if (NumWaitingOn == 0)
        return JOB_RESULT_OK;
    
    // Now wait until LocalWaiter == NumWaitingOn. Use the alternator approach like normal Wait.
    U32 Alternator = 1;
    for (;;)
    {
        
        // First check if we are done
        {
            std::lock_guard<std::mutex> _Guard(_aliveJobsLock);
            if (LocalWaiter == NumWaitingOn)
                break; // Done !
        }
        
        // Wait and alternate, see normal Wait above.
        
        ThreadSleep(WAIT_INTERNVAL_MS * Alternator);
        Alternator = (Alternator + 1) & 1; // Alternate between 0 and 1
    }
    
    return Result;
}

JobHandle JobScheduler::EnqueueOne(const JobHandle &hJob, JobDescriptor descriptor)
{
    // Just need to call the internal one with an array size of 1. If it fails, handle is invalid anyway
    JobHandle handle{};
    _EnqueueJobs(hJob._jobID, &descriptor, 1, &handle);
    return handle;
}

std::vector<JobHandle> JobScheduler::EnqueueAll(const JobHandle &hJob, const std::vector<JobDescriptor> &jobs)
{
    // Handles array. Populate it first as pointer array is passed in.
    std::vector<JobHandle> handles{};
    handles.reserve(jobs.size());
    for (auto i = 0; i < jobs.size(); i++)
        handles.push_back(JobHandle{});
    
    _EnqueueJobs(hJob._jobID, (JobDescriptor *)jobs.data(), (U32)jobs.size(), handles.data());
    
    return handles; // Moved
}

JobResult JobScheduler::GetResult(const JobHandle &hJob)
{
    JobResult Result = JOB_RESULT_NONE;
    {
        std::lock_guard<std::mutex> _Guard(_aliveJobsLock); // Lock counters array
        auto jobCounter = _jobCounters.find(hJob._jobID);   // Find counter and check if valid
        if (jobCounter == _jobCounters.end())
            return JOB_RESULT_NONE;
        // If the job is still running, ie result is NONE, return running.
        if (jobCounter->second.Result == JOB_RESULT_NONE)
            return JOB_RESULT_RUNNING;
        Result = jobCounter->second.Result;
    }
    // Assert that the result is either OK or FAIL
    TTE_ASSERT(Result == JOB_RESULT_OK || Result == JOB_RESULT_FAIL, "Job result must be OK or FAIL");
    return Result;
}

void JobList::Append(Job &&job)
{
    // Switch to an array: destroy previous singular job and move that and new job into array
    if (NumQueued == 1)
    {
        Job firstJob = std::move(Singular);
        Singular.~Job();
        new (&Queued) std::vector<Job>();
        Queued.push(std::move(firstJob));
    }
    else if (NumQueued == 0)
    {
        // Assign local
        NumQueued = 1;
        Singular = std::move(job);
        return;
    }
    // Push new job
    Queued.push(std::move(job));
    NumQueued++;
}

void JobList::Dequeue(Job &destJob)
{
    if (NumQueued >= 2)
    {
        destJob = std::move(const_cast<Job &>(Queued.top()));
        Queued.pop();
        if (--NumQueued == 1)
        {
            // Sizeof array is now 1, change to storing locally. Change array type from vector to singular job.
            Job singularJob = std::move(const_cast<Job &>(Queued.top()));
            Queued.~hacked_priority_queue();
            new (&Singular) Job();
            Singular = std::move(singularJob);
        }
    }
    else if (NumQueued == 1)
    {
        NumQueued = 0;
        destJob = std::move(Singular);
    } // No jobs to dequeue
}
