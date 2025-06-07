#include "libtock/tock.h"
#include <stdio.h>
#include <stdlib.h>

#include <libtock/kernel/ipc.h>

#include <u8g2-tock.h>
#include <u8g2.h>


u8g2_t u8g2;

bool   started          = false;
size_t screen_service   = -1; 
char   log_buf[32] __attribute__((aligned(32)));

const char SCREEN_SERVICE_NAME[] = "org.tockos.tutorials.attestation.screen";

static void ipc_callback(__attribute__ ((unused)) int   pid,
                         __attribute__ ((unused)) int   len,
                         __attribute__ ((unused)) int   arg2,
                         __attribute__ ((unused)) void* ud) {
  started = true;
}

void wait_for_start(void) {
  ipc_register_service_callback("org.tockos.tutorials.attestation.valid", ipc_callback,
                                NULL);
  yield_for(&started);
}

int setup_logging() {
  returncode_t ret;
  
  ret = ipc_discover(SCREEN_SERVICE_NAME, &screen_service);
  if (ret != RETURNCODE_SUCCESS) {
    printf("Encryption oracle service not found.\n");
    return ret;
  }

  ipc_share(screen_service, log_buf, 64);

  return 0;
}

int log_to_screen(const char *message) {
  returncode_t ret;

  uint16_t len = strnlen(message, sizeof(log_buf));
  memcpy(log_buf, message, len);

  ret = ipc_notify_service(screen_service);
  if (ret != RETURNCODE_SUCCESS) {
    printf("Failed to request a log to screen.\n");
    return ret;
  }
  
  return 0;
}

int main(void) {
  returncode_t ret;

  // Wait to receive the signal to start from the app selector.
  wait_for_start();

  // Set up logging service.
  setup_logging();

  // Try logging something to the screen.
  log_to_screen("Yippee!");

  while (1) {
    yield();
  }
}
