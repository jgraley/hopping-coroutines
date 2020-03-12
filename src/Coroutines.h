/*
  Coroutines.h - Coroutines for Gadgets.
  Created by John Graley, 2020.
  (C) John Graley LGPL license applies.
*/
#ifndef Coroutines_h
#define Coroutines_h

#include "Coroutines_ARM.h"

#include <functional>
#include <csetjmp> 
#include <cstdint>
#include "Arduino.h"

#if __cplusplus <= 199711L
  #error This library needs at least a C++11 compliant compiler
#endif

void gcoroutines_set_logger( std::function< void(const char *) > logger );

class Coroutine
{
public:
  explicit Coroutine( std::function<void()> child_main_function_ ); 
  ~Coroutine();
    
  void operator()();
  inline static Coroutine *get_current();
  inline static void yield();

private:
  enum ChildStatus
  {
    READY,
    RUNNING,
    COMPLETE
  };
    
  byte *prepare_child_stack( byte *frame_pointer, byte *stack_pointer );
  void prepare_child_jmp_buf(jmp_buf initial_jmp_buf, byte *child_stack_pointer );
  
  [[ noreturn ]] void start_child();
  void yield_nonstatic();

  const uint32_t magic;
  std::function<void()> child_main_function;
  const int stack_size;
  byte * const child_stack_memory;
  ChildStatus child_status;
  jmp_buf parent_jmp_buf;
  jmp_buf child_jmp_buf;
    
  static const int default_stack_size = 1024;
};

///-- 
// Implement the inline functions here

Coroutine *Coroutine::get_current()
{
  return (Coroutine *)( get_cls() );
}


void Coroutine::yield()
{
  Coroutine * const that = get_current();
  if( that )
    that->yield_nonstatic();
}

#endif