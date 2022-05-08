/**
    @file
    @author  Alexander Sherikov
    @copyright 2022 Alexander Sherikov. Licensed under the Apache License,
    Version 2.0. (see LICENSE or http://www.apache.org/licenses/LICENSE-2.0)
    @brief
*/


#define BOOST_TEST_MODULE thread_supervisor
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/results_reporter.hpp>
#include <boost/timer/timer.hpp>


struct GlobalFixtureConfig
{
    GlobalFixtureConfig()
    {
        boost::unit_test::results_reporter::set_level(boost::unit_test::DETAILED_REPORT);
    }
    ~GlobalFixtureConfig() = default;
};


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
// Depending on Boost version a compiler may issue a warning about extra ';',
// at the same time, compilation may fail on some systems if ';' is omitted.
BOOST_GLOBAL_FIXTURE(GlobalFixtureConfig);
#pragma GCC diagnostic pop


#include "thread_supervisor/supervisor.h"


namespace
{
    class TestThreadSupervisor : public tut::thread::InheritableSupervisor<>
    {
    public:
        std::atomic<std::size_t> counter_;

    public:
        void stopSupervisedThreads() override
        {
            getThreadSupervisor().stop();
        }

        void threadFunction()
        {
            for (;;)
            {
                if (isThreadSupervisorInterrupted())
                {
                    break;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        void threadCounter()
        {
            ++counter_;
        }

        TestThreadSupervisor()
        {
            counter_ = 0;
        }

        ~TestThreadSupervisor()
        {
            stopSupervisedThreads();
        }
    };
}  // namespace



BOOST_AUTO_TEST_CASE(ThreadSupervisorStop)
{
    TestThreadSupervisor pool;
    pool.addSupervisedThread(tut::thread::Parameters(), &TestThreadSupervisor::threadFunction, &pool);
    pool.addSupervisedThread(tut::thread::Parameters(), &TestThreadSupervisor::threadFunction, &pool);
    pool.addSupervisedThread(tut::thread::Parameters(), &TestThreadSupervisor::threadFunction, &pool);
}


BOOST_AUTO_TEST_CASE(ThreadSupervisorCount)
{
    TestThreadSupervisor pool;
    BOOST_CHECK_EQUAL(pool.counter_, static_cast<std::size_t>(0));

    pool.addSupervisedThread(tut::thread::Parameters(tut::thread::Parameters::Restart(/*attempts=*/10, /*sleep_ms=*/5),
                                                     tut::thread::Parameters::TerminationPolicy::IGNORE),
                             &TestThreadSupervisor::threadCounter,
                             &pool);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    BOOST_CHECK_EQUAL(pool.counter_, static_cast<std::size_t>(10));
    BOOST_CHECK(not pool.isThreadSupervisorInterrupted());
}

