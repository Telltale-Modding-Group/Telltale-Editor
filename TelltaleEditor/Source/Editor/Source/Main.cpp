#include <Core/Config.hpp>
#include <Core/Thread.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <Scripting/LuaManager.hpp>

// ============================ TEMPORARY STUFF ============================

std::atomic<U32> c{}; // At the last call, this should be 1001 (1 parent, 1000 enqueued). the order of enqueued jobs executed is random.

// test job function. takes in running thread, previous job tesult
Bool TestJobFn(const JobThread &thrd, void *pUserArgA, void *pUserArgB)
{
    ThreadSleep(1);                                                       // Sleep to dispurse thread work, assume this is a heavy function!
    printf("Hello from %s! Counter: %d\n", thrd.ThreadName.c_str(), ++c); // printf is thread safe
    return true; // If were to return false from any of these jobs, we would print 'whoops' instead of succeed.
}

// Test to show job system working.
void Test()
{
    JobScheduler::Initialise();

    JobHandle parentJob = JobScheduler::Instance->Post(MakeJob(&TestJobFn, 0, 0)); // post normal job which the rest will be executed after

    // for(U32 i = 0; i < 10; i++) // stress test. note this is bad, see below.
    //     h = JobScheduler::Instance->EnqueueOne(h, MakeJob(&TestJobFn, (void*)i, 0)); // enqueue job. this could be done on this thread easily.

    // the way above assumes all jobs depend on the previous one, ie they need to be in that order (so pointless using jobs). exactly from i to 1000.
    // if we only care about order after the first job, we can use EnqueueAll. Lets run these after the previous ones finish. Watch speed diff.

    std::vector<JobDescriptor> descriptors{};
    for (U32 i = 0; i < 1000; i++) // Push 1000 jobs on 8 workers, each takes ~1m (sleep99% of it) so total time around 1000/8 * 1 ms
        descriptors.push_back(MakeJob(&TestJobFn, 0, 0));

    auto Queued = JobScheduler::Instance->EnqueueAll(parentJob, descriptors); // In one call, enqueue them all.

    // Lets cancel one of the enqueued jobs such that we print 1000 instead of 1001
    JobScheduler::Instance->Cancel(Queued[69], false);

    JobResult Result =
        JobScheduler::Instance->Wait(Queued); // wait for array of handles to all finish. Set breakpoint to ensure they ran successfully
    if (Result == JOB_RESULT_OK)
        printf("all succeeded!\n");
    else
        printf("Whoops: %d\n", Result);

    JobScheduler::Shutdown();
}

// ========================================================================

void TestLua() {
    LuaManager Man[3]; // test init each version
    Man[0].Initialise(LuaVersion::LUA_5_2_3);
    Man[1].Initialise(LuaVersion::LUA_5_1_4);
    Man[2].Initialise(LuaVersion::LUA_5_0_2);
    
    std::string TestCode = "local aVar = 100\nprint(\"Did you see me\", aVar)";
    for(int i = 0; i < 3; i++){
        Man[i].RunText(TestCode.c_str(), (U32)TestCode.length());
    }
}

int main()
{

    // Arguments printed to console will always be in order 0 then 1. check! ensures order constraints of code.
    //Test();
    TestLua();

    TTE_LOG("Running TTE Version %s", TTE_VERSION);
    return 0;
}
