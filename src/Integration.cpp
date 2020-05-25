/**
 * Integration.cpp - Coroutines for Gadgets.
 * Created by John Graley, 2020.
 * (C) John Graley LGPL license applies.
 * 
 * Here, we integrate Gadget Coroutines into the Arduino environment.
 */

#include "Coroutine.h"

#include <cstring>
#include <functional>
#include <csetjmp> 
#include <cstdint>

#include "Arduino.h"

using namespace std;
using namespace GC;
using namespace Arm;

// This makes sure the TR is a NULL pointer for the foreground
// context (i.e. when outside any coroutine)
void __attribute__ ((constructor)) init_baseline_tr()
{
  set_tr( nullptr );
}


// Implement Arduino yield operation (called by eg delay()) to yield any 
// coroutine that might be running. This makes functions like delay() 
// become seemingly coroutine-aware.
extern  "C" void __attribute__((used)) yield(void) 
{
  Coroutine::yield();
}

// This does what the system yield does if you don't over-ride it.
extern void system_idle_tasks()
{
#if defined(USE_TINYUSB)
  tud_task();
  tud_cdc_write_flush();
#endif
}


void bring_in_Integration()
{
}
