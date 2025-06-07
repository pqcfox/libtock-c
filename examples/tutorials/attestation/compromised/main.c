#include <stdio.h>
#include <stdlib.h>

#include <libtock/kernel/ipc.h>

bool started = false;

static void ipc_callback(__attribute__ ((unused)) int   pid,
                         __attribute__ ((unused)) int   len,
                         __attribute__ ((unused)) int   arg2,
                         __attribute__ ((unused)) void* ud) {
  started = true;
}

int main(void) {
  ipc_register_service_callback("org.tockos.tutorials.attestation.compromised", ipc_callback,
                                NULL);

  yield_for(&started);

  printf("Hello IPC! We're selected!\n");

  while (1) {
    yield();
  }
}
