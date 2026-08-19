# HVAS BLE Integration - Ready

## BLE identity
Device name:
`ENVILIFE TSP HVAS`

ESP32-S3 acts as BLE Peripheral / GATT Server.
Mobile phone acts as BLE Central / GATT Client.

## GATT
Service:
`7f4a1000-5d6b-4e7f-9a10-000000000001`

Command characteristic:
`7f4a1000-5d6b-4e7f-9a10-000000000002`
- WRITE / WRITE_NR
- Mobile -> ESP32

Response characteristic:
`7f4a1000-5d6b-4e7f-9a10-000000000003`
- NOTIFY / READ
- ESP32 -> Mobile

## Communication flow
Mobile sends a JSON command to the command characteristic, for example:

`{"cmd":"ping"}`

ESP32 forwards that command to the existing ATmega UART protocol using `sendCommand()`.
The ATmega response is then sent back to the mobile app through the response characteristic as a BLE notification.

This preserves the existing UART protocol and CRC handling in `communication.cpp`.

## First test
1. Flash this project.
2. Open Serial Monitor at 115200.
3. Look for:
   - `[BLE] READY`
   - `[BLE] NAME : ENVILIFE TSP HVAS`
   - `[BLE] Advertising started`
4. In a BLE/GATT scanner, scan for `ENVILIFE TSP HVAS`.
5. Connect.
6. Enable notifications on the Response characteristic.
7. Write this JSON to Command:
   `{"cmd":"ping"}`
8. The ESP32 should forward it to the ATmega and notify the response.

## Important
This is the first integration/POC layer. The UUIDs are now fixed in firmware; the mobile app should use exactly the same UUIDs.

BLE payload size is intentionally limited to 511 bytes in this bridge. If larger telemetry messages are required, agree on MTU/fragmentation with the mobile developer before expanding the protocol.

The existing UART link remains:
115200, 8N1, JSON line protocol, CRC16 Modbus as implemented by `communication.cpp`.
