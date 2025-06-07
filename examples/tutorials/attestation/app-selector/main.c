#include <stdio.h>
#include <stdlib.h>

#include <libtock/interface/button.h>
#include <libtock/kernel/ipc.h>

#include <u8g2-tock.h>
#include <u8g2.h>

#include <mui.h>
#include <mui_u8g2.h>

#define NO_SELECTED_APP            0
#define VALID_ENCRYPTION_APP       1
#define COMPROMISED_ENCRYPTION_APP 2

const char VALID_ENCRYPTION_SERVICE[]       = "org.tockos.tutorials.attestation.valid";
const char COMPROMISED_ENCRYPTION_SERVICE[] = "org.tockos.tutorials.attestation.compromised";

u8g2_t u8g2;
mui_t ui;
bool action              = false;
uint8_t app_choice       = 0;
size_t app_start_service = -1;

static uint8_t mui_hrule(mui_t* mui, uint8_t msg) {
  switch (msg) {
    case MUIF_MSG_DRAW:
      u8g2_DrawHLine(&u8g2, 0, mui_get_y(mui), u8g2_GetDisplayWidth(&u8g2));
      break;
  }
  return 0;
}

muif_t muif_list[] = {
  MUIF_U8G2_LABEL(),
  MUIF_U8G2_FONT_STYLE(0, u8g2_font_helvB08_tr),
  MUIF_U8G2_FONT_STYLE(1, u8g2_font_helvR08_tr),

  MUIF_RO("HR", mui_hrule),
  MUIF_VARIABLE("RB", &app_choice, mui_u8g2_u8_radio_wm_pi),
  MUIF_VARIABLE("ST", NULL, mui_u8g2_btn_exit_wm_fi),
};

fds_t* fds =
  MUI_FORM(1)
  MUI_STYLE(0)
  MUI_LABEL(8, 10, "Tock Attestation Demo")
  MUI_XY("HR", 0, 12)
  MUI_STYLE(1)
  MUI_LABEL(5, 25, "App: ")
  MUI_XYAT("RB", 30, 25, 1, "Valid")
  MUI_XYAT("RB", 30, 40, 2, "Compromised")
  MUI_XYT("ST", 63, 58, " Start ")
;

static void button_callback(
  __attribute__ ((unused)) returncode_t ret,
  int                                   btn_num,
  bool                                  val) {

  // Hnadle button presses, routing them to the menu UI library.
  if (val) {
    if (btn_num == 0) {
      mui_PrevField(&ui);
    } else if (btn_num == 2) {
      mui_NextField(&ui);
    } else if (btn_num == 3) {
      mui_SendSelect(&ui);
    }
    action = true;
  }
}

int enable_interrupts(void) {
  returncode_t ret;

  // Enable interrupts on each button.
  int count;
  ret = libtock_button_count(&count);
  if (ret != RETURNCODE_SUCCESS) {
    printf("Unable to fetch button count.\n");
    return ret;
  }

  // Register a shared callback for every button.
  for (int i = 0; i < count; i++) {
    libtock_button_notify_on_press(i, button_callback);
  }

  return 0;
}

int prompt_for_app_choice(void) {
  returncode_t ret;

  // Initialize the menu.
  ret = u8g2_tock_init(&u8g2);
  if (ret < 0) {
    printf("Failed to initialize graphics library.\n");
    return ret;
  }

  u8g2_ClearBuffer(&u8g2);
  u8x8_InitDisplay(u8g2_GetU8x8(&u8g2));
  u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);

  mui_Init(&ui, &u8g2, fds, muif_list, sizeof(muif_list) / sizeof(muif_t));
  mui_GotoForm(&ui, 1, 0);

  // Redraw on each button press until a selection is made.
  while (mui_IsFormActive(&ui)) {
    u8g2_FirstPage(&u8g2);
    do
    {
      mui_Draw(&ui);
    } while (u8g2_NextPage(&u8g2));

    action = false;
    yield_for(&action);
  }

  return 0;
}

int start_encryption_app(const char* service) {
  returncode_t ret;

  // Discover the provided encryption app IPC service.
  ret = ipc_discover(service, &app_start_service);
  if (ret != RETURNCODE_SUCCESS) {
    printf("Encryption oracle service not found.\n");
    return ret;
  }

  // Notify it so the correct encryption app can start.
  ret = ipc_notify_service(app_start_service);
  if (ret != RETURNCODE_SUCCESS) {
    printf("Encryption oracle service unable to be notified.\n");
    return ret;
  }

  return 0;
}


int main(void) {
  returncode_t ret;

  // Enable interrupts so button presses register.
  ret = enable_interrupts();
  if (ret != RETURNCODE_SUCCESS) return ret;

  // Prompt the user for an app until they select one.
  while (app_choice == NO_SELECTED_APP) {
    ret = prompt_for_app_choice();
    if (ret != RETURNCODE_SUCCESS) return ret;

    // Based on the app choice, notify the proper app over IPC.
    switch (app_choice) {
      case VALID_ENCRYPTION_APP:
        printf("Starting valid service...");
        ret = start_encryption_app(VALID_ENCRYPTION_SERVICE);
        break;
      case COMPROMISED_ENCRYPTION_APP:
        printf("Starting compromised service...");
        ret = start_encryption_app(COMPROMISED_ENCRYPTION_SERVICE);
        break;
    }
    if (ret != RETURNCODE_SUCCESS) return ret;
  }

  while (1) {
    yield();
  }
}
