# stus-ve.direct
Smart Tiny Universal Sensor for Victron Energy VE.Direct devices

## Purpose
My primary goal here was to extract telemetry from several Victron Energy devices used in my DIY RV solar installation.
It was tested with two MPPT charge controller, a Smart Shunt and an Inverter. Other Victron device PIDs can be added in the [VEDirectManager.cpp](src/VEDirectManager.cpp) section:
```
const std::set<uint16_t> PIDS_MPPT = {0XA057, 0XA055}; //
const std::set<uint16_t> PIDS_INV = {0xA2FA}; //
const std::set<uint16_t> PIDS_BATT = {0XA389}; //
const std::set<uint16_t> PIDS_WITH_SUPPLEMENTALS = {0XA389}; //
```

[VE.Direct protocol documentation](https://www.victronenergy.com/upload/documents/VE.Direct-Protocol-3.33.pdf)

## Radio messages

Data is transmitted with nrf24l01 modules using pre-defined 32byte messages.
The message format is defined in https://github.com/jaisor/stus-rf24-commons

## Temperature sensor

I wanted to monitor the chassis temperature of the devices in case they start overheating in the relatively small space in the RV trailer. The software is capable of using several different sensors, see the TEMP_SENSOR section in [Configuration.h](src/Configuration.h) for supported hardware and pins. 

By default I'm using DS18B20 because they are available cheap, with a long cable, in a metal enclosure, easily attached to metal chassis with copper tape and with a good temperature range (-67°F to +257°F)