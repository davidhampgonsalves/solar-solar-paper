next: espidf with ulc example

if wake up from ULC
  <!-- check cap voltage -->
  turn on voltage regulator
  beep?
  <!-- if > 3v print -->
deep sleep

ULC
  check voltage every few minutes and if above 2.5v wake up


circuit -
solar -> shotsky diode -> super cap (with 3.3v zenier) -> esp32 3.3v pin (bypasses LDO vreg)
supercap -> transitor -> dc->dc -> esp32 3.3v
supercap -> esp32 analog pin for power monitoring

todos:
* talk about how good https://github.com/bitbank2/JPEGDEC is.
* talk about dithering, inverting space color.
* solar power -> supercapicitor -> https -> jpg decoding -> dithering -> grey levels -> invert space -> display -> deep sleep
* E-Paper limitations 4 grey levels (talk about ben kravnov getting 8 levels)



Great video as always, thank you. I use another method which works in C without programming the ULP. There is a wake up hook function when the ESP32 wakes up. It gets called before booting the ESP32, before the flash is activated.

C functions can be placed into the RTC memory for little processing. This function can continue to boot the ESP32 (which takes 0.3 secs) or put it into deeplseep again. This hook requires only 3 ms to check some processing. The advantage is can it can be programmed in C, the disadvantage is  that only low-level IO can be done because program functions (FLASH) are not available. However all registers and ESP32 ROM functions can be used, or own functions marked with IRAM_ATTR will also work.

This is pretty much low level like the ULP, however programmed in C with proper print debugging, etc.

RTC_DATA_ATTR int wake_count;

void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
  if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_ULP)  esp_default_wake_deep_sleep();
}


ESP-32 logic:

power on -> docs/deep-sleep-stub.rst
    if not a wakeup -> go directly to sleep so we don't brown out
    if wakeup
        start ULP (or get fancier and check voltage) and deep sleep

ULP


Next charge supercap, use it to power esp-3


connect SC to solar + shotski + esp-32
set enable pin to low
when 2.5v zenier -> npm to enable pin
when esp-32 turns on it latches npn pin on till its done its work.

charge super caps to 5v
at 5v the voltage supervisor turns on the esp-32

just try the simpliest first

supercaps charged to 3.6v