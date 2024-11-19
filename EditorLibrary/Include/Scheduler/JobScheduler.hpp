#include "Config.hpp"

#include <atomic>
#include <vector>
#include <queue>
#include <mutex>

// Number of threads
#define NUM_SCHEDULER_THREADS 8

/// <summary>
/// This JobScheduler is what a lot of the Meta system builds upon.
/// A Job is posted in the Scheduler and this job can be waited on and queried. It is completed asyncronously
/// on one of the worker threads backend. Jobs can be chained in the scheduler, such that when one finished the next
/// one is executed directly after. When chaining jobs its possible such that when one job finished, instead *multiple*
/// jobs are then posted. This is very useful for dispatching lots of jobs to be executed in parallel.
/// 
///	This system provides a very simple and effective way to execute lots of code quickly and safely across all the cores (ideally).
/// 
/// </summary>
class JobScheduler;
struct JobThread;

// Job Priorities
enum JobPriority {
	JOB_PRIORITY_NORMAL = 0,
	JOB_PRIORITY_HIGH = 1,
	JOB_PRIORITY_VERY_HIGH = 2,
	JOB_PRIORITY_HIGHEST = 3,
	JOB_PRIORITY_COUNT = 4,
};

// Job Results
enum JobResult : U32 {
	// No result. Used for unchained-jobs, where there was no previous job.
	JOB_RESULT_NONE = (U32)-1,
	// This job completed with no errors
	JOB_RESULT_OK = 0,
	// This job failed to execute.
	JOB_RESULT_FAIL = 1,
	// The job was cancelled due to shutdown.
	JOB_RESULT_CANCELLED = 2,
	// The job wait timed-out
	JOB_RESULT_TIMEOUT = 3,
};

/// <summary>
/// Job Function prototype, see Job class. Job functions to be run must have this signature.
/// First argument is the job thread we are running on (thread safe), second is the previous job result.
/// The last two arguments are passed in the descriptor to the job.
/// </summary>
using JobFunction = JobResult(*)(JobThread&, JobResult, void* pUserArgA, void* pUserArgB);

/// <summary>
/// A Job. This is used internally and represents a function and two user arguments which can be passed into it, when its run.
/// These have associated priorities, such that higher priority jobs are executed before lower ones.
/// </summary>
struct Job {
	
	JobFunction RunnableFunction;
	void* UserArgA, * UserArgB;
	JobPriority Priority;
	U32 JobID;

	// Default Constructor
	inline Job() : RunnableFunction(0), UserArgA(0), UserArgB(0), Priority(JobPriority::JOB_PRIORITY_NORMAL) {}

	// Jobs should not be copied, only moved.
	Job(const Job&) = delete;
	Job& operator=(const Job&) = delete;

	// Jobs are moveable only.
	Job(Job&&) = default;
	Job& operator=(Job&&) = default;

};

/// <summary>
/// A JobHandle is a lightweight object which gives access to a job. Functions in the scheduler give access to it.
/// When none of these objects refer to a job anymore, the job will still run however the result will not be able to
/// be queried - but this is OK as you should refer to jobs using this and not their internal IDs.
/// </summary>
struct JobHandle {

	friend class JobScheduler;

	// Default constructor, with ID of 0 denoting no valid ID.
	inline JobHandle() : _jobID(0) {}

	// Bool operator: if jobHandle => execute code if this is a valid handle to a job.
	inline operator bool() {
		return _jobID != 0;
	}

	// TODO move/copy ctor and assigns. update refs. and dtor decr.

private:
	U32 _jobID;
};

/// <summary>
/// This is used by the user to describe a job to be posted to the job system.
/// Set the values here to be posted to the job system.
/// </summary>
struct JobDescriptor {

	JobFunction AsyncFunction = 0;
	JobPriority Priority = JOB_PRIORITY_NORMAL;
	void* UserArgA = 0, * UserArgB = 0;

};

/// <summary>
/// JobThread represents a single thread which is able to run jobs.
/// </summary>
struct JobThread {

	//JobThread name
	String ThreadName;
	//Thread number
	U32 ThreadNumber;
	//Thread safe alive guard. If set to False (99% of the time is True), this job thread will exit when no more jobs are available.
	std::atomic_bool AliveGuard;
	//Strong force exit guard. If set to True, after current job finished thread will exit (ie user wants to cancel jobs).
	std::atomic_bool ForceExit;

};

/// <summary>
/// Internally used to count the number of references (JobHandles) to a job, such that when there are no more
/// the job result information is freed from memory.
/// </summary>
struct JobCounter {

	U32 JobID = 0;
	U32 JobRefs = 0;

};

/// <summary>
/// JobScheduler. 
/// </summary>
class JobScheduler {
public:

	/// <summary>
	/// Constructor to initialise the system and spawn threads.
	/// </summary>
	JobScheduler();

	/// <summary>
	/// Shuts down the job system (called in destructor with true). If the argument is true, any pending or future jobs are cancelled.
	/// </summary>
	void Shutdown(Bool bKillAwaiting);

	/// <summary>
	/// Posts a job to the job scheduler. The job will be run eventually and the return value can be used with other functions in this
	/// class.
	/// </summary>
	JobHandle Post(JobDescriptor descriptor);

	/// <summary>
	/// Attempts to cancel the given job, if it hasn't started yet. If it has or the job handed was invalid, false is returned.
	/// Returns true if it is cancelled. If you don't want to cancel queued jobs (ie ones with this as its Parent), set the second argument
	/// to false. This will have cancelled passed to the jobs as the previous job result.
	/// </summary>
	Bool Cancel(const JobHandle&, Bool cancelQueued = true);

	/// <summary>
	/// Attempts to make this thread wait for the given job to finish. Returns the result of the job.
	/// Returns RESULT_NONE if the job has already finished (ie invalid). 
	/// Pass a timeout, in which if longer than that passes false is RESULT_TIMEOUT. Default is infinite.
	/// </summary>
	JobResult Wait(const JobHandle&, U32 timeoutMillis = (U32)-1);

	/// <summary>
	/// Enqueue a job such that it is run when the parent job finishes. If Cancel is called with cancel queued being true,
	/// this job will not be run. If it is false then this will run and with the previous job result as cancelled.
	/// Returns the handle of the queued job, or an invalid job this function fails.
	/// </summary>
	JobHandle EnqueueOne(const JobHandle&, JobDescriptor descriptor);

	/// <summary>
	/// Enqueue an array of jobs to be executed after the parent one finishes. See EnqueueOne for functionality and return value.
	/// Returns an array of handles for each enqueued job. If the array is empty, this function failed.
	/// </summary>
	std::vector<JobHandle> EnqueueAll(const JobHandle&, const std::vector<JobDescriptor>& jobs);

	/// <summary>
	/// Gets the result of the given job. Returns RESULT_NONE if the job is invalid or is still running or hasn't finished yet.
	/// </summary>
	JobResult GetResult(const JobHandle&);

	/// <summary>
	/// Shutdown all jobs and cancel any future jobs.
	/// </summary>
	inline ~JobScheduler() {
		Shutdown(true);
	}

private:

	static void _JobThreadFn(JobScheduler& scheduler, U32 threadIndex);

	JobThread _threads[NUM_SCHEDULER_THREADS];// thread array

	std::priority_queue<Job> _pendingJobs;
	std::mutex _jobsLock; // Jobs array protector.

	Bool _running; // true if we are currently running, false otherwise
	std::atomic<U32> _runningID; // Running ID generator for jobs.

	JobCounter* _counters; // Job counters allocated at initialisation.
	std::vector<Bool> _availCounters; // Available counter indices (a bitset), indexes into _counters.
	std::mutex _countersLock; // Protects avail counters and counters.

};