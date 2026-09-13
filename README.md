# Pico SN74HC14N Wind Speed Test

This is a simple project to run a very simple FreeRTOS app on the pico that uses an interrupt with an SN74HC14N to work out wind speed from an anemometer.

## Cloning the project

Clone the project with FreeRTOS submodules to get the pico functionality:

```
git clone --recurse-submodules https://github.com/eremiticengineer/pico-sn74hc14n-wind-speed-test
```

If you cloned without recursing submodules:

```
git submodule update --init --recursive
```

## FreeRTOS-Kernal setup for new projects

When creating a FreeRTOS project from scratch, clone the main branch into the project. The main branch at the moment has the necessary pico functionality:

```
git init
git submodule add https://github.com/FreeRTOS/FreeRTOS-Kernel.git lib/FreeRTOS-Kernel
git submodule update --init --recursive
git add .gitmodules lib/FreeRTOS-Kernel
```

## FreeRTOSConfig.h

This file customises FreeRTOS for your project. The file:

```
include/FreeRTOSConfig.h
```

is this one from the pico-examples:

```
pico-examples/freertos/FreeRTOSConfig_examples_common.h
```

## References

[Task priorites](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/01-Tasks-and-co-routines/03-Task-priorities)
[uxTaskGetStackHighWaterMark](https://www.freertos.org/Documentation/02-Kernel/04-API-references/03-Task-utilities/04-uxTaskGetStackHighWaterMark)

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
