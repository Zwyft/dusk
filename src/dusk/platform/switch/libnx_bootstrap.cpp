#if defined(__SWITCH__)
#include <cstddef>
#include <switch.h>

extern "C" {
// Keep bootstrap overrides minimal for hbmenu/homebrew compatibility.
// Forcing AppletType_Application can break startup in applet/homebrew context.
alignas(16) u8 __nx_exception_stack[0x4000];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
}
#endif
