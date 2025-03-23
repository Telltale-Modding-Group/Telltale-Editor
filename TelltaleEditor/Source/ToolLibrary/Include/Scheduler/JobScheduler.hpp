#pragma once

#include <Core/Config.hpp>

#include <Scripting/ScriptManager.hpp>

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// Number of worker threads
#define NUM_SCHEDULER_THREADS 8
// Interval for Wait functions in MS
#define WAIT_INTERNVAL_MS 30

/// <summary>
///
/// This JobScheduler is what a lot of the Meta system builds upon.
/// A Job is posted in the Scheduler and this job can be waited on and queried. It is completed asyncronously
/// on one of the worker threads backend. Jobs can be chained in the scheduler, such that when one finished the next
/// one is executed directly after. When chaining jobs its possible such that when one job finished, instead *multiple*
/// jobs are then posted. This is very useful for dispatching lots of jobs to be executed in parallel.
///
/// This system provides a very simple and effective way to execute lots of code quickly and safely across all the cores (ideally).
///
/// Post() posts a new job descriptor such that it will be run in the future ASAP on a worker thread.
/// Wait() waits for a job to complete.
///
/// </summary>
class JobScheduler;

struct JobThread;

// Job Priorities
enum JobPriority
{
    JOB_PRIORITY_NORMAL = 0,
    JOB_PRIORITY_HIGH = 1,
    JOB_PRIORITY_VERY_HIGH = 2,
    JOB_PRIORITY_HIGHEST = 3,
    JOB_PRIORITY_COUNT = 4,
};

// See JobScheduler::_WaitJobThread.
enum JobThreadWaitResult
{
    JOB_THREAD_RESULT_OK,
    JOB_THREAD_RESULT_CANCEL_EXIT, // Exit and cancel and ignore any due jobs.
};

// Job Results
enum JobResult : U32
{
    // No result. Used for unchained-jobs, where there was no previous job.
    JOB_RESULT_NONE = (U32)-1,
    // This job completed with no errors
    JOB_RESULT_OK = 0,
    // This job failed to execute.
    JOB_RESULT_FAIL = 1,
    // The job was cancelled due to shutdown.
    JOB_RESULT_CANCELLED = 2,
    // The job is still running
    JOB_RESULT_RUNNING = 3,
};

/// <summary>
/// Job Function prototype, see Job class. Job functions to be run must have this signature.
/// The last two arguments are passed in the descriptor to the job.
/// Return if the job ran successfully, determines what GetResult returns.
/// </summary>
using JobFunction = Bool (*)(const JobThread &thread, void *pUserArgA, void *pUserArgB);

/// <summary>
/// A Job. This is used internally and represents a function and two user arguments which can be passed into it, when its run.
/// These have associated priorities, such that higher priority jobs are executed before lower ones.
/// </summary>
struct Job
{
    
    JobFunction RunnableFunction;
    void *UserArgA, *UserArgB;
    JobPriority Priority;
    U32 JobID;
    
    // Default Constructor
    inline Job() : RunnableFunction(0), UserArgA(0), UserArgB(0), Priority(JobPriority::JOB_PRIORITY_NORMAL), JobID(0) {}
    
    // Comparison operator for the priority queue.
    inline bool operator<(const Job &rhs) const { return Priority < rhs.Priority; }
    
    // Jobs should not be copied, only moved.
    Job(const Job &) = delete;
    Job &operator=(const Job &) = delete;
    
    // Jobs are moveable only.
    Job(Job &&) = default;
    Job &operator=(Job &&) = default;
};

/// <summary>
/// A JobHandle is a lightweight object which gives access to a job. Functions in the scheduler give access to it.
/// When none of these objects refer to a job anymore, the job will still run however the result will not be able to
/// be queried - but this is OK as you should refer to jobs using this and not their internal IDs.
/// </summary>
struct JobHandle
{
    
    friend class JobScheduler;
    
    // Default constructor, with ID of 0 denoting no valid ID.
    inline JobHandle() : _jobID(0) {}
    
    // Bool operator: if jobHandle => execute code if this is a valid handle to a job.
    inline operator bool() const { return _jobID != 0; }
    
    inline Bool IsValid() const { return _jobID != 0; }
    
    // Comparison operator
    inline bool operator==(const JobHandle &rhs) const { return _jobID == rhs._jobID; }
    
    inline ~JobHandle(); // Defined at end of file
    
    inline JobHandle &operator=(const JobHandle &rhs); // Defined at end of file
    
    inline JobHandle(const JobHandle &rhs) { *this = rhs; }
    
    // Simple moves
    inline JobHandle(JobHandle &&rhs) noexcept
    {
        _jobID = rhs._jobID;
        rhs._jobID = 0;
    }
    
    inline JobHandle &operator=(JobHandle &&rhs) noexcept
    {
        _jobID = rhs._jobID;
        rhs._jobID = 0;
        return *this;
    }
    
    // empty the handle
    inline void Reset()
    {
        _jobID = 0;
    }
    
private:
    // Constructor for internal use.
    inline JobHandle(U32 id) : _jobID(id) {}
    
    U32 _jobID; // Job ID
};

/// <summary>
/// This is used by the user to describe a job to be posted to the job system.
/// Set the values here to be posted to the job system.
/// </summary>
struct JobDescriptor
{
    
    JobFunction AsyncFunction = 0;
    JobPriority Priority = JOB_PRIORITY_NORMAL;
    void *UserArgA = 0, *UserArgB = 0;
};

// Helper function to make a job descriptor.
inline JobDescriptor MakeJob(JobFunction Fn, void *ArgA, void *ArgB, JobPriority P = JOB_PRIORITY_NORMAL) { return JobDescriptor{Fn, P, ArgA, ArgB}; }

/// <summary>
/// JobThread represents a single thread which is able to run jobs. This is stored on stack for each worker thread.
/// </summary>
struct JobThread
{
    
    // JobThread name
    String ThreadName;
    // Thread number
    U32 ThreadNumber;
    // Current job being executed.
    Job CurrentJob;
    
    // Lua state for this thread.
    mutable LuaManager L;
    
};

/// <summary>
/// Internally used to count the number of references (JobHandles) to a Job, such that when there are no more
/// the job result information is freed from memory.
/// </summary>
struct JobCounter
{
    
    U32 Refs = 1; // Number of references. One as default the job scheduler owns a reference - once job has run it decrements that.
    JobResult Result = JOB_RESULT_NONE; // Result of the job
    U32 *PostIncrement = NULL;          // Used in Wait(multiple jobs). Internally used to count how many jobs left to finish.
    JobResult *PostResult = NULL;       // If non-null, when job finishes if it failed then this is assigned to FAIL such that Wait knows the result.
    Bool SchedulerReleased =
    false; // When the Refs is decremented internally, this gets set to true - meaning the job is only tracked by external JobHandles
};

/// <summary>
/// Used internally. When jobs are queued this struct acts as a small vector class (See folly/Boost) as a commonly only 1 job will
/// be queued after a job, so an vector will have a lot of overhead in comparison. Adds minimal complexity
/// </summary>
struct JobList
{
    
    union
    {
        hacked_priority_queue<Job> Queued; // Array of jobs queued
        Job Singular;                      // Singular queued job.
    };
    
    U32 NumQueued; // Number of queued jobs
    
    // Initialises to size of 1.
    inline JobList(Job &&queued) : Singular(std::move(queued)), NumQueued(1) {}
    
    // Initialises to size of 0.
    inline JobList() : Singular(), NumQueued(0) {}
    
    // Appends a job to the list.
    void Append(Job &&job);
    
    // Dequeue one. Size must be >= 1
    void Dequeue(Job &destJob);
    
    // On Destruct, call correct destructor in Union which C++ won't do automatically.
    inline ~JobList()
    {
        if (NumQueued <= 1)
            Singular.~Job();
        else
            Queued.~hacked_priority_queue();
    }
    
    // JobLists should not be copied, only moved.
    JobList(const JobList &) = delete;
    JobList &operator=(const JobList &) = delete;
    
    // JobLists are moveable only.
    inline JobList(JobList &&rhs) : JobList() { *this = std::move(rhs); }
    
    // Must be explicity defined (see c++ spec) as destructor is defined. Just moves, and doesn't copy.
    inline JobList &operator=(JobList &&rhs) noexcept
    {
        
        // Destroy self first
        if (NumQueued <= 1)
            Singular.~Job();
        else
            Queued.~hacked_priority_queue();
        
        // Move RHS into self
        NumQueued = rhs.NumQueued;
        rhs.NumQueued = 0;
        if (NumQueued <= 1)
            Singular = std::move(rhs.Singular);
        else
            Queued = std::move(rhs.Queued);
        return *this;
    }
};

/// <summary>
/// JobScheduler. This is very similar to Telltale Games' JobScheduler.
/// Written by Lucas
/// </summary>
class JobScheduler
{
public:
    
    /// <summary>
    /// Constructor to initialise the system and spawn threads.
    /// This version takes in a set of registration functions for each worker thread lua state.
    /// </summary>
    JobScheduler(LuaFunctionCollection&& jobThreadFunctions);
    
    /// <summary>
    /// Constructor to initialise the system and spawn threads.
    /// </summary>
    JobScheduler();
    
    /// <summary>
    /// Posts a job to the job scheduler. The job will be run eventually and the return value can be used with other functions in this
    /// class.
    /// </summary>
    JobHandle Post(JobDescriptor descriptor);
    
    /// <summary>
    /// See Post(). This version an array of them, with no ordering constraints on their execution. If return true, the out handles array will contain Num waitable handles.
    /// </summary>
    Bool PostAll(JobDescriptor* pDescriptors, U32 Num, JobHandle* pOutHandles);
    
    /// <summary>
    /// Attempts to cancel the given job, if it hasn't started yet. If it has or the job handed was invalid, false is returned.
    /// Returns true if it is cancelled. If you don't want to cancel queued jobs (ie ones with this as its Parent), set the second argument
    /// to false. This will have cancelled passed to the jobs as the previous job result.
    /// </summary>
    Bool Cancel(const JobHandle &, Bool cancelQueued);
    
    /// <summary>
    /// Attempts to make this thread wait for the given job to finish. Returns the result of the job.
    /// Returns RESULT_NONE if the job has already finished (ie invalid).
    /// Do not call this multiple times on different threads. Only call once or multiple times on same thread.
    /// </summary>
    JobResult Wait(const JobHandle &job);
    
    /// <summary>
    /// Waits for all the handles to finish. Returns OK if all the jobs succeed OR had already finished, FAIL if any of them failed.
    /// You cannot wait on any of the same jobs from multiple threads! I.e only call once for each set of job handles, otherwise an assert will be
    /// fired. Note that there should NOT be any duplicate handles! Otherwise an assert will be thrown.
    /// </summary>
    JobResult Wait(U32 N, const JobHandle* jobHandles);
    
    /// <summary>
    /// Enqueue a job such that it is run when the parent job finishes. If the parent job does not exist (ie finished)
    /// then its run normally. If the parent job is not valid (IsValid is false), this returns an invalid handle.
    /// Returns the handle of the queued job, or an invalid job this function fails.
    /// </summary>
    JobHandle EnqueueOne(const JobHandle &job, JobDescriptor descriptor);
    
    /// <summary>
    /// Enqueue an array of jobs to be executed after the parent one finishes. See EnqueueOne for functionality and return value.
    /// Returns an array of handles for each enqueued job. If the array is empty, this function failed.
    /// </summary>
    std::vector<JobHandle> EnqueueAll(const JobHandle &job, const std::vector<JobDescriptor> &jobs);
    
    /// <summary>
    /// Gets the result of the given job. Returns RESULT_NONE if the job is invalid or hasn't finished yet. RESULT_RUNNING if running or is scheduled to run.
    /// </summary>
    JobResult GetResult(const JobHandle &);
    
    /// <summary>
    /// Shutdown all jobs and cancel any future jobs.
    /// </summary>
    inline ~JobScheduler() { _Shutdown(true); }
    
    /// Returns true if the callee is running from a worker thread
    static Bool IsRunningFromWorker();
    
    /// Only call if IsRunningFromWorker() returned true. Returns the job thread information.
    static JobThread& GetCurrentThread();
    
    // Singleton Init/Shutdown.
    
    /**
     Initialises the job scheduler with the given function collection to register to each thread local lua manage (state)
     */
    static void Initialise(LuaFunctionCollection Col = {});
    
    /**
     Shutsdown the job scheduler
     */
    static void Shutdown();
    
    // We use the heap instead of locally in a TU such that we can control initialisation.
    static JobScheduler *Instance;
    
private:
    friend struct JobHandle; // Counter Increment/Decrement
    friend struct JobThread; // Job threads are closely tied to this.
    
    static void _JobThreadFn(JobScheduler &scheduler, U32 threadIndex);
    
    /// <summary>
    /// Internal: runs the current job on the given thread and any enqueued jobs on that thread.
    /// Returns if we need to exit because we are shutting down.
    /// </summary>
    static JobThreadWaitResult _JobThreadRunJob(JobScheduler &scheduler, JobThread &thread);
    
    /// <summary>
    /// Shuts down the job system (called in destructor with true). If the argument is true, any pending or future jobs are cancelled.
    /// </summary>
    void _Shutdown(Bool bKillAwaiting);
    
    /// <summary>
    /// Internal: Signal a JobThread that a job has been posted and it should wake up
    /// </summary>
    void _SignalJobThread(U32 numSignals = 1);
    
    /// <summary>
    /// Internal: Wait a job thread until a signal is posted if argument is true. Returns the wait result, ie if we are still running or should exit.
    /// The argument is false, essentially we are just checking if we need to keep running cancelled jobs on exit, and
    /// the CurrentJob in the thread is not touched. Will only return OK or CANCEL_EXIT.
    /// If CurrentJob inside the thread has a NULL runnable function, we need to loop again.
    /// </summary>
    JobThreadWaitResult _WaitJobThread(Bool bWaitSemaphore, JobThread &myself);
    
    /// <summary>
    /// Internal: Increment the number of references to the Job (ie JobHandle is copied)
    /// </summary>
    void _IncrementRefs(U32 jobID);
    
    /// <summary>
    /// Internal: Decrement the number of references to the Job. If its now zero, the internal counter is freed. The second argument is true if we are
    /// releasing the internal scheduler reference to the job. See JobCounter.
    /// </summary>
    void _DecrementRefs(U32 jobID, Bool bReleaseSched);
    
    /// <summary>
    /// Internal: Internally used to get and set job result from the counter object for a given job handle. If called with
    /// only one argument, or you pass in result none, only a get operation (similar to C# get). Else return old value and assign new.
    /// </summary>
    JobResult _GetSetCounter(U32 jobRef, JobResult newValue = JOB_RESULT_NONE);
    
    /// <summary>
    /// Internal: Generates a Job ID internally. The range of IDs generated is just the return value + 1 for each numID, eg calling
    /// with argument of 2 and the function returns lets say 5: this corresponds to 5 and 6 being newly generated IDs.
    /// Set initial refs to the number of references to initialise. If no job handle is created from this, use 1 (job system only)
    /// else use 2 for job system and the returning handle you use (for each handle this number will be set).
    ///
    /// This ASSUMES that the _aliveJobsLock is held (hence unsafe), as this creates the job counters.
    /// </summary>
    U32 _GenerateJobCountersUnsafe(U32 numIDs, U32 initialRefs);
    
    /// <summary>
    /// Internally enqueues the job descriptor array to execute after the job handle. The output handle array is
    /// populated with the new job handles.
    /// </summary>
    void _EnqueueJobs(U32 jobID, JobDescriptor *pJobDescriptors, U32 nJobs, JobHandle *pOutHandles);
    
    /// <summary>
    /// Internal: Dequeues a job for the given job ID that is pending to run. Returns if any were dequeued.
    /// Calling this function assumes the previous job (ie job jobID) has finished. This may dispuse jobs onto other cores which are queued.
    /// </summary>
    Bool _DequeueJob(U32 jobID, Job &);
    
    /// <summary>
    /// Internal: Registers (adds to arrays) some number of jobs and initialises the handles array passed in. If parent is
    /// included in this call, then these jobs are instead added to the enqueued array for the given job handle.
    /// If the parent job is not found (ie finished), these are executed ASAP like normal jobs - assuming the previous already finished between
    /// it being posted and you queuing the jobs.
    /// </summary>
    void _RegisterJobs(U32 nJobs, JobDescriptor *pJobDescriptors, JobHandle *pOutHandles, U32 parent = (U32)-1);
    
    /// <summary>
    /// Makes a job from a given descriptor. Pass in the newly allocated job ID
    /// </summary>
    void _MakeJob(U32 jobID, Job &job, JobDescriptor &descriptor);
    
    /// <summary>
    /// Interally used by Wait. Checks the return value of the job (result). First argument is used for assertions.
    /// </summary>
    void _PerformWaitCheck(Bool bFirstIt, U32 jobID, JobResult &outResult);
    
    /// <summary>
    /// Used internally by Cancel to reduce duplicated code
    /// </summary>
    void _CancelRemoveJob(const JobHandle &hJob, Bool cancelQueued);
    
    hacked_priority_queue<Job> _pendingJobs;
    std::mutex _jobsLock; // Jobs array protector.
    
    // These 3 variables create a semaphore used to signal job threads.
    std::mutex _jobThreadNotifierLock;              // Mutex. Takes priority over all other mutexes in this class
    std::condition_variable _jobThreadNotifierCond; // Allows thread notification and waiting
    U32 _jobThreadNotifierCount;                    // Semaphore counter. Number of PENDING jobs - matched to pending jobs (not enqueued!!)
    
    std::atomic<Bool> _running; // true if we are currently running, false otherwise. cancelJobs is protected by this atomic by acq/rel, order matters
    // (this one first)
    Bool _cancelJobs; // if running is false and this is true, then cancel any jobs and exit quick asap. if running false and this false, finish job
    // queues.
    
    std::atomic<U32> _runningID; // Running ID generator for jobs.
    
    std::map<U32, JobCounter> _jobCounters; // Map of Job ID => Patches between job handle objects and internal system, as well as ref counting.
    std::map<U32, JobList> _enqueuedJobs;   // Map of Job ID => enqueued jobs to run.
    
    std::mutex _aliveJobsLock; // Protects counters and enqueued jobs.
    
    std::thread _workers[NUM_SCHEDULER_THREADS];
    
    LuaFunctionCollection _workerScriptCollection; // Worker threads use this at initialisation
    
};

// Implementations for JobHandle structs, they depend on the scheduler class and these should defintely be inlined!

JobHandle::~JobHandle()
{
    if (IsValid() && JobScheduler::Instance)                   // Ensure scheduler is alive. If not, no need to decrement
        JobScheduler::Instance->_DecrementRefs(_jobID, false); // Handle is finished with, decrement number of references
}

JobHandle &JobHandle::operator=(const JobHandle &rhs)
{
    _jobID = rhs._jobID;
    if (IsValid() && JobScheduler::Instance)            // Ensure scheduler is alive. If not, no need to increment.
        JobScheduler::Instance->_IncrementRefs(_jobID); // We have copied the handle, increment number of references
    return *this;
}
