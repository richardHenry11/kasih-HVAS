HVAS SYSTEM PAGE PATCH

Replace/add:
  screen_system.cpp
  ui_screens.h

IMPORTANT:
  Delete the old screen_system.c from the project if it exists.
  Do NOT keep both screen_system.c and screen_system.cpp, otherwise
  the linker can report duplicate definitions.

The patch provides:
- GUI layout matching the supplied System design.
- Date & Time popup with numeric keypad and real set_rtc command.
- SD card live status.
- WiFi status + SSID/password connection popup.
- Device ID and Firmware live values from get_system.
- About popup.
- Reboot confirmation and real ESP restart.
- Shutdown information (true power-off is not available in software).
- Brightness popup/slider, but the current board configuration uses
  CH422G SWITCH_EXPANDER backlight mode, which is ON/OFF only. The
  percentage therefore updates the UI setting but does not claim to
  physically dim the panel.
