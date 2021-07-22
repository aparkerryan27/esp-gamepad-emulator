

/*
 *  main.c
 *
 *  Minimal main application that initializes BTstack, prepares the example and enters BTstack's Run Loop.
 *
 *  If needed, you can create other threads. Please note that BTstack's API is not thread-safe and can only be
 *  called from BTstack timers or in response to its callbacks, e.g. packet handlers.
 */

#include "btstack_port_esp32.h"
#include "btstack_run_loop.h"
#include "hci_dump.h"
#include "hci_dump_embedded_stdout.h"

#include <stddef.h>

extern int btstack_main(int argc, const char * argv[]);

int app_main(void){

    // optional: enable packet logger
    // hci_dump_init(hci_dump_embedded_stdout_get_instance());

    // Configure BTstack for ESP32 VHCI Controller
    btstack_init();

    // Setup example
    btstack_main(0, NULL);

    // Enter run loop (forever)
    btstack_run_loop_execute();

    return 0;
}
