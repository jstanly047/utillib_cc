
#include <thread/JobManager.h>
#include <atomic>
#include <gtest/gtest.h>
#include <chrono>

struct Flags
{
  std::atomic<bool> lingerAtWork{true};
  std::atomic<bool> started{false};
  std::atomic<bool> finished{false};
  std::atomic<bool> wasCanceled{false};
};

class DummyJob : public Job
{
  Flags* m_flags;
public:
  inline DummyJob(Flags* flags) : m_flags(flags)
  {
  }

  bool doWork() override
  {
    m_flags->started = true;
    while (m_flags->lingerAtWork)
      std::this_thread::yield();

    if (shouldCancel(0,0))
      m_flags->wasCanceled = true;

    m_flags->finished = true;
    return true;
  }

  const std::string getType() const override final
  {
    return "";
  }

  bool operator==(const Job *job) const override final
  {
    return this == job;
  }
};

class ReallyDumbJob : public Job
{
  Flags* m_flags;
public:
  inline ReallyDumbJob(Flags* flags) : m_flags(flags) {}

  bool doWork() override
  {
    m_flags->finished = true;
    return true;
  }

  const std::string getType() const override final
  {
    return "";
  }

  bool operator==(const Job *job) const override final
  {
    return this == job;
  }
};

class TestJobManager : public testing::Test
{
protected:
  TestJobManager() { }

  ~TestJobManager() override
  {
    m_jobManger.cancelJobs();
    m_jobManger.restart();
  }

  JobManager m_jobManger;
};

TEST_F(TestJobManager, addJob)
{
  Flags* flags = new Flags();
  ReallyDumbJob* job = new ReallyDumbJob(flags);
  m_jobManger.addJob(job, nullptr);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_TRUE(flags->finished);
  delete flags;
}

TEST_F(TestJobManager, CancelJob)
{
  unsigned int id;
  Flags* flags = new Flags();
  DummyJob* job = new DummyJob(flags);
  id = m_jobManger.addJob(job, nullptr);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_TRUE(flags->started);

  m_jobManger.cancelJob(id);

  flags->lingerAtWork = false;
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_TRUE(flags->finished);

  EXPECT_TRUE(flags->wasCanceled);
  delete flags;
}

namespace
{
struct JobControlPackage
{
  JobControlPackage()
  {
    // We're not ready to wait yet
    jobCreatedLock = std::unique_lock<std::mutex>(jobCreatedMutex);
  }

  ~JobControlPackage()
  {
  }

  bool ready = false;
  std::condition_variable jobCreatedCond;
  std::mutex jobCreatedMutex;
  std::unique_lock<std::mutex> jobCreatedLock;
};

class BroadcastingJob :
  public Job
{
public:

  BroadcastingJob(JobControlPackage &package) :
    m_package(package),
    m_finish(false)
  {
  }

  void finishAndStopBlocking()
  {
    std::unique_lock<std::mutex> lock(m_blockMutex);

    m_finish = true;
    m_block.notify_all();
  }

  const std::string getType() const override
  {
    return "BroadcastingJob";
  }

  bool operator==(const Job *job) const override final
  {
    return this == job;
  }

  bool doWork() override
  {
    {
      std::unique_lock<std::mutex> lock(m_package.jobCreatedMutex);

      m_package.ready = true;
      m_package.jobCreatedCond.notify_all();
    }

    std::unique_lock<std::mutex> blockLock(m_blockMutex);

    // Block until we're told to go away
    while (!m_finish)
      m_block.wait(blockLock);
    return true;
  }

private:

  JobControlPackage &m_package;

  std::condition_variable m_block;
  std::mutex m_blockMutex;
  bool m_finish;
};

BroadcastingJob *
WaitForJobToStartProcessing(Job::PRIORITY priority, JobControlPackage &package, JobManager &jobManger)
{
  BroadcastingJob* job = new BroadcastingJob(package);
  jobManger.addJob(job, nullptr, priority);

  // We're now ready to wait, wait and then unblock once ready
  while (!package.ready)
    package.jobCreatedCond.wait(package.jobCreatedLock);

  return job;
}
}

TEST_F(TestJobManager, PauseLowPriorityJob)
{
  JobControlPackage package;
  BroadcastingJob *job (WaitForJobToStartProcessing(Job::PRIORITY_LOW_PAUSABLE, package, m_jobManger));

  EXPECT_TRUE(m_jobManger.isProcessing(Job::PRIORITY_LOW_PAUSABLE));
  m_jobManger.pauseJobs();
  EXPECT_FALSE(m_jobManger.isProcessing(Job::PRIORITY_LOW_PAUSABLE));
  m_jobManger.unPauseJobs();
  EXPECT_TRUE(m_jobManger.isProcessing(Job::PRIORITY_LOW_PAUSABLE));

  job->finishAndStopBlocking();
}

TEST_F(TestJobManager, isProcessing)
{
  JobControlPackage package;
  BroadcastingJob *job (WaitForJobToStartProcessing(Job::PRIORITY_LOW_PAUSABLE, package, m_jobManger));

  EXPECT_EQ(0, m_jobManger.isProcessing(""));

  job->finishAndStopBlocking();
}
