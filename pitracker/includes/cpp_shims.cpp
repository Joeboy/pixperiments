extern "C" {

#include <pi/debug.h>

void abort(void) {
    debug("abort\r\n");
    while (1);
}


void __cxa_pure_virtual(void) {
    debug("attempt to use a virtual function before object has been constructed\r\n");
    for ( ; ; );
}

}


namespace __gnu_cxx {
    void __verbose_terminate_handler() {
        debug("verbose_terminate_handler\r\n");
    }
}
