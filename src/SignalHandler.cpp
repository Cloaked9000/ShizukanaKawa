//
// Created by fred on 14/04/18.
//

#include "SignalHandler.h"

#include <exception>
#include <csignal>
#include <future>
#include "Log.h"

#ifdef _WIN32
typedef BOOL (WINAPI *StackWalk64_func)
        (
                DWORD,
                HANDLE,
                HANDLE,
                LPSTACKFRAME64,
                PVOID,
                PREAD_PROCESS_MEMORY_ROUTINE64,
                PFUNCTION_TABLE_ACCESS_ROUTINE64,
                PGET_MODULE_BASE_ROUTINE64,
                PTRANSLATE_ADDRESS_ROUTINE64
        );

typedef BOOL (WINAPI *SymInitialize_func)
        (
                HANDLE hProcess,
                PCTSTR UserSearchPath,
                BOOL fInvadeProcess
        );

typedef BOOL (WINAPI *SymCleanup_func)
        (
                HANDLE hProcess
        );

typedef PVOID (WINAPI *SymFunctionTableAccess64_func)
        (
                HANDLE hProcess,
                DWORD64 AddreBase
        );

typedef DWORD64 (WINAPI *SymGetModuleBase64_func)
        (
                HANDLE hProcess,
                DWORD64 dwAddr
        );

typedef BOOL (WINAPI *SymFromAddr_func)
        (
                HANDLE,
                DWORD64,
                PDWORD64,
                PSYMBOL_INFO
        );

typedef BOOL (WINAPI *RtlNtStatusToDosError_func)
        (
                NTSTATUS
        );

HMODULE hDbgHelpDll;
StackWalk64_func funStackWalk64;
SymInitialize_func funSymInitialize;
SymCleanup_func funSymCleanup;
SymFunctionTableAccess64_func funSymFunctionTableAccess64;
SymGetModuleBase64_func funSymGetModuleBase64;
SymFromAddr_func funSymFromAddr;

HMODULE hNtdllDll;
RtlNtStatusToDosError_func funRtlNtStatusToDosError;
#else
#include <sys/ucontext.h>
#include <ucontext.h>
#include <Log.h>

struct sigaction SignalHandler::sa;
#endif

void SignalHandler::install()
{
    static SignalHandler handler;
}

SignalHandler::SignalHandler()
{
    std::set_terminate(SignalHandler::exception_handler);

#ifdef __linux__
    //Setup signal handlers for Linux
    sa.sa_sigaction = &SignalHandler::linux_fatal_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;


    //Install the signal handlers
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);

    //Enable core dumps if we can
    struct rlimit core_limits = {};
    core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &core_limits);
#else
    //Dynamically load required shared libraries and relevant function pointers
    hDbgHelpDll = LoadLibrary(TEXT("DbgHelp.dll"));
    hNtdllDll= LoadLibrary(TEXT("Ntdll.dll"));
    if(hDbgHelpDll != nullptr)
    {
        funStackWalk64 = (StackWalk64_func) GetProcAddress(hDbgHelpDll, "StackWalk64");
        funSymInitialize = (SymInitialize_func) GetProcAddress(hDbgHelpDll, "SymInitialize");
        funSymCleanup = (SymCleanup_func) GetProcAddress(hDbgHelpDll, "SymCleanup");
        funSymFunctionTableAccess64 = (SymFunctionTableAccess64_func) GetProcAddress(hDbgHelpDll, "SymFunctionTableAccess64");
        funSymGetModuleBase64 = (SymGetModuleBase64_func) GetProcAddress(hDbgHelpDll, "SymGetModuleBase64");
        funSymFromAddr = (SymFromAddr_func) GetProcAddress(hDbgHelpDll, "SymFromAddr");
        if(!funStackWalk64 || !funSymInitialize || !funSymCleanup || !funSymFunctionTableAccess64 || !funSymGetModuleBase64 || !funSymFromAddr)
        {
            frlog << Log::warn << "Couldn't load functions from DbgHelp.dll" << Log::end;
            if(!FreeLibrary(hDbgHelpDll))
                frlog << Log::crit << "Failed to free library Ntdll.dll. Error: " << GetLastError() << Log::end;
            hDbgHelpDll = nullptr;
        }
    }
    if(hNtdllDll != nullptr)
    {
        funRtlNtStatusToDosError = (RtlNtStatusToDosError_func)GetProcAddress(hNtdllDll, "RtlNtStatusToDosError");
        if(!funRtlNtStatusToDosError)
        {
            frlog << Log::warn << "Couldn't load function RtlNtStatusToDosError from Ntdll.dll. Error: " << GetLastError() << Log::end;
            if(!FreeLibrary(hNtdllDll))
                frlog << Log::crit << "Failed to free library Ntdll.dll. Error: " << GetLastError() << Log::end;
            hNtdllDll = nullptr;
        }
    }

    if(hDbgHelpDll == nullptr)
    {
        frlog << Log::warn << "Failed to load DbgHelp.dll. Stack trace generation disabled" << Log::end;
    }

    if(hNtdllDll == nullptr)
    {
        frlog << Log::warn << "Failed to load Ntdll.dll. Error code to string conversion disabled." << Log::end;
    }

    //Install the signal handler for Windows
    SetUnhandledExceptionFilter(windows_fatal_exception_handler);
#endif
}

SignalHandler::~SignalHandler()
{
#ifdef _WIN32
    //Unload shared libraries
    if(hDbgHelpDll)
    {
        if(!FreeLibrary(hDbgHelpDll))
            frlog << Log::crit << "Failed to free library Ntdll.dll. Error: " << GetLastError() << Log::end;
    }
    if(hNtdllDll)
    {
        if(!FreeLibrary(hNtdllDll))
            frlog << Log::crit << "Failed to free library Ntdll.dll. Error: " << GetLastError() << Log::end;
    }
#endif
}

void SignalHandler::exception_handler()
{
#ifdef _WIN32
    CONTEXT current_context;
    RtlCaptureContext(&current_context);
#else
    ucontext_t current_context;
    if(getcontext(&current_context) < 0)
    {
        frlog << Log::crit << "Failed to get current thread context. getcontext() errno: " << errno << Log::end;
    }
#endif
    if(auto excep = std::current_exception())
    {
        std::string excep_name = DEMANGLE(std::current_exception().__cxa_exception_type()->name());
        try
        {
            std::rethrow_exception(excep);
        }
        catch(char const *m)
        {
            complete_log("An unhandled exception consisting of '" + std::string(m) + "'. The process will now exit", &current_context);
        }
        catch(const std::exception &e)
        {
            complete_log("An unhandled exception of type '" + excep_name + "' was thrown. Reason: " + e.what() + ". The process will now exit", &current_context);
        }
        catch(...)
        {
            complete_log("There was an unhandled exception of type '" + excep_name + "'. The process will now exit", &current_context);
        }
    }
    complete_log("Exception handler called, but no exception is available. The process will now exit", &current_context);
}

void SignalHandler::complete_log(const std::string &error_message, void *thread_context)
{
    //todo: Dump various stuff (connected clients etc)

    //Print crash info
    frlog << error_message << ". Stack trace: " << Log::end;
    print_stacktrace(thread_context);

    //Flush log and exit
    frlog.flush();
    std::_Exit(EXIT_FAILURE);
}

#ifdef _WIN32

bool SignalHandler::exception_code_to_string(DWORD excep_code, char *buffer, size_t buffer_sz)
{
    //Not enabled
    if(hNtdllDll == nullptr)
    {
        char err_msg[] = "String error messages disabled";
        if(buffer_sz > sizeof(*err_msg))
        {
            strcpy(buffer, err_msg);
        }
        return false;
    }

    //Format the exception code into a printable string
    char formatted_message[255];
    DWORD dwRes = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                                hNtdllDll, funRtlNtStatusToDosError(excep_code), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR)buffer, buffer_sz, nullptr);
    if(dwRes == 0)
    {
        frlog << Log::warn << "Failed to convert exception code to string. Reason: " << GetLastError() << Log::end;
        return false;
    }
    else
    {
        //The converted string may contain unwanted characters. Strip them.
        //Some strings also contain {UNWANTED-TEXT}, strip that too.
        bool excep_began = false;
        bool excep_ended = false;
        char *c, *f;
        for(c = buffer, f = formatted_message; *c != '\0'; ++c)
        {
            if(*c == '{')
                excep_began = true;
            if(*c != 13 && *c != 10 && *c != '.' && (!excep_began || excep_ended))
                *f++ = *c;
            if(*c == '}')
                excep_ended = true;
        }
        *f++ = '\0';
    }

    //Move resulting stripped string into output buffer
    strcpy(buffer, formatted_message);
    return true;
}

LONG SignalHandler::windows_fatal_exception_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    //Convert the error message we have into a printable string
    char formatted_message[255] = {0};
    exception_code_to_string(ExceptionInfo->ExceptionRecord->ExceptionCode, formatted_message, sizeof(formatted_message));
    frlog << Log::info << "Got exception 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode
          << " (" << formatted_message << "). Fault address: 0x" << ExceptionInfo->ExceptionRecord->ExceptionAddress
          << ". Rip: 0x" << ExceptionInfo->ContextRecord->Rip << std::dec << Log::end;

    //Print a stack trace if the stack is intact
    if(ExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_STACK_OVERFLOW)
    {
        print_stacktrace(ExceptionInfo->ContextRecord);
    }
    else
    {
        frlog << Log::info << "Overflow at: 0x" << std::hex << ExceptionInfo->ContextRecord->Rip << std::dec << Log::end;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


void SignalHandler::print_stacktrace(void *thread_context)
{
    //Not enabled
    if(hDbgHelpDll == nullptr)
    {
        frlog << Log::info << "Stack tracing not enabled. Not printing." << Log::end;
        return;
    }

    auto context = static_cast<CONTEXT*>(thread_context);

    //Initialise the symbol handler
    funSymInitialize(GetCurrentProcess(), nullptr, true);

    //Setup initial stack frame to backtrace from
    STACKFRAME64 frame = {0};
    frame.AddrPC.Offset = context->Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context->Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context->Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;

    //Allocate memory to store temporary symbol information for each frame
    auto symbol = (PSYMBOL_INFO) malloc(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR));
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    //Walk through the last 16 frames in the stack
    int frame_number = 0;
    while(funStackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), GetCurrentThread(), &frame, context, 0, funSymFunctionTableAccess64, funSymGetModuleBase64, 0) && frame_number < 16)
    {
        //Try to fetch the frame for the current stack frame offset and print it
        //This will fail if the program doesn't have any debugging symbols, so ??? will be printed instead
        DWORD64 displacement = 0;
        if(funSymFromAddr(GetCurrentProcess(), frame.AddrPC.Offset, &displacement, symbol))
        {
            frlog << Log::info << "[" << frame_number << "]: " << symbol->Name << " [0x" << std::hex << symbol->Address << std::dec << "]" << Log::end;
        }
        else
        {
            frlog << Log::info << "[" << frame_number << "]: ???" << Log::end;
        }
        ++frame_number;
    }

    //Cleanup symbol handler and the temporary symbol buffer
    funSymCleanup(GetCurrentProcess());
    free(symbol);
}

#else
void SignalHandler::linux_fatal_signal_handler(int code, siginfo_t *signal_info, void *signal_context)
{
    //If it's a fault, print fault address
    auto *ctx = static_cast<ucontext_t *>(signal_context);
    if(code == SIGSEGV)
    {
        printf("Got signal %d. Fault address: %p. Rip: 0x%lx\n", code, signal_info->si_addr, ctx->uc_mcontext.fpregs->rip);
    }
    else
    {
        printf("Got signal %d\n", code);
    }

    complete_log("There was an unhandled signal of type: " + std::to_string(code) + ". The process will now exit", ctx);
}

void SignalHandler::print_stacktrace(void *thread_context)
{
    //Cast context to useful type
    auto ctx = static_cast<ucontext_t*>(thread_context);

    //Prepare to get a stack trace of the last 16 things
    void *trace[16];
    char **messages = nullptr;

    size_t funcname_length = 256;
    auto *funcname = (char*)malloc(funcname_length);

    //Get a backtrace, and overwrite sigaction with caller's address
    int trace_size = backtrace(trace, 16);
    //trace[1] = (void *)ctx->uc_mcontext.fpregs->rip;
    (void)ctx;

    //Get symbols for the trace
    messages = backtrace_symbols(trace, trace_size);

    //Print it out
    for(int i = 1; i < trace_size; ++i) //Skip first one as that's us
    {
        //Extract the function name, so that it can be demangled
        size_t function_name_begin = 0;
        size_t function_name_end = 0;
        size_t frame_address_begin = 0;
        size_t frame_address_end = 0;
        size_t function_offset_position = 0;
        for(size_t a = 0; messages[i][a] != '\0'; ++a)
        {
            if(messages[i][a] == '(')
                function_name_begin = a + 1;
            else if(messages[i][a] == '+')
                function_name_end = a;
            else if(messages[i][a] == '[')
                frame_address_begin = a + 1;
            else if(messages[i][a] == ']')
                frame_address_end = a;
            else if(messages[i][a] == ')')
                function_offset_position = a;
        }
        messages[i][function_name_end] = '\0';

        //Demangle the name
        int status;
        char* ret = abi::__cxa_demangle(messages[i] + function_name_begin, funcname, &funcname_length, &status);

        if(status == 0) //Demangling succeeded, print demangled info
        {
            messages[i][frame_address_end] = '\0';
            messages[i][function_offset_position] = '\0';

            funcname = ret; //__cxa_demangle could have reallocated the string if it needed expanding
            frlog << Log::info << "[" << i << "]: " << funcname << " + " << &messages[i][function_name_end + 1] << " [" << &messages[i][frame_address_begin] << "]" << Log::end;
        }
        else //Demangling failed, restore original string and print
        {
            if(function_name_end != 0)
                messages[i][function_name_end] = '[';
            messages[i][function_name_end] = '[';
            frlog << Log::info << "[" << i << "]: " << messages[i] << Log::end;
        }
    }

    //Free malloced memory
    free(messages);
    free(funcname);
}

#endif
