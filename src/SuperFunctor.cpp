

#include "SuperFunctor.h"
#include "CoroTracing.h"

using namespace std;

SuperFunctor::SuperFunctor() :
   entrypoint_fpt( UnpackMemberFunctionPointer( &SuperFunctor::operator() ) ) 
{
    const pair<ThumbInstruction *, int> assembly = GetAssembly();
    ASSERT( assembly.second <= MAX_INSTRUCTIONS, "Thunk assmebly too long" );
    memcpy( entrypoint_thunk, assembly.first, assembly.second * sizeof(ThumbInstruction) );     
}


SuperFunctor::~SuperFunctor()
{
}


SuperFunctor::EntryPointFP SuperFunctor::GetEntryPoint()
{
    auto ep_word = (uint32_t )entrypoint_thunk;
    ep_word |= 1; // set the "thumb" bit
    return (EntryPointFP)ep_word;
}


void SuperFunctor::operator()()
{
    // We end up here on the first superfunctor invocation because our
    // constructor used the SuperFunctor vtable when unpacking: bootstrap
    // into the correct vtable by unpacking again.
    EntryPointMFP mfp = &SuperFunctor::operator();
    EntryPointFPT new_fpt = UnpackMemberFunctionPointer( mfp ); 
    
    // If someone invoked operator() on a SuperFunctor instance, don't 
    // recurse infinitely!
    if( new_fpt == entrypoint_fpt )
        return; 
    entrypoint_fpt = new_fpt;
        
    // Invoke the function. We'd like a tail call here, due to re-entrancy.
    entrypoint_fpt(this);
}


pair<SuperFunctor::ThumbInstruction *, int> SuperFunctor::GetAssembly()
{
    const int pipeline_overshoot = 4;

    volatile bool actually_run_it = false;
    if( actually_run_it )
    {
        BEGIN:
        asm( "mov r0, pc\n\t" // Get PC
             "sub r0, %[this_to_PC_offset]\n\t" // Calculate "this" pointer into r0
             "ldr r1, [r0, %[this_to_entrypoint_offset]]\n\t" // Get entrypoint function pointer
             "bx  r1" // Jump to it (tail call)
              : : [this_to_PC_offset] "I" (offsetof(SuperFunctor, entrypoint_thunk) + pipeline_overshoot),
                  [this_to_entrypoint_offset] "M" (offsetof(SuperFunctor, entrypoint_fpt)) : );            
        END:
            for(;;);
    }
    
    auto begin = (ThumbInstruction *)(&&BEGIN);
    auto end = (ThumbInstruction *)(&&END);
    
    return make_pair( begin, end-begin );
}


SuperFunctor::EntryPointFPT SuperFunctor::UnpackMemberFunctionPointer( EntryPointMFP mfp )
{
    // alias an array of words over the member function pointer
    typedef array<uint32_t, sizeof(EntryPointMFP)/sizeof(uint32_t)> MFPWords;
    MFPWords &mfp_words = *(MFPWords *)&mfp;
    ASSERT( mfp_words.size() == 2, "Unexpected size of member function pointer"); 
    
    auto this_words = (uint32_t *)this;
    
    EntryPointFPT fpt;
    if( mfp_words[1] & 1 )
    {
        int this_offset_bytes = mfp_words[1] >> 1;
        auto vtable = (EntryPointFPT *)(this_words[this_offset_bytes>>2]);
        int vtable_offset_bytes = mfp_words[0];
        fpt = vtable[vtable_offset_bytes>>2];
    }
    else
    {
        fpt = (EntryPointFPT)(mfp_words[0]);        
    }        
    return fpt;
}


#ifdef SUPERFUNCTOR_TESTS  
void SuperFunctor::TestPCRelative()
{
    uint8_t *pc_grab;
    asm( "mov %[result], pc" :  [result] "=r" (pc_grab)  : : );    
    TRACE("Grabbed PC: %p; function pointer:%p", pc_grab, &SuperFunctor::TestPCRelative);
    /* What I learned:
     * - Grabbed PC is ahead of the mov instruction by 4 bytes (actually 2 
     *   instructions, I think, so only correct in thumb mode)
     * - Function pointer has bit 0 set to 1, indicating that it points
     *   to a thumb instruction (if bit 0 were 0, a Cortex M0 would fault)
     */
}


void SuperFunctor::TestMemberFunctionPointer()
{
    void (SuperFunctor::* volatile mfp)() = &SuperFunctor::operator();
    TRACE("Size of member function pointer: %d", sizeof(mfp));
    TRACE("As uint32_t: %x %x", ((uint32_t *)&mfp)[0], ((uint32_t *)&mfp)[1] );
    SuperFunctor s;
    //(s.*mfp)();
    TRACE("Thises: %p %p", &s, (SuperFunctor *)&s );
    TRACE("entrypoint_thunk: %p", &(s.entrypoint_thunk) );

    /* What I learned:
     * - Size of MFP is 8 bytes: Updated:
     * If non-virtual: { FP, 0 }
     * If virtual: { vtable-offset, (object-offset<<1) | 1 }
     * 
     * To invoke function given { P, Q }:
     * if( Q.0 == 0 )
     *   call P
     * else
     *   call *( *(object+(Q>>1)) + P )
     */   
}


void SuperFunctor::TestUnpack()
{
    SuperFunctor s;
    EntryPointMFP mfp = &SuperFunctor::operator();
    EntryPointFPT fpt = s.UnpackMemberFunctionPointer( mfp ); 
    TRACE("Thises: %p %p", &s, (SuperFunctor *)&s );
    TRACE("operator() as uint32_t: %x %x", ((uint32_t *)&mfp)[0], ((uint32_t *)&mfp)[1] );
    TRACE("FPT as uint32_t: %x", ((uint32_t *)&fpt)[0] );
}
#endif
