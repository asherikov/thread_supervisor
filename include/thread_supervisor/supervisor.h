/**
    @file
    @author  Alexander Sherikov
    @copyright 2022 Alexander Sherikov. Licensed under the Apache License,
    Version 2.0. (see LICENSE or http://www.apache.org/licenses/LICENSE-2.0)
    @brief
*/


#pragma once

#include <thread>
#include <functional>
#include <atomic>
#include <mutex>
#include <iostream>
#include <chrono>
#include <list>

#include <pthread.h>

#include "util.h"


namespace tut
{
    namespace log
    {
        class StdErr
        {
        public:
            template <class t_Arg, class... t_Args>
            void log(const t_Arg &arg, t_Args &&... args) const
            {
                std::cerr << arg;
                log(std::forward<t_Args>(args)...);
            }

            void log() const
            {
                std::cerr << std::endl;
            }
        };
    }  // namespace log


    namespace thread
    {
        class Parameters
        {
        public:
            enum class TerminationPolicy
            {
                IGNORE,
                KILLALL
            };

            enum class ExceptionPolicy
            {
                CATCH,
                PASS  // application is going to crash
            };


            class Restart
            {
            public:
                std::size_t attempts_;  // 0 = unlimited
                std::size_t sleep_ms_;

            public:
                explicit Restart(const std::size_t attempts = 0, const std::size_t sleep_ms = 0)
                {
                    attempts_ = attempts;
                    sleep_ms_ = sleep_ms;
                }

                bool isUnlimited() const
                {
                    return (0 == attempts_);
                }

                bool isOk(const std::size_t attempt) const
                {
                    return (isUnlimited() or attempt < attempts_);
                }

                bool isEnabled() const
                {
                    return (isOk(1));
                }

                void wait() const
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms_));
                }
            };


            class Scheduling
            {
            public:
                int priority_;
                int policy_;
                bool ignore_failures_;

            public:
                explicit Scheduling(const int priority = 0, const int policy = SCHED_FIFO, const bool ignore_failures = true)
                {
                    priority_ = priority;
                    policy_ = policy;
                    ignore_failures_ = ignore_failures;
                }


                bool apply(const std::thread::native_handle_type &&handle) const
                {
                    if (SCHED_FIFO != policy_ or 0 != priority_)  // custom parameters
                    {
                        sched_param sched_params;
                        sched_params.sched_priority = priority_;

                        if (0 != pthread_setschedparam(handle, policy_, &sched_params))
                        {
                            return (false);
                        }
                    }
                    return (true);
                }
            };


        public:
            Restart restart_;
            Scheduling scheduling_;

#ifdef THREAD_SANITIZER_THOU_SHALT_NOT_PASS  // do not allow threads to exit / crash quietly
            TerminationPolicy termination_policy_ = TerminationPolicy::KILLALL;
            ExceptionPolicy exception_policy_ = ExceptionPolicy::PASS;
#else
            TerminationPolicy termination_policy_ = TerminationPolicy::IGNORE;
            ExceptionPolicy exception_policy_ = ExceptionPolicy::CATCH;
#endif


        public:
            template <class... t_Args>
            Parameters(const Restart &&restart, t_Args &&... args) : Parameters(std::forward<t_Args>(args)...)
            {
                restart_ = restart;
            }

            template <class... t_Args>
            Parameters(const Scheduling &&scheduling, t_Args &&... args) : Parameters(std::forward<t_Args>(args)...)
            {
                scheduling_ = scheduling;
            }

            template <class... t_Args>
            Parameters(const TerminationPolicy termination_policy, t_Args &&... args)
              : Parameters(std::forward<t_Args>(args)...)
            {
                termination_policy_ = termination_policy;
            }

            template <class... t_Args>
            Parameters(const ExceptionPolicy exception_policy, t_Args &&... args)
              : Parameters(std::forward<t_Args>(args)...)
            {
                exception_policy_ = exception_policy;
            }

            Parameters()
            {
            }
        };



        template <class t_Logger = log::StdErr>
        class Supervisor;


        class Thread
        {
            // THREAD_SANITIZER_DISABLE_CLASS_COPY(Thread)

        public:
            using List = std::list<Thread>;
            using Reference = List::iterator;

        public:
            Parameters parameters_;
            std::thread thread_;
            Reference self_;


        protected:
            template <class t_Logger, class t_Function, class... t_Args>
            void startOnce(Supervisor<t_Logger> *supervisor, t_Function &&function, t_Args &&... args)
            {
                switch (parameters_.exception_policy_)
                {
                    case Parameters::ExceptionPolicy::PASS:
                        std::bind(function, std::forward<t_Args>(args)...)();
                        break;

                    case Parameters::ExceptionPolicy::CATCH:
                        try
                        {
                            std::bind(function, std::forward<t_Args>(args)...)();
                        }
                        catch (const std::exception &e)
                        {
                            supervisor->log("Supervisor / intercepted thread exception: ", e.what());
                        }
                        break;

                    default:
                        supervisor->log("Supervisor error: unknown exception handling type");
                        break;
                }
            }


            template <class t_Logger, class t_Function, class... t_Args>
            void startLoop(Supervisor<t_Logger> *supervisor, t_Function &&function, t_Args &&... args)
            {
                bool started = false;
                for (std::size_t attempt = 0; parameters_.restart_.isOk(attempt) and not supervisor->isInterrupted(); ++attempt)
                {
                    if (started)
                    {
                        parameters_.restart_.wait();
                        if (supervisor->isInterrupted())
                        {
                            break;
                        }
                        else
                        {
                            supervisor->log("Supervisor / restarting thread: ",
                                            attempt + 1,
                                            " / ",
                                            parameters_.restart_.isUnlimited() ? 0 : parameters_.restart_.attempts_);
                        }
                    }
                    else
                    {
                        started = true;
                    }

                    startOnce(supervisor, function, std::forward<t_Args>(args)...);
                }
            }


            template <class t_Logger, class t_Function, class... t_Args>
            void startThread(Supervisor<t_Logger> *supervisor, t_Function &&function, t_Args &&... args)
            {
                if (not parameters_.scheduling_.apply(thread_.native_handle()))
                {
                    supervisor->log("Supervisor error: could not configure custom thread scheduling.");
                    if (not parameters_.scheduling_.ignore_failures_)
                    {
                        std::terminate();
                    }
                }

                if (parameters_.restart_.isEnabled())
                {
                    startLoop(supervisor, function, std::forward<t_Args>(args)...);
                }
                else
                {
                    startOnce(supervisor, function, std::forward<t_Args>(args)...);
                }


                switch (parameters_.termination_policy_)
                {
                    case Parameters::TerminationPolicy::KILLALL:
                        supervisor->interrupt();
                        supervisor->drop(self_);
                        break;

                    case Parameters::TerminationPolicy::IGNORE:
                        supervisor->drop(self_);
                        break;

                    default:
                        supervisor->log("Supervisor error: unknown termination handling type");
                        break;
                }
            }


        public:
            template <class t_Logger, class... t_Args>
            void start(Supervisor<t_Logger> *supervisor, Reference self, const Parameters &&parameters, t_Args &&... args)
            {
                self_ = self;
                parameters_ = parameters;
                thread_ = std::thread(&Thread::startThread<t_Logger, t_Args...>, this, supervisor, std::forward<t_Args>(args)...);
            }

            void join()
            {
                if (thread_.joinable())
                {
                    thread_.join();
                }
            }
        };



        template <class t_Logger>
        class Supervisor : public t_Logger
        {
            friend class Thread;
            THREAD_SANITIZER_DISABLE_CLASS_COPY(Supervisor)


        protected:
            std::atomic<bool> interrupted_;

            std::mutex threads_mutex_;
            Thread::List threads_;

            std::mutex terminated_threads_mutex_;
            std::list<Thread::Reference> terminated_threads_;


        protected:
            bool wait(const std::size_t wait_ms)
            {
                const std::size_t sleep_ms = 10;

                std::size_t counter = 0;
                {
                    std::lock_guard<std::mutex> lock(threads_mutex_);
                    for (;;)
                    {
                        if (counter * sleep_ms < wait_ms)
                        {
                            {
                                std::lock_guard<std::mutex> terminated_lock(terminated_threads_mutex_);

                                for (Thread::Reference &thread_ref : terminated_threads_)
                                {
                                    thread_ref->join();
                                    threads_.erase(thread_ref);
                                }
                                terminated_threads_.clear();
                            }

                            if (not threads_.empty())
                            {
                                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                                ++counter;
                            }
                            else
                            {
                                return (true);
                            }
                        }
                        else
                        {
                            log("Threads did not terminate in the given time after request.");
                            return (false);
                        }
                    }
                }
            }


            bool empty()
            {
                std::lock_guard<std::mutex> lock(threads_mutex_);
                return (threads_.empty());
            }


            void drop(const std::list<Thread>::iterator &item)
            {
                std::lock_guard<std::mutex> lock(terminated_threads_mutex_);
                terminated_threads_.push_back(item);
            }


        public:
            using t_Logger::log;


            Supervisor()
            {
                interrupted_ = false;
            }


            ~Supervisor()
            {
                if (false == isInterrupted())
                {
                    // - throwing in destructor -> terminate(), unless noexcept(false) is set.
                    // - noexcept(false) however interferes with other stuff in derived classes.
                    // - this check is intended to detect API misuse and should not fail under
                    // normal conditions.
                    log("Destructor of Supervisor is reached with 'interrupted_ = false',"
                        "which means that terminate() method was not called.");
                    std::terminate();
                }

                if (not empty())
                {
                    log("Destructor of Supervisor is reached with some threads still running.");
                    std::terminate();
                }
            }


            void interrupt()
            {
                interrupted_ = true;
            }


            bool isInterrupted() const
            {
                return (interrupted_);
            }


            bool stop(const std::size_t wait_ms = 10000)
            {
                interrupt();
                return (wait(wait_ms));
            }


            template <class... t_Args>
            void add(t_Args &&... args)
            {
                if (isInterrupted())
                {
                    log("Addition of a thread attempted after interrupt.");
                    std::terminate();
                }
                else
                {
                    std::lock_guard<std::mutex> lock(threads_mutex_);
                    threads_.emplace_back();
                    threads_.back().start(this, --threads_.end(), std::forward<t_Args>(args)...);
                }
            }
        };



        template <class t_Logger = log::StdErr>
        class InheritableSupervisor
        {
        protected:
            Supervisor<t_Logger> thread_supervisor_;


        public:
            const Supervisor<t_Logger> &getThreadSupervisor() const
            {
                return (thread_supervisor_);
            }

            Supervisor<t_Logger> &getThreadSupervisor()
            {
                return (thread_supervisor_);
            }


            // should call stop(), pure virtual to remind that threads must be destructed properly at the right time.
            virtual void stopSupervisedThreads() = 0;
            /*
            {
                getThreadSupervisor().stop();
            }
            */


            template <class... t_Args>
            void addSupervisedThread(t_Args &&... args)
            {
                getThreadSupervisor().add(std::forward<t_Args>(args)...);
            }


            bool isThreadSupervisorInterrupted() const
            {
                return(getThreadSupervisor().isInterrupted());
            }
        };
    }  // namespace thread
}  // namespace tut
