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
        /**
         * Simple stderr logger, can be overridden by corresponding template
         * parameter.
         */
        class StdErr
        {
        public:
            template <class t_Arg, class... t_Args>
            void log(const t_Arg &arg, t_Args &&...args) const
            {
                std::cerr << arg;
                // cppcheck-suppress ignoredReturnValue
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
        /// Thread parameters
        class Parameters
        {
        public:
            /// What to do when thread terminated
            enum class TerminationPolicy
            {
                IGNORE,  /// ignore and proceed
                KILLALL  /// kill all other threads
            };

            /// Exception handling policy
            enum class ExceptionPolicy
            {
                CATCH,  /// Catch & report
                PASS    /// application is going to crash
            };


            /// Restarting parameters
            class Restart
            {
            public:
                std::size_t attempts_;  /// 0 = unlimited
                std::size_t sleep_ms_;  /// delay between attempts

            public:
                explicit Restart(const std::size_t attempts = 0, const std::size_t sleep_ms = 0)  // NOLINT
                {
                    attempts_ = attempts;
                    sleep_ms_ = sleep_ms;
                }

                [[nodiscard]] bool isUnlimited() const
                {
                    return (0 == attempts_);
                }

                [[nodiscard]] bool isOk(const std::size_t attempt) const
                {
                    return (isUnlimited() or attempt < attempts_);
                }

                [[nodiscard]] bool isEnabled() const
                {
                    return (isOk(1));
                }

                void wait() const
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms_));
                }
            };


            /// Scheduling parameters, POSIX threads only, see pthread_setschedparam
            class Scheduling
            {
            public:
                int priority_;          /// thread priority
                int policy_;            /// thread policy
                bool ignore_failures_;  /// proceed if scheduling parameters could not be set

            public:
                explicit Scheduling(
                        const int priority = 0,         // NOLINT
                        const int policy = SCHED_FIFO,  // NOLINT
                        const bool ignore_failures = true)
                {
                    priority_ = priority;
                    policy_ = policy;
                    ignore_failures_ = ignore_failures;
                }


                bool apply(const std::thread::native_handle_type &&handle) const  // NOLINT return value can be ignored
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

#ifdef THREAD_SUPERVISOR_THOU_SHALT_NOT_PASS  /// do not allow threads to exit / crash quietly
            TerminationPolicy termination_policy_ = TerminationPolicy::KILLALL;
            ExceptionPolicy exception_policy_ = ExceptionPolicy::PASS;
#else
            TerminationPolicy termination_policy_ = TerminationPolicy::IGNORE;
            ExceptionPolicy exception_policy_ = ExceptionPolicy::CATCH;
#endif


        public:
            template <class... t_Args>
            Parameters(const Restart &&restart, t_Args &&...args)  // NOLINT noexplicit
              : Parameters(std::forward<t_Args>(args)...)
            {
                restart_ = restart;
            }

            template <class... t_Args>
            Parameters(const Scheduling &&scheduling, t_Args &&...args)  // NOLINT noexplicit
              : Parameters(std::forward<t_Args>(args)...)
            {
                scheduling_ = scheduling;
            }

            template <class... t_Args>
            Parameters(const TerminationPolicy termination_policy, t_Args &&...args)  // NOLINT noexplicit
              : Parameters(std::forward<t_Args>(args)...)
            {
                termination_policy_ = termination_policy;
            }

            template <class... t_Args>
            Parameters(const ExceptionPolicy exception_policy, t_Args &&...args)  // NOLINT noexplicit
              : Parameters(std::forward<t_Args>(args)...)
            {
                exception_policy_ = exception_policy;
            }

            Parameters() = default;
        };



        template <class t_Logger = log::StdErr>
        class Supervisor;


        /// Thread handling class
        class Thread
        {
            // THREAD_SUPERVISOR_DISABLE_CLASS_COPY(Thread)

        public:
            using List = std::list<Thread>;
            using Reference = List::iterator;

        public:
            Parameters parameters_;
            std::thread thread_;
            Reference self_;


        protected:
            template <class t_Logger, class t_Function, class... t_Args>
            void startOnce(Supervisor<t_Logger> *supervisor, t_Function &&function, t_Args &&...args)
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
            void startLoop(Supervisor<t_Logger> *supervisor, t_Function &&function, t_Args &&...args)
            {
                bool started = false;
                for (std::size_t attempt = 0; parameters_.restart_.isOk(attempt) and not supervisor->isInterrupted();
                     ++attempt)
                {
                    if (started)
                    {
                        parameters_.restart_.wait();
                        if (supervisor->isInterrupted())
                        {
                            break;
                        }

                        supervisor->log(
                                "Supervisor / restarting thread: ",
                                attempt + 1,
                                " / ",
                                parameters_.restart_.isUnlimited() ? 0 : parameters_.restart_.attempts_);
                    }
                    else
                    {
                        started = true;
                    }

                    startOnce(supervisor, function, std::forward<t_Args>(args)...);
                }
            }


            template <class t_Logger, class t_Function, class... t_Args>
            void startThread(Supervisor<t_Logger> *supervisor, t_Function &&function, t_Args &&...args)
            {
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
            void start(
                    Supervisor<t_Logger> *supervisor,
                    Reference self,
                    const Parameters &&parameters,
                    t_Args &&...args)
            {
                self_ = self;
                parameters_ = parameters;
                thread_ = std::thread(
                        &Thread::startThread<t_Logger, t_Args...>, this, supervisor, std::forward<t_Args>(args)...);

                if (not parameters_.scheduling_.apply(thread_.native_handle()))
                {
                    supervisor->log("Supervisor error: could not configure custom thread scheduling.");
                    if (not parameters_.scheduling_.ignore_failures_)
                    {
                        std::terminate();
                    }
                }
            }

            void join()
            {
                if (thread_.joinable())
                {
                    thread_.join();
                }
            }
        };



        /// Thread supervisor
        template <class t_Logger>
        class Supervisor : public t_Logger
        {
            friend class Thread;
            THREAD_SUPERVISOR_DISABLE_CLASS_COPY(Supervisor)

        protected:
            enum class Status
            {
                UNDEFINED = 0,
                ACTIVE = 1,
                INTERRUPTED = 2
            };


        protected:
            std::atomic<Status> status_;

            std::mutex threads_mutex_;
            Thread::List threads_;

            std::mutex terminated_threads_mutex_;
            std::list<Thread::Reference> terminated_threads_;


        protected:
            bool wait(const std::size_t wait_ms)
            {
                const std::size_t sleep_ms = 10;

                {
                    const std::lock_guard<std::mutex> lock(threads_mutex_);
                    std::size_t counter = 0;
                    for (;;)
                    {
                        if (counter * sleep_ms < wait_ms)
                        {
                            {
                                const std::lock_guard<std::mutex> terminated_lock(terminated_threads_mutex_);

                                for (const Thread::Reference &thread_ref : terminated_threads_)
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
                            // cppcheck-suppress ignoredReturnValue
                            log("Threads did not terminate in the given time after request.");
                            return (false);
                        }
                    }
                }
            }


            bool empty()
            {
                const std::lock_guard<std::mutex> lock(threads_mutex_);
                return (threads_.empty());
            }


            void drop(const std::list<Thread>::iterator &item)
            {
                const std::lock_guard<std::mutex> lock(terminated_threads_mutex_);
                terminated_threads_.push_back(item);
            }


        public:
            using t_Logger::log;


            Supervisor()
            {
                status_ = Status::UNDEFINED;
            }


            ~Supervisor()
            {
                if (Status::ACTIVE == status_)
                {
                    // - throwing in destructor -> terminate(), unless noexcept(false) is set.
                    // - noexcept(false) however interferes with other stuff in derived classes.
                    // - this check is intended to detect API misuse and should not fail under
                    // normal conditions.
                    // cppcheck-suppress ignoredReturnValue
                    log("Destructor of Supervisor is reached with active status,"
                        "which means that interrupt() method has not been called.");
                    std::terminate();
                }

                if (not empty())
                {
                    // cppcheck-suppress ignoredReturnValue
                    log("Destructor of Supervisor is reached with some threads still running.");
                    std::terminate();
                }
            }


            /// Interrupt execution, see @ref isInterrupted
            void interrupt()
            {
                status_ = Status::INTERRUPTED;
            }


            /// Check if interrupted, child threads should stop when true (pass supervisor as a parameter).
            [[nodiscard]] bool isInterrupted() const
            {
                return (Status::INTERRUPTED == status_);
            }


            /// Interrupt and wait for children to stop
            bool stop(const std::size_t wait_ms = 10000)
            {
                interrupt();
                return (wait(wait_ms));
            }


            /// Add a thread: (<thread parameters>, <function pointer>, <function parameters>)
            template <class... t_Args>
            void add(t_Args &&...args)
            {
                if (isInterrupted())
                {
                    // cppcheck-suppress ignoredReturnValue
                    log("Addition of a thread attempted after interrupt.");
                    std::terminate();
                }
                else
                {
                    status_ = Status::ACTIVE;
                    const std::lock_guard<std::mutex> lock(threads_mutex_);
                    threads_.emplace_back();
                    threads_.back().start(this, --threads_.end(), std::forward<t_Args>(args)...);
                }
            }
        };



        /// Inheritable supervisor simplifies starting class methods in separate threads
        template <class t_Logger = log::StdErr>
        class InheritableSupervisor
        {
        protected:
            Supervisor<t_Logger> thread_supervisor_;

        protected:
            ~InheritableSupervisor() = default;

        public:
            [[nodiscard]] const Supervisor<t_Logger> &getThreadSupervisor() const
            {
                return (thread_supervisor_);
            }

            Supervisor<t_Logger> &getThreadSupervisor()
            {
                return (thread_supervisor_);
            }


            /// should call stop(), pure virtual to remind that threads must be
            /// destructed properly at the right time, e.g., from a destructor
            virtual void stopSupervisedThreads() = 0;
            /*
            {
                getThreadSupervisor().stop();
            }
            */


            /// Add a thread: (<thread parameters>, <function pointer>, <this>, <function parameters>)
            template <class... t_Args>
            void addSupervisedThread(t_Args &&...args)
            {
                getThreadSupervisor().add(std::forward<t_Args>(args)...);
            }


            /// Should be checked by threaded class methods
            [[nodiscard]] bool isThreadSupervisorInterrupted() const
            {
                return (getThreadSupervisor().isInterrupted());
            }
        };
    }  // namespace thread
}  // namespace tut
