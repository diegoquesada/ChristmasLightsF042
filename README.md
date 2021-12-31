# ChristmasLightsF042
Drives Christmas lights via STM32F042 and a Grove digital light sensor.

![Component diagram](https://github.com/diegoquesada/ChristmasLightsF042/blob/main/Docs/diagram.png?raw=true)

Components:
- [STM32F042K6 Nucleo board](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-nucleo-boards/nucleo-f042k6.html)
- [Grove Digital Light Sensor](https://wiki.seeedstudio.com/Grove-Digital_Light_Sensor/)
- [Grove 2-channel SPDT relay](https://wiki.seeedstudio.com/Grove-2-Channel_SPDT_Relay/)
- Buttons
- LED

The relay is potentially active between the hours of 8am and 11pm, but only turned on if light
measured by the TSL2561 is below a threshold.

Two buttons allow the user to configure RTC time. BTN1 to enter time configuration mode, then
BTN2 to increment the hours component. BTN1 saves hours and allows BTN2 to increment the minutes.
Finally BTN1 saves the minutes. Three flashes of the LED confirm time has been set.