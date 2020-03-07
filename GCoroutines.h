/*
  GCoroutines.h - Coroutines for Gadgets.
  Created by John Graley, 2020.
  (C) John Graley LGPL license applies.
*/
#ifndef GCoroutines_h
#define GCoroutines_h

#include "Arduino.h"
#include <functional>
#include <csetjmp> 
#include <cstdint>
	
/// @TODO require C++11

void gcoroutines_set_logger( std::function< void(const char *) > logger );

class GCoroutine
{
public:
    typedef uint8_t byte;

    explicit GCoroutine( std::function<void()> child_main_function_ ); 
    ~GCoroutine();
    
    void run_iteration();
    static void yield();

private:
    enum ChildStatus
    {
        READY,
        RUNNING,
        COMPLETE
    };
    
    [[ noreturn ]] void start_child();

    const uint32_t magic;
    std::function<void()> child_main_function;
    const int stack_size;
    byte * const child_stack_memory;
    ChildStatus child_status;
    jmp_buf parent_jmp_buf;
    jmp_buf child_jmp_buf;
    
    static const int default_stack_size = 1024;
};

void co_yield();

#endif
