# HVAS BLE Integration - Final Protocol

## BLE identity
- Device: `ENVILIFE TSP HVAS`
- ESP32-S3: BLE Peripheral / GATT Server
- Android: BLE Central / GATT Client

## UUID
- Service: `7f4a1000-5d6b-4e7f-9a10-000000000001`
- Command WRITE: `7f4a1000-5d6b-4e7f-9a10-000000000002`
- Response/Notify: `7f4a1000-5d6b-4e7f-9a10-000000000003`

## Commands
- Start sampling + automatic realtime stream: `{"cmd":"start_sampling"}`
- Pause sampling: `{"cmd":"pause_sampling"}`
- Stop sampling + stop continuous stream: `{"cmd":"stop_sampling"}`
- Start stream explicitly: `{"cmd":"start_stream","interval":1000}`
- Stop stream: `{"cmd":"stop_stream"}`
- Print only: `{"cmd":"print_only"}`
- Save only: `{"cmd":"save_only"}`
- Print and save: `{"cmd":"print_and_save"}`
- Manual GET commands remain available: `get_system`, `get_rtc`, `get_bme`, `get_voltage`, `get_gps`, `get_pzem`.

## Important notification rule
The mobile app must subscribe to the Response/Notify characteristic (`...0003`).
WRITE responses and telemetry are separate notification packets.
Do not parse every packet as telemetry.

## START flow
1. Android writes `{"cmd":"start_sampling"}`.
2. ESP32 starts the same sampling controller used by the LCD.
3. ESP32 enables realtime stream at 1000 ms.
4. ESP32 sends the command ACK first.
5. ESP32 immediately sends the first `type=telemetry` packet.
6. ESP32 continues telemetry every interval.

## Telemetry packet
`type=telemetry` contains:
- sampling state, elapsed time, remaining time
- system status + uptime
- RTC
- BME280
- DC voltage
- GPS
- PZEM

The telemetry packet is kept below 511 bytes. Static system metadata (`device`, `version`, `power_on_count`) is sent separately as `type=system` every 5 seconds.

Example telemetry:
```json
{"type":"telemetry","sampling":{"state":"running","elapsed_time":"00:00:01","remaining_time":"23:59:59"},"system":{"status":"ok","uptime_ms":123456},"rtc":{"connected":true,"datetime":"2000-01-01 00:21:43","temperature":29.75},"bme":{"connected":true,"temperature":27.3,"humidity":46.9,"pressure":941.68},"voltage":{"connected":true,"value":17.96},"gps":{"connected":true,"fix":false,"latitude":0,"longitude":0,"altitude":0,"speed_kmh":0,"satellites":0},"pzem":{"connected":false,"voltage":0,"current":0,"power":0,"energy":0,"frequency":0,"power_factor":0}}
```

## System metadata packet
```json
{"type":"system","status":"ok","device":"atmega2560 pheripheral Co-Processor","version":"v1.2.8","uptime_ms":123456,"power_on_count":33}
```

## PAUSE
`pause_sampling` turns A1/pump off temporarily but keeps the BLE stream active. Telemetry continues and reports `sampling.state = paused`.

## STOP
`stop_sampling` turns A1/pump off, disables continuous stream, sends a final telemetry packet with `sampling.state = stopped`, and then sends the STOP ACK.

## Automatic finish
When the 24-hour sampling timer reaches the target, the sampling controller turns A1 off and changes state to `finished`. BLE sends a final `type=telemetry` packet, then automatically disables the stream.

## Result actions
These execute the same result-controller functions used by the LCD:
- `print_only`
- `save_only`
- `print_and_save`

The result popup itself is UI logic; Android should show its own popup after receiving the STOP/FINISHED state.

## Android parser rule
Pseudo logic:
```text
if packet.cmd != null:
    handle command ACK
else if packet.type == "telemetry":
    update sampling + sensor UI
else if packet.type == "system":
    update system metadata UI
```

Do NOT expect sensor values inside the `start_sampling` ACK.
