//
// Created by fred on 14/04/18.
//

#ifndef SFTPMEDIASTREAMER_SIGNALHANDLER_H
#define SFTPMEDIASTREAMER_SIGNALHANDLER_H

#include <sys/types.h>
#include <csignal>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <winternl.h>
#else
#include <execinfo.h>
#include <sys/resource.h>
#endif


class SignalHandler
{
public:

    /*!
     * Installs signal handlers. This must be called, or the
     * error handlers will not run.
     */
    static void install();

private:
    SignalHandler();
    ~SignalHandler();

    /*!
     * Called automatically when there's an unhandled exception.
     */
    static void exception_handler();

    /*!
     * Shared logging code between the signal handler and the exception handler.
     *
     * @param error_message The generated message so far.
     * @param thread_context The current thread context, for printing a stack trace.
     */
    static void complete_log(const std::string &error_message, void *thread_context);

    /*!
     * Prints a stack trace to log
     *
     * @param thread_context The context of the thread to print a stacktrace for
     */
    static void print_stacktrace(void *thread_context);

#ifdef _WIN32
    /*!
     * The handler for unrecoverable exceptions. Prints a stacktrace
     * and other debugging information.
     *
     * @param exception_info The exception information
     * @return What the process should do next (continue/default handler etc)
     */
    static long windows_fatal_exception_handler(EXCEPTION_POINTERS *ExceptionInfo);

    /*!
     * Converts an exception code into a printable string
     *
     * @param excep_code The code to convert
     * @param buffer The input&output buffer for the null-terminated strings
     * @param buffer_sz The buffer length
     * @return True on success, false on failure
     */
    static bool exception_code_to_string(DWORD excep_code, char *buffer, size_t buffer_sz);
#else
    /*
     * Called automatically when there's a signal that results
     * in an ungraceful stop.
     *
     * @param code The code of the signal being handled
     */
    static void linux_fatal_signal_handler(int code, siginfo_t *signal_info, void *signal_context);

    static struct sigaction sa;
#endif
};



#endif //SFTPMEDIASTREAMER_SIGNALHANDLER_H
