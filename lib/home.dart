import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:hvas/main.dart';
import 'package:path_provider/path_provider.dart';
import 'dart:io';
import 'package:permission_handler/permission_handler.dart';
import 'package:intl/intl.dart';
import 'dart:math';
import 'package:geolocator/geolocator.dart';

class DashboardApp extends StatelessWidget {
  final BluetoothDevice? device;

  const DashboardApp({super.key, this.device});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Fitur(device: device),
    );
  }
}

class Fitur extends StatefulWidget {
  final BluetoothDevice? device;

  const Fitur({super.key, this.device});

  @override
  State<Fitur> createState() => _FiturState();
}

class _FiturState extends State<Fitur> {
  BluetoothAdapterState _btState = BluetoothAdapterState.unknown;
  BluetoothCharacteristic? txCharacteristic;
  BluetoothCharacteristic? rxCharacteristic;
  final TextEditingController _controller = TextEditingController();
  List<String> messages = [];
  BluetoothDevice? selectedDevice;
  StreamSubscription<List<int>>? _notifySubscription;
  bool isListening = false;
  final random = Random();
  String humidity = "";
  String temperature = "";
  String airFlow = "";
  String pressure = "";
  String windSpeed = "";
  String windDirection = "";
  String clock = "";
  String setar = "";
  String latitude = "";
  String longitude = "";
  final TextEditingController latController = TextEditingController();
  final TextEditingController lngController = TextEditingController();

  // bool isRunning = false;
  // Timer? dummyTimer;

  late Stream<DateTime> _timeStream;
  late StreamSubscription<DateTime> _timeSubscription;
  String _currentTime = "";

  // realtime date Vars
  String currentDateTime =
      DateFormat('yyyy-MM-dd – kk:mm:ss').format(DateTime.now());

  // parameter vars
  final TextEditingController humidityController = TextEditingController();
  final TextEditingController temperatureController = TextEditingController();
  final TextEditingController airFlowController = TextEditingController();
  final TextEditingController pressureController = TextEditingController();
  final TextEditingController windSpeedController = TextEditingController();
  final TextEditingController windDirectionController = TextEditingController();
  final TextEditingController setarController = TextEditingController();
  final TextEditingController intervalController = TextEditingController();
  // final TextEditingController _timerController = TextEditingController();
  Timer? _timer;
  Timer? _bmeTimer;

  //Button State
  bool isActivated = true;
  bool isDeactivateButtonEnabled = false;

  //Interval timer autosave variable
  Duration autoSaveDuration = Duration(minutes: 1);

  // Timer Start Sample
  final TextEditingController _sampController = TextEditingController();
  String startSamp = "";

  // Dropdown List
  List<String> saveItems = [];
  String _buffer = '';

  // Selected Dropdown
  String? _selectedItem;

  @override
  void initState() {
    super.initState();
    //Ask Permits
    _requestPermissions();

    // setState(() {
    //   _geData("GETDATA");

    // });

    _discoverServices();

    FlutterBluePlus.adapterState.listen((BluetoothAdapterState state) {
      if (mounted) {
        setState(() {
          _btState = state;
        });
      }
    });
    _timeStream = Stream<DateTime>.periodic(
        const Duration(seconds: 1), (_) => DateTime.now());
    _timeSubscription = _timeStream.listen((time) {
      setState(() {
        _currentTime =
            "${time.hour.toString().padLeft(2, '0')}:${time.minute.toString().padLeft(2, '0')}:${time.second.toString().padLeft(2, '0')}";
      });
    });
    _checkConnectionStatus();

    // autosave timer
    // _startAutoSaving();
  }

//   void randomVal() {
//   if (_timer != null && _timer!.isActive) return; // cegah timer dobel

//   isRunning = true;
//   _timer = Timer.periodic(const Duration(seconds: 1), (_) {
//     if (!isRunning) return; // jangan update kalau sudah OFF

//     setState(() {
//       temperature = (20 + random.nextDouble() * 10).toStringAsFixed(1); // 20–30 °C
//         humidity = (40 + random.nextDouble() * 30).toStringAsFixed(1);    // 40–70 %RH
//         pressure = (990 + random.nextDouble() * 20).toStringAsFixed(1);   // 990–1010 hPa
//         airFlow = (10 + random.nextDouble() * 40).toStringAsFixed(1);    // 1000.0 – 1020.0
//     });
//   });
// }

// void randomValNull() {
//   isRunning = false;
//   _timer?.cancel();
//   _timer = null;
// }

  void _startAutoSave(int minutes) {
    _timer?.cancel(); // Cancel previous timer if exists
    _timer = Timer.periodic(Duration(minutes: minutes), (timer) {
      // Format autosave log sama dengan manual save log
      String currentDateTime =
          DateFormat('yyyy-MM-dd – kk:mm:ss').format(DateTime.now());
      String autoSaveLog = "==============autosave Logs==================\n\n"
          "Start Samp: $currentDateTime\n"
          "Start: $setar \n"
          "================================\n"
          "Humidity: $humidity %RH,\n"
          "Temperature: $temperature °C,\n"
          "Air Flow: $airFlow L/min,\n"
          "Pressure: $pressure hPa\n\n"
          "================================\n";

      _autoSaveLogToFile(autoSaveLog);
      print(_controller);
    });
  }

  void _stopAutoSave() {
    if (_timer != null) {
      _timer!.cancel();
      _timer = null;
      setState(() {
        isDeactivateButtonEnabled = false; // Disable the stop button
        isActivated = true; // Enable the start button
      });
      _showSnackbar("AutoSave stop");
      print("Auto-save stopped.");
    }
  }

  Future<void> _checkConnectionStatus() async {
    if (widget.device == null) return;
    while (true) {
      await Future.delayed(const Duration(seconds: 5)); // Check every 5 seconds
      if (widget.device!.isConnected) {
        print("Connected");
      } else {
        print('Disconnected');
        disconnected();
        break;
      }
    }
  }

  void _processDataFile(Uint8List data) {
    _processData(data);
  }

  void disconnected() {
    if (!mounted) return;
    showDialog(
        context: context,
        builder: (context) {
          return AlertDialog(
            title: const Column(
              children: [
                Text("Disconnected"),
                Icon(
                  Icons.close_rounded,
                  size: 40,
                  color: Colors.red,
                )
              ],
            ),
            actions: [
              ElevatedButton(
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
                  onPressed: () {
                    Navigator.push(context,
                        MaterialPageRoute(builder: (context) => HomeScreen()));
                  },
                  child: const Text(
                    "OK",
                    style: TextStyle(color: Colors.white),
                  ))
            ],
          );
        });
  }

  Future<Directory> _getDownloadDirectory() async {
    Directory? directory = await getExternalStorageDirectory();
    if (directory != null) {
      final downloadDir = Directory('${directory.path}/Download');
      if (await downloadDir.exists()) {
        return downloadDir;
      } else {
        return directory;
      }
    } else {
      return getApplicationDocumentsDirectory();
    }
  }

  void _showSnackbar(String message) {
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(
      content: Center(child: Text(message)),
      duration: const Duration(seconds: 3),
    ));
  }

  Future<void> _discoverServices() async {
    if (widget.device == null) return;
    try {
      if (Platform.isAndroid) {
        try {
          await widget.device!.requestMtu(512);
        } catch (e) {
          print("MTU request failed: $e");
        }
      }
      List<BluetoothService> services = await widget.device!.discoverServices();
      for (BluetoothService service in services) {
        for (BluetoothCharacteristic characteristic
            in service.characteristics) {
          if (characteristic.properties.notify ||
              characteristic.properties.indicate) {
            rxCharacteristic = characteristic;
            await characteristic.setNotifyValue(true);
            _notifySubscription =
                characteristic.onValueReceived.listen((value) {
              if (value.isNotEmpty) {
                messages.add("Device: ${String.fromCharCodes(value)}");
                _processData(Uint8List.fromList(value));
              }
            });
            isListening = true;
          }
          if (characteristic.properties.write ||
              characteristic.properties.writeWithoutResponse) {
            txCharacteristic = characteristic;
          }
        }
      }
    } catch (e) {
      print("Error discovering services: $e");
    }
  }

  @override
  void dispose() {
    _timer?.cancel();
    _bmeTimer?.cancel();
    _notifySubscription?.cancel();
    _timeSubscription.cancel();
    widget.device?.disconnect();
    // _stopAutoSave();
    super.dispose();
  }

  void _disconnected() async {
    await widget.device?.disconnect();
    setState(() {
      isListening = false;
    });
    Navigator.pushAndRemoveUntil(
      context,
      MaterialPageRoute(builder: (context) => MyApp()),
      (Route<dynamic> route) => false,
    );
    print('Disconnected From Device');
  }

  void _activateActive() {
    isActivated = true;
  }

  void _disabledButton() {
    isDeactivateButtonEnabled = true;
  }

  // Button Function
  void _activate(String text) async {
    // 1. Initial gets
    List<String> getCmds = [
      '{"cmd":"get_bme"}\n',
      '{"cmd":"get_rtc"}\n',
      '{"cmd":"get_voltage"}\n',
      '{"cmd":"get_pzem"}\n',
      '{"cmd":"get_system"}\n'
    ];

    for (String cmd in getCmds) {
      txCharacteristic?.write(utf8.encode(cmd));
      setState(() => messages.add(cmd.trim()));
      await Future.delayed(const Duration(milliseconds: 200));
    }

    // 2. Start stream
    String cmdStream = '{"cmd":"start_stream","interval":1000}\n';
    txCharacteristic?.write(utf8.encode(cmdStream));
    setState(() => messages.add(cmdStream.trim()));
    await Future.delayed(const Duration(milliseconds: 200));

    // 3. Start sampling
    String cmdSample;
    String rawInput = _sampController.text.trim();
    print('DEBUG START SAMPLING: Raw Input dari Textfield = "$rawInput"');
    
    int? durationMinutes = int.tryParse(rawInput);
    if (durationMinutes != null && durationMinutes > 0) {
      int durationSeconds = durationMinutes * 60;
      print('DEBUG START SAMPLING: Berhasil dikonversi! $durationMinutes menit -> $durationSeconds detik');
      cmdSample = '{"cmd":"start_sampling","duration_seconds":$durationSeconds}\n';
    } else {
      print('DEBUG START SAMPLING: Input kosong atau bukan angka > 0. Mengirim tanpa durasi.');
      cmdSample = '{"cmd":"start_sampling"}\n';
    }
    
    print('DEBUG START SAMPLING: Final JSON yang dikirim = $cmdSample');
    
    txCharacteristic?.write(utf8.encode(cmdSample));
    setState(() {
      messages.add(cmdSample.trim());
      isActivated = false;
    });

    _controller.clear();
    _showSnackbar('Starting...');
    print('starting...');
    print(messages);
  }

  void _showPostStopDialog() {
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text('Data Action'),
          content: const Text('What would you like to do with the data?'),
          actions: [
            TextButton(
              onPressed: () {
                Navigator.of(context).pop();
                String cmd = '{"cmd":"print_only"}\n';
                txCharacteristic?.write(utf8.encode(cmd));
                setState(() => messages.add(cmd.trim()));
              },
              child: const Text('PRINT', style: TextStyle(color: Colors.blue)),
            ),
            TextButton(
              onPressed: () {
                Navigator.of(context).pop();
                String cmd = '{"cmd":"save_only"}\n';
                txCharacteristic?.write(utf8.encode(cmd));
                setState(() => messages.add(cmd.trim()));
              },
              child: const Text('SAVE', style: TextStyle(color: Colors.green)),
            ),
            TextButton(
              onPressed: () {
                Navigator.of(context).pop();
                String cmd = '{"cmd":"print_and_save"}\n';
                txCharacteristic?.write(utf8.encode(cmd));
                setState(() => messages.add(cmd.trim()));
              },
              child: const Text('PRINT & SAVE',
                  style: TextStyle(color: Colors.orange)),
            ),
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: const Text('CANCEL', style: TextStyle(color: Colors.grey)),
            ),
          ],
        );
      },
    );
  }

  void _deactivate(String text) {
    String cmd = '{"cmd":"stop_sampling"}\n';
    txCharacteristic?.write(utf8.encode(cmd));

    setState(() {
      messages.add(cmd.trim());
      _stopAutoSave();
    });
    _showSnackbar('Stop Sampling (Stream Stopped)...');
    _controller.clear();
    print('stopping');
    print(messages);

    // Show popup
    _showPostStopDialog();
  }

  void _pauseSamp(String text) {
    String cmd = '{"cmd":"pause_sampling"}\n';
    txCharacteristic?.write(utf8.encode(cmd));
    setState(() {
      messages.add(cmd.trim());
      isActivated = true;
    });
    _controller.clear();
    print('pause sampling');
    print(messages);
  }

  void _geData(String text) {
    String cmd = '{"cmd":"get_printer"}\n';
    txCharacteristic?.write(utf8.encode(cmd));
    setState(() {
      messages.add(cmd.trim());
    });
    _controller.clear();
    print('getting Data Print');
    print(messages);
  }

  void _getRealtime(String text) {
    String cmd = '{"cmd":"get_bme"}\\n';
    txCharacteristic?.write(utf8.encode(cmd));
    setState(() {
      messages.add(cmd.trim());
    });
    _controller.clear();
    print('getting realtime sensor (get_bme)');
    print(messages);
  }

  void _stopStream() {
    String cmd = '{"cmd":"stop_stream"}\\n';
    txCharacteristic?.write(utf8.encode(cmd));
    setState(() {
      messages.add(cmd.trim());
    });
    _controller.clear();
    print('stopping realtime stream');
    print(messages);
  }

  void _syncTimeWithPhone() {
    String nowFormatted =
        DateFormat('yyyy-MM-dd HH:mm:ss').format(DateTime.now());
    String cmd = '{"cmd":"set_rtc","datetime":"$nowFormatted"}\n';
    txCharacteristic?.write(utf8.encode(cmd));
    setState(() {
      messages.add(cmd.trim());
    });
    _showSnackbar("Waktu device disinkronkan: $nowFormatted");
    print('Syncing RTC time to device: $nowFormatted');
  }

  void _getGps() async {
    String cmd = '{"cmd":"get_gps"}\n';
    txCharacteristic?.write(utf8.encode(cmd));
    setState(() {
      messages.add(cmd.trim());
    });
    _controller.clear();
    print('getting gps');
    print(messages);

    try {
      Position position = await Geolocator.getCurrentPosition(
          desiredAccuracy: LocationAccuracy.high);
      setState(() {
        latitude = position.latitude.toString();
        longitude = position.longitude.toString();
        latController.text = latitude;
        lngController.text = longitude;
      });
      _showSnackbar("Phone GPS retrieved");
    } catch (e) {
      print('Error getting GPS: $e');
      _showSnackbar("Failed to get phone GPS");
    }
  }

  void _print(String text) {
    String cmd =
        '{"cmd":"print_text","text":"HVAS Data Print\n","align":"left","bold":false,"size":1}\n';
    txCharacteristic?.write(utf8.encode(cmd));
    setState(() {
      messages.add("Print Request");
    });
    _showSnackbar('Printing...');
    _controller.clear();
    print('Printing...');
    print(messages);
  }

  void _updateSaveItems(List<String> newItems) {
    setState(() {
      saveItems = newItems;
    });
  }

  void _onPrintButtonPressed(String command) {
    setState(() {
      if (_selectedItem != null) {
        int selectedIndex = saveItems.indexOf(_selectedItem!);

        // Print text using the new JSON command protocol
        String textToPrint = "HVAS File Index $selectedIndex\n";
        String cmd = '{"cmd":"print_text","text":"' +
            textToPrint +
            '","align":"left","bold":false,"size":1}\\n';

        txCharacteristic?.write(utf8.encode(cmd));

        setState(() {
          messages.add('{"cmd":"print_text"...}');
        });

        _showSnackbar("Printing file at index $selectedIndex");
      } else {
        _showSnackbar("Please select a file to print.");
      }
    });
  }

  // Buffer for incomplete JSON packets
  String _jsonBuffer = "";

  void _processData(Uint8List data) {
    try {
      String dataString = String.fromCharCodes(data);
      _jsonBuffer += dataString;

      // Extract sensor values using regex as a fallback for truncated packets from the device
      _tryExtractSensorValues(_jsonBuffer);

      while (true) {
        int startIndex = _jsonBuffer.indexOf('{');
        if (startIndex == -1) {
          _jsonBuffer = ""; // Discard junk
          break;
        }

        if (startIndex > 0) {
          _jsonBuffer = _jsonBuffer.substring(startIndex);
          startIndex = 0;
        }

        bool parsedSuccessfully = false;
        int currentStart = 0;

        while (currentStart != -1 && currentStart < _jsonBuffer.length) {
          bool foundForThisStart = false;
          int currentEnd = _jsonBuffer.indexOf('}', currentStart);

          while (currentEnd != -1) {
            String candidate = _jsonBuffer.substring(currentStart, currentEnd + 1);
            try {
              Map<String, dynamic> jsonData = jsonDecode(candidate);
              print('Parsed JSON: $jsonData');

              // Debug print untuk hasil get_rtc
              if (jsonData.containsKey('time') ||
                  jsonData.containsKey('date') ||
                  jsonData.containsKey('rtc')) {
                print('==== HASIL GET_RTC: $jsonData ====');
              }

              _handleParsedData(jsonData);

              _jsonBuffer = _jsonBuffer.substring(currentEnd + 1);
              parsedSuccessfully = true;
              foundForThisStart = true;
              break;
            } catch (e) {
              currentEnd = _jsonBuffer.indexOf('}', currentEnd + 1);
            }
          }

          if (foundForThisStart) break;
          currentStart = _jsonBuffer.indexOf('{', currentStart + 1);
        }

        if (!parsedSuccessfully) {
          // If buffer has multiple start braces and is getting large, the previous ones are likely truncated dead packets
          int lastBrace = _jsonBuffer.lastIndexOf('{');
          if (lastBrace > 0 && _jsonBuffer.length > 512) {
            _jsonBuffer = _jsonBuffer.substring(lastBrace);
          } else if (_jsonBuffer.length > 4096) {
            _jsonBuffer = "";
          }
          break; // Wait for more data
        }
      }
    } catch (e) {
      print('Error parsing data: $e');
    }
  }

  void _tryExtractSensorValues(String data) {
    bool updated = false;
    
    try {
      Iterable<RegExpMatch> tempMatches = RegExp(r'"(?:temp|temperature)"\s*:\s*([\d\.]+)').allMatches(data);
      if (tempMatches.isNotEmpty) {
        temperature = tempMatches.last.group(1)!;
        temperatureController.text = temperature;
        updated = true;
      }
      
      Iterable<RegExpMatch> humMatches = RegExp(r'"(?:hum|humidity)"\s*:\s*([\d\.]+)').allMatches(data);
      if (humMatches.isNotEmpty) {
        humidity = humMatches.last.group(1)!;
        humidityController.text = humidity;
        updated = true;
      }
      
      Iterable<RegExpMatch> pressMatches = RegExp(r'"(?:press|pressure)"\s*:\s*([\d\.]+)').allMatches(data);
      if (pressMatches.isNotEmpty) {
        pressure = pressMatches.last.group(1)!;
        pressureController.text = pressure;
        updated = true;
      }
      
      Iterable<RegExpMatch> flowMatches = RegExp(r'"flow"\s*:\s*([\d\.]+)').allMatches(data);
      if (flowMatches.isNotEmpty) {
        airFlow = flowMatches.last.group(1)!;
        airFlowController.text = airFlow;
        updated = true;
      }
      
      Iterable<RegExpMatch> windSpeedMatches = RegExp(r'"wind_speed_ms"\s*:\s*([\d\.]+)').allMatches(data);
      if (windSpeedMatches.isNotEmpty) {
        windSpeed = windSpeedMatches.last.group(1)!;
        windSpeedController.text = windSpeed;
        updated = true;
      }
      
      Iterable<RegExpMatch> windDirMatches = RegExp(r'"wind_direction_deg"\s*:\s*([\d\.]+)').allMatches(data);
      if (windDirMatches.isNotEmpty) {
        windDirection = windDirMatches.last.group(1)!;
        windDirectionController.text = windDirection;
        updated = true;
      }

      if (updated) {
        setState(() {});
      }
    } catch (e) {
      print('Regex extraction error: $e');
    }
  }

  void _handleParsedData(Map<String, dynamic> jsonData) {
    setState(() {
      // 1. Direct root keys (temp, hum, press, flow, temperature, humidity, pressure)
      if (jsonData.containsKey('temp') && jsonData['temp'] != null) {
        temperature = jsonData['temp'].toString();
        temperatureController.text = temperature;
      }
      if (jsonData.containsKey('temperature') &&
          jsonData['temperature'] != null) {
        temperature = jsonData['temperature'].toString();
        temperatureController.text = temperature;
      }
      if (jsonData.containsKey('hum') && jsonData['hum'] != null) {
        humidity = jsonData['hum'].toString();
        humidityController.text = humidity;
      }
      if (jsonData.containsKey('humidity') &&
          jsonData['humidity'] != null) {
        humidity = jsonData['humidity'].toString();
        humidityController.text = humidity;
      }
      if (jsonData.containsKey('press') && jsonData['press'] != null) {
        pressure = jsonData['press'].toString();
        pressureController.text = pressure;
      }
      if (jsonData.containsKey('pressure') &&
          jsonData['pressure'] != null) {
        pressure = jsonData['pressure'].toString();
        pressureController.text = pressure;
      }
      if (jsonData.containsKey('flow') && jsonData['flow'] != null) {
        airFlow = jsonData['flow'].toString();
        airFlowController.text = airFlow;
      }
      if (jsonData.containsKey('wind_speed_ms') && jsonData['wind_speed_ms'] != null) {
        windSpeed = jsonData['wind_speed_ms'].toString();
        windSpeedController.text = windSpeed;
      }
      if (jsonData.containsKey('wind_direction_deg') && jsonData['wind_direction_deg'] != null) {
        windDirection = jsonData['wind_direction_deg'].toString();
        windDirectionController.text = windDirection;
      }

      // 2. Nested BME object (Telemetry / Sub-objects)
      if (jsonData.containsKey('bme') && jsonData['bme'] is Map) {
        var bme = jsonData['bme'];
        if (bme['temperature'] != null) {
          temperature = bme['temperature'].toString();
          temperatureController.text = temperature;
        } else if (bme['temp'] != null) {
          temperature = bme['temp'].toString();
          temperatureController.text = temperature;
        }

        if (bme['humidity'] != null) {
          humidity = bme['humidity'].toString();
          humidityController.text = humidity;
        } else if (bme['hum'] != null) {
          humidity = bme['hum'].toString();
          humidityController.text = humidity;
        }

        if (bme['pressure'] != null) {
          pressure = bme['pressure'].toString();
          pressureController.text = pressure;
        } else if (bme['press'] != null) {
          pressure = bme['press'].toString();
          pressureController.text = pressure;
        }
      }

      // 3. Nested sensors object (Legacy format)
      if (jsonData.containsKey('sensors') && jsonData['sensors'] is Map) {
        var s = jsonData['sensors'];
        if (s['temperature'] != null) {
          temperature = s['temperature'].toString();
          temperatureController.text = temperature;
        }
        if (s['humidity'] != null) {
          humidity = s['humidity'].toString();
          humidityController.text = humidity;
        }
        if (s['pressure'] != null) {
          pressure = s['pressure'].toString();
          pressureController.text = pressure;
        }
        if (s['flow'] != null) {
          airFlow = s['flow'].toString();
          airFlowController.text = airFlow;
        }
        if (s['START'] != null) setar = s['START'].toString();
      }

      // 4. File items for dropdown
      if (jsonData.containsKey('file') && jsonData['file'] != null) {
        List<String> fetchedItems = List<String>.from(jsonData['file']);
        saveItems = fetchedItems;
        print('Updated saveItems: $saveItems');
      }
    });
  }

  Future<void> _requestPermissions() async {
    var status = await Permission.manageExternalStorage.status;
    Map<Permission, PermissionStatus> statuses = await [
      Permission.storage,
      Permission.manageExternalStorage,
    ].request();

    statuses.forEach((permission, status) {
      if (status != PermissionStatus.granted) {
        print('Permission not granted for: $permission');
      }
    });

    if (status.isDenied) {
      if (await Permission.manageExternalStorage.request().isGranted) {
        // Permit Granted
        print('Manage external storage permission granted');
      } else {
        // show message or send user to homepage
        openAppSettings();
      }
    } else if (status.isPermanentlyDenied) {
      // guide user to settings
      openAppSettings();
    } else {
      // permits already granted
      print('Manage external storage permission already granted');
    }
  }

  Future<void> _autoSaveLogToFile(String text) async {
    try {
      if (await Permission.manageExternalStorage.request().isGranted ||
          await Permission.storage.isGranted) {
        final directory = await _getDownloadDirectory();
        final file = File('${directory.path}/sensor_log.txt');
        await file.writeAsString(text, mode: FileMode.append);
        print('Data saved to ${file.path}');
      } else {
        print('Storage Permission not Granted!');
      }
    } catch (e) {
      _failedSavedData();
      print('Error saving data: $e');
    }
  }

  Future<void> _saveLogToFile(String text) async {
    try {
      if (await Permission.manageExternalStorage.request().isGranted ||
          await Permission.storage.isGranted) {
        final directory = await _getDownloadDirectory();
        final file = File('${directory.path}/sensor_log.txt');
        await file.writeAsString(text, mode: FileMode.append);
        print('Data saved to ${file.path}');
        _savedData();
      } else {
        print('Storage Permission not Granted!');
      }
    } catch (e) {
      _failedSavedData();
      print('Error saving data: $e');
    }
  }

  void _onSampTimer(String command) {
    setState(() {
      String timer = _sampController.text;
      startSamp = timer;
      _showSnackbar("Sampling duration set to $timer minutes");
    });
  }

//   void _onPrintButtonPressed(String text) {
//     txCharacteristic?.write(utf8.encode(text + ""));
//     setState(() {
//       if (_selectedItem != null) {
//     int selectedIndex = saveItems.indexOf(_selectedItem!);
//     if (selectedIndex != 0) {
//       // String printData = saveItems[selectedIndex];
//       // String jsonString = jsonEncode({"command": "000E", "file": prin});
//           setState(() {
//             messages.add("$selectedIndex");
//           });
//       // _print(jsonString);
//     }
//     _showSnackbar("Printing file at index $selectedIndex");
//   } else {
//     _showSnackbar("Please select a file to print.");
//   }
//     });
// }

  void _savedData() {
    showDialog(
        context: context,
        builder: (context) {
          return AlertDialog(
            title: const Column(
              children: [
                Icon(Icons.check_circle_rounded,
                    color: Colors.green, size: 40.0),
                Padding(
                  padding: EdgeInsets.all(12.0),
                  child: Text(
                    "Data Saved on local ^_^",
                    style: TextStyle(fontWeight: FontWeight.w100),
                  ),
                ),
              ],
            ),
            content: ElevatedButton(
                style: ElevatedButton.styleFrom(backgroundColor: Colors.green),
                onPressed: () {
                  //logic button
                  Navigator.of(context).pop();
                },
                child: const Text(
                  "OK",
                  style: TextStyle(color: Colors.white),
                )),
          );
        });
  }

  void _failedSavedData() {
    showDialog(
        context: context,
        builder: (context) {
          return AlertDialog(
            title: const Column(
              children: [
                Icon(Icons.close_rounded, color: Colors.red, size: 40.0),
                Padding(
                  padding: EdgeInsets.all(12.0),
                  child: Text(
                    "Failed Saving",
                    style: TextStyle(fontWeight: FontWeight.w100, fontSize: 20),
                  ),
                ),
              ],
            ),
            content: ElevatedButton(
                style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
                onPressed: () {
                  //logic button
                  Navigator.of(context).pop();
                },
                child: const Text(
                  "OK",
                  style: TextStyle(color: Colors.white),
                )),
          );
        });
  }

  void _save(String text) {
    txCharacteristic?.write(utf8.encode(text));
    setState(() {
      messages.add("000G");
    });
    _saveLogToFile(
        "================================\n\nStart Samp: $currentDateTime\nStart: $setar %RH \n================================\n Humidity: $humidity %RH,\n Temperature: $temperature C,\n Air Flow: $airFlow L/min,\n Pressure: $pressure hPa\n\n================================");
    _controller.clear();
    print('Saving...');
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFF1F5F0),
      body: SafeArea(
        child: Container(
          decoration: const BoxDecoration(
            gradient: LinearGradient(
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
              colors: [
                Color(0xFF2E5A1C), // Deep Forest Green
                Color(0xFF4C8A2C), // Lush Green
                Color(0xFFE8F5E9), // Soft Mint Surface
              ],
              stops: [0.0, 0.28, 0.45],
            ),
          ),
          child: Column(
            children: [
              // ==========================================
              // 1. MODERN TOP HEADER & REALTIME CLOCK
              // ==========================================
              Padding(
                padding:
                    const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  crossAxisAlignment: CrossAxisAlignment.center,
                  children: [
                    // Brand Logo & Title
                    Expanded(
                      child: Row(
                        children: [
                          Container(
                            width: 52,
                            height: 52,
                            decoration: BoxDecoration(
                              color: Colors.white.withOpacity(0.18),
                              borderRadius: BorderRadius.circular(14),
                              border: Border.all(
                                  color: Colors.white.withOpacity(0.35)),
                              boxShadow: [
                                BoxShadow(
                                  color: Colors.black.withOpacity(0.08),
                                  blurRadius: 8,
                                  offset: const Offset(0, 3),
                                ),
                              ],
                            ),
                            padding: const EdgeInsets.all(5),
                            child: Image.asset(
                              "img/LogoHVAS.png",
                              fit: BoxFit.contain,
                            ),
                          ),
                          const SizedBox(width: 10),
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                const Text(
                                  "HVAS SAMPLER",
                                  style: TextStyle(
                                    color: Colors.white,
                                    fontSize: 15,
                                    fontWeight: FontWeight.w800,
                                    letterSpacing: 0.5,
                                  ),
                                  maxLines: 1,
                                  overflow: TextOverflow.ellipsis,
                                ),
                                Text(
                                  "High Volume Air Sampler",
                                  style: TextStyle(
                                    color: Colors.white.withOpacity(0.85),
                                    fontSize: 11,
                                    fontWeight: FontWeight.w400,
                                  ),
                                  maxLines: 1,
                                  overflow: TextOverflow.ellipsis,
                                ),
                              ],
                            ),
                          ),
                        ],
                      ),
                    ),
                    const SizedBox(width: 8),

                    // Digital Clock & Disconnect Button
                    Column(
                      crossAxisAlignment: CrossAxisAlignment.end,
                      children: [
                        // Interactive Clock Badge with Sync
                        InkWell(
                          onTap: _syncTimeWithPhone,
                          borderRadius: BorderRadius.circular(8),
                          child: Container(
                            padding: const EdgeInsets.symmetric(
                                horizontal: 8, vertical: 3),
                            decoration: BoxDecoration(
                              color: Colors.black.withOpacity(0.25),
                              borderRadius: BorderRadius.circular(8),
                              border: Border.all(
                                  color: Colors.white.withOpacity(0.2),
                                  width: 0.8),
                            ),
                            child: Row(
                              mainAxisSize: MainAxisSize.min,
                              children: [
                                Text(
                                  _currentTime.isEmpty
                                      ? "--:--:--"
                                      : _currentTime,
                                  style: const TextStyle(
                                    color: Colors.white,
                                    fontSize: 15,
                                    fontWeight: FontWeight.w700,
                                    letterSpacing: 0.8,
                                    fontFamily: 'monospace',
                                  ),
                                ),
                                const SizedBox(width: 4),
                                const Icon(Icons.sync_rounded,
                                    size: 13, color: Colors.white70),
                              ],
                            ),
                          ),
                        ),
                        const SizedBox(height: 5),
                        InkWell(
                          onTap: _disconnected,
                          borderRadius: BorderRadius.circular(20),
                          child: Container(
                            padding: const EdgeInsets.symmetric(
                                horizontal: 10, vertical: 3.5),
                            decoration: BoxDecoration(
                              color: Colors.white,
                              borderRadius: BorderRadius.circular(20),
                              boxShadow: [
                                BoxShadow(
                                  color: Colors.black.withOpacity(0.12),
                                  blurRadius: 6,
                                  offset: const Offset(0, 2),
                                ),
                              ],
                            ),
                            child: const Row(
                              mainAxisSize: MainAxisSize.min,
                              children: [
                                Icon(Icons.bluetooth_disabled_rounded,
                                    size: 12, color: Colors.redAccent),
                                SizedBox(width: 3),
                                Text(
                                  "Disconnect",
                                  style: TextStyle(
                                    color: Colors.redAccent,
                                    fontSize: 11,
                                    fontWeight: FontWeight.bold,
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),

              // ==========================================
              // 2. SCROLLABLE DASHBOARD BODY
              // ==========================================
              Expanded(
                child: SingleChildScrollView(
                  physics: const BouncingScrollPhysics(),
                  padding:
                      const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                  child: Column(
                    children: [
                      // SAMPLING DURATION INPUT CARD
                      Container(
                        padding: const EdgeInsets.all(12),
                        decoration: BoxDecoration(
                          color: Colors.white,
                          borderRadius: BorderRadius.circular(18),
                          boxShadow: [
                            BoxShadow(
                              color: Colors.black.withOpacity(0.04),
                              blurRadius: 10,
                              offset: const Offset(0, 3),
                            ),
                          ],
                        ),
                        child: Row(
                          children: [
                            Container(
                              padding: const EdgeInsets.all(10),
                              decoration: BoxDecoration(
                                color: const Color(0xFFE8F5E9),
                                borderRadius: BorderRadius.circular(12),
                              ),
                              child: const Icon(
                                Icons.timer_outlined,
                                color: Color(0xFF2E7D32),
                                size: 22,
                              ),
                            ),
                            const SizedBox(width: 12),
                            Expanded(
                              child: TextField(
                                controller: _sampController,
                                keyboardType: TextInputType.number,
                                style: const TextStyle(
                                    fontSize: 14, fontWeight: FontWeight.w600),
                                decoration: const InputDecoration(
                                  isDense: true,
                                  border: InputBorder.none,
                                  hintText: "Sampling duration (minutes)",
                                  hintStyle: TextStyle(
                                      color: Colors.grey, fontSize: 13),
                                ),
                              ),
                            ),
                            ElevatedButton(
                              style: ElevatedButton.styleFrom(
                                backgroundColor: const Color(0xFF2E7D32),
                                foregroundColor: Colors.white,
                                elevation: 0,
                                padding: const EdgeInsets.symmetric(
                                    horizontal: 18, vertical: 10),
                                shape: RoundedRectangleBorder(
                                  borderRadius: BorderRadius.circular(12),
                                ),
                              ),
                              onPressed: () {
                                _onSampTimer("TIME");
                              },
                              child: const Text(
                                "Set",
                                style: TextStyle(
                                    fontWeight: FontWeight.bold, fontSize: 13),
                              ),
                            ),
                          ],
                        ),
                      ),
                      const SizedBox(height: 14),

                      // ==========================================
                      // 3. MAIN CONTROL BUTTONS (2x2 GRID)
                      // ==========================================
                      Container(
                        padding: const EdgeInsets.all(14),
                        decoration: BoxDecoration(
                          color: Colors.white,
                          borderRadius: BorderRadius.circular(20),
                          boxShadow: [
                            BoxShadow(
                              color: Colors.black.withOpacity(0.05),
                              blurRadius: 12,
                              offset: const Offset(0, 4),
                            ),
                          ],
                        ),
                        child: Column(
                          children: [
                            Row(
                              children: [
                                // START BUTTON
                                Expanded(
                                  child: _buildControlButton(
                                    title: "START",
                                    subtitle: "Start sampling",
                                    icon: Icons.play_arrow_rounded,
                                    gradient: const LinearGradient(
                                      colors: [
                                        Color(0xFF2E7D32),
                                        Color(0xFF43A047)
                                      ],
                                    ),
                                    enabled: isActivated,
                                    onPressed: isActivated
                                        ? () {
                                            _disabledButton();
                                            _activate("000C");
                                          }
                                        : null,
                                  ),
                                ),
                                const SizedBox(width: 12),
                                // STOP/OFF BUTTON
                                Expanded(
                                  child: _buildControlButton(
                                    title: "STOP",
                                    subtitle: "Turn off",
                                    icon: Icons.stop_rounded,
                                    gradient: const LinearGradient(
                                      colors: [
                                        Color(0xFFD32F2F),
                                        Color(0xFFE53935)
                                      ],
                                    ),
                                    enabled: isDeactivateButtonEnabled,
                                    onPressed: isDeactivateButtonEnabled
                                        ? () {
                                            isDeactivateButtonEnabled = false;
                                            _activateActive();
                                            _deactivate("STOP");
                                            _stopStream();
                                          }
                                        : null,
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 12),
                            Row(
                              children: [
                                // PAUSE BUTTON
                                Expanded(
                                  child: _buildControlButton(
                                    title: "PAUSE",
                                    subtitle: "Pause run",
                                    icon: Icons.pause_rounded,
                                    gradient: const LinearGradient(
                                      colors: [
                                        Color(0xFF1976D2),
                                        Color(0xFF2196F3)
                                      ],
                                    ),
                                    enabled: true,
                                    onPressed: () {
                                      _pauseSamp("PAUSE");
                                    },
                                  ),
                                ),
                                const SizedBox(width: 12),
                                // SAVE BUTTON
                                Expanded(
                                  child: _buildControlButton(
                                    title: "SAVE",
                                    subtitle: "Save log",
                                    icon: Icons.save_rounded,
                                    gradient: const LinearGradient(
                                      colors: [
                                        Color(0xFFE65100),
                                        Color(0xFFFB8C00)
                                      ],
                                    ),
                                    enabled: true,
                                    onPressed: () {
                                      _save("000G");
                                    },
                                  ),
                                ),
                              ],
                            ),
                          ],
                        ),
                      ),
                      const SizedBox(height: 16),

                      // ==========================================
                      // 4. LIVE SENSOR METRICS (2x2 CARDS)
                      // ==========================================
                      Container(
                        padding: const EdgeInsets.all(16),
                        decoration: BoxDecoration(
                          color: Colors.white,
                          borderRadius: BorderRadius.circular(20),
                          boxShadow: [
                            BoxShadow(
                              color: Colors.black.withOpacity(0.04),
                              blurRadius: 10,
                              offset: const Offset(0, 3),
                            ),
                          ],
                        ),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Row(
                              mainAxisAlignment: MainAxisAlignment.spaceBetween,
                              children: [
                                const Row(
                                  children: [
                                    Icon(Icons.sensors_rounded,
                                        size: 18, color: Color(0xFF2E7D32)),
                                    SizedBox(width: 6),
                                    Text(
                                      "Live Sensor Telemetry",
                                      style: TextStyle(
                                        fontSize: 14,
                                        fontWeight: FontWeight.w700,
                                        color: Color(0xFF263238),
                                      ),
                                    ),
                                  ],
                                ),
                                Container(
                                  padding: const EdgeInsets.symmetric(
                                      horizontal: 8, vertical: 3),
                                  decoration: BoxDecoration(
                                    color: const Color(0xFFE8F5E9),
                                    borderRadius: BorderRadius.circular(10),
                                  ),
                                  child: const Text(
                                    "BME280",
                                    style: TextStyle(
                                      fontSize: 10,
                                      fontWeight: FontWeight.bold,
                                      color: Color(0xFF2E7D32),
                                    ),
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 12),
                            Row(
                              children: [
                                // Temperature Card
                                Expanded(
                                  child: _buildSensorCard(
                                    label: "Temperature",
                                    value: temperature.isEmpty
                                        ? (temperatureController.text.isEmpty
                                            ? "--"
                                            : temperatureController.text)
                                        : temperature,
                                    unit: "°C",
                                    icon: Icons.thermostat_rounded,
                                    accentColor: const Color(0xFFFF5722),
                                    bgColor: const Color(0xFFFBE9E7),
                                  ),
                                ),
                                const SizedBox(width: 12),
                                // Humidity Card
                                Expanded(
                                  child: _buildSensorCard(
                                    label: "Humidity",
                                    value: humidity.isEmpty
                                        ? (humidityController.text.isEmpty
                                            ? "--"
                                            : humidityController.text)
                                        : humidity,
                                    unit: "%RH",
                                    icon: Icons.water_drop_rounded,
                                    accentColor: const Color(0xFF0288D1),
                                    bgColor: const Color(0xFFE1F5FE),
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 12),
                            Row(
                              children: [
                                // Pressure Card
                                Expanded(
                                  child: _buildSensorCard(
                                    label: "Pressure",
                                    value: pressure.isEmpty
                                        ? (pressureController.text.isEmpty
                                            ? "--"
                                            : pressureController.text)
                                        : pressure,
                                    unit: "hPa",
                                    icon: Icons.speed_rounded,
                                    accentColor: const Color(0xFF7B1FA2),
                                    bgColor: const Color(0xFFF3E5F5),
                                  ),
                                ),
                                const SizedBox(width: 12),
                                // Air Flow Card
                                Expanded(
                                  child: _buildSensorCard(
                                    label: "Air Flow",
                                    value: airFlow.isEmpty
                                        ? (airFlowController.text.isEmpty
                                            ? "--"
                                            : airFlowController.text)
                                        : airFlow,
                                    unit: "L/min",
                                    icon: Icons.air_rounded,
                                    accentColor: const Color(0xFF00897B),
                                    bgColor: const Color(0xFFE0F2F1),
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 12),
                            Row(
                              children: [
                                // Wind Speed Card
                                Expanded(
                                  child: _buildSensorCard(
                                    label: "Wind Speed",
                                    value: windSpeed.isEmpty
                                        ? (windSpeedController.text.isEmpty
                                            ? "--"
                                            : windSpeedController.text)
                                        : windSpeed,
                                    unit: "m/s",
                                    icon: Icons.storm_rounded,
                                    accentColor: const Color(0xFF3949AB),
                                    bgColor: const Color(0xFFE8EAF6),
                                  ),
                                ),
                                const SizedBox(width: 12),
                                // Wind Direction Card
                                Expanded(
                                  child: _buildSensorCard(
                                    label: "Wind Direction",
                                    value: windDirection.isEmpty
                                        ? (windDirectionController.text.isEmpty
                                            ? "--"
                                            : windDirectionController.text)
                                        : windDirection,
                                    unit: "°",
                                    icon: Icons.explore_rounded,
                                    accentColor: const Color(0xFFF57C00),
                                    bgColor: const Color(0xFFFFE0B2),
                                  ),
                                ),
                              ],
                            ),
                          ],
                        ),
                      ),
                      const SizedBox(height: 16),

                      // ==========================================
                      // 5. UTILITIES & CONFIGURATION PANEL
                      // ==========================================
                      Container(
                        padding: const EdgeInsets.all(16),
                        decoration: BoxDecoration(
                          color: Colors.white,
                          borderRadius: BorderRadius.circular(20),
                          boxShadow: [
                            BoxShadow(
                              color: Colors.black.withOpacity(0.04),
                              blurRadius: 10,
                              offset: const Offset(0, 3),
                            ),
                          ],
                        ),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            const Row(
                              children: [
                                Icon(Icons.settings_suggest_rounded,
                                    size: 18, color: Color(0xFF455A64)),
                                SizedBox(width: 6),
                                Text(
                                  "Device Operations & Storage",
                                  style: TextStyle(
                                    fontSize: 14,
                                    fontWeight: FontWeight.w700,
                                    color: Color(0xFF263238),
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 14),

                            // Autosave Row
                            Container(
                              padding: const EdgeInsets.all(10),
                              decoration: BoxDecoration(
                                color: const Color(0xFFF5F7FA),
                                borderRadius: BorderRadius.circular(14),
                                border:
                                    Border.all(color: const Color(0xFFE4E7EB)),
                              ),
                              child: Row(
                                children: [
                                  const Icon(Icons.sync_rounded,
                                      size: 20, color: Color(0xFF00897B)),
                                  const SizedBox(width: 8),
                                  Expanded(
                                    child: TextField(
                                      controller: _controller,
                                      keyboardType: TextInputType.number,
                                      style: const TextStyle(fontSize: 13),
                                      decoration: const InputDecoration(
                                        isDense: true,
                                        border: InputBorder.none,
                                        hintText: "Autosave interval (mins)",
                                        hintStyle: TextStyle(
                                            color: Colors.grey, fontSize: 12),
                                      ),
                                    ),
                                  ),
                                  ElevatedButton(
                                    style: ElevatedButton.styleFrom(
                                      backgroundColor: const Color(0xFF00897B),
                                      foregroundColor: Colors.white,
                                      elevation: 0,
                                      padding: const EdgeInsets.symmetric(
                                          horizontal: 14, vertical: 8),
                                      shape: RoundedRectangleBorder(
                                        borderRadius: BorderRadius.circular(10),
                                      ),
                                    ),
                                    onPressed: () {
                                      int? minutes =
                                          int.tryParse(_controller.text);
                                      if (minutes != null) {
                                        _startAutoSave(minutes);
                                        _showSnackbar(
                                            "Autosave set to every $minutes minutes");
                                      } else {
                                        _showSnackbar(
                                            "Please enter a valid number");
                                      }
                                    },
                                    child: const Text("Set",
                                        style: TextStyle(
                                            fontWeight: FontWeight.bold,
                                            fontSize: 12)),
                                  ),
                                ],
                              ),
                            ),
                            const SizedBox(height: 12),

                            // Quick Action Chips (2 rows)
                            Row(
                              children: [
                                Expanded(
                                  child: OutlinedButton.icon(
                                    style: OutlinedButton.styleFrom(
                                      foregroundColor: const Color(0xFF2E7D32),
                                      side: const BorderSide(
                                          color: Color(0xFFA5D6A7)),
                                      shape: RoundedRectangleBorder(
                                        borderRadius: BorderRadius.circular(12),
                                      ),
                                      padding: const EdgeInsets.symmetric(
                                          vertical: 10),
                                    ),
                                    onPressed: _syncTimeWithPhone,
                                    icon: const Icon(Icons.sync_rounded,
                                        size: 16),
                                    label: const Text("Sync Time",
                                        style: TextStyle(
                                            fontSize: 12,
                                            fontWeight: FontWeight.bold)),
                                  ),
                                ),
                                /*
                                const SizedBox(width: 8),
                                Expanded(
                                  child: OutlinedButton.icon(
                                    style: OutlinedButton.styleFrom(
                                      foregroundColor: const Color(0xFF00838F),
                                      side: const BorderSide(
                                          color: Color(0xFF80DEEA)),
                                      shape: RoundedRectangleBorder(
                                        borderRadius: BorderRadius.circular(12),
                                      ),
                                      padding: const EdgeInsets.symmetric(
                                          vertical: 10),
                                    ),
                                    onPressed: () {
                                      _getRealtime("GET_RTC");
                                    },
                                    icon: const Icon(Icons.refresh_rounded,
                                        size: 16),
                                    label: const Text("Poll Sensor",
                                        style: TextStyle(
                                            fontSize: 12,
                                            fontWeight: FontWeight.bold)),
                                  ),
                                ),
                                */
                              ],
                            ),
                            const SizedBox(height: 8),
                            Row(
                              children: [
                                Expanded(
                                  child: OutlinedButton.icon(
                                    style: OutlinedButton.styleFrom(
                                      foregroundColor: const Color(0xFFE65100),
                                      side: const BorderSide(
                                          color: Color(0xFFFFCC80)),
                                      shape: RoundedRectangleBorder(
                                        borderRadius: BorderRadius.circular(12),
                                      ),
                                      padding: const EdgeInsets.symmetric(
                                          vertical: 10),
                                    ),
                                    onPressed: _getGps,
                                    icon: const Icon(Icons.gps_fixed_rounded,
                                        size: 16),
                                    label: const Text("Get GPS",
                                        style: TextStyle(
                                            fontSize: 12,
                                            fontWeight: FontWeight.bold)),
                                  ),
                                ),
                                /*
                                const SizedBox(width: 8),
                                Expanded(
                                  child: OutlinedButton.icon(
                                    style: OutlinedButton.styleFrom(
                                      foregroundColor: const Color(0xFFC62828),
                                      side: const BorderSide(
                                          color: Color(0xFFEF9A9A)),
                                      shape: RoundedRectangleBorder(
                                        borderRadius: BorderRadius.circular(12),
                                      ),
                                      padding: const EdgeInsets.symmetric(
                                          vertical: 10),
                                    ),
                                    onPressed: _stopStream,
                                    icon: const Icon(Icons.stop_circle_outlined,
                                        size: 16),
                                    label: const Text("Stop Stream",
                                        style: TextStyle(
                                            fontSize: 12,
                                            fontWeight: FontWeight.bold)),
                                  ),
                                ),
                                */
                              ],
                            ),
                            const SizedBox(height: 12),
                            Row(
                              children: [
                                Expanded(
                                  child: TextField(
                                    controller: latController,
                                    readOnly: true,
                                    decoration: InputDecoration(
                                      labelText: "Latitude (Phone)",
                                      border: OutlineInputBorder(
                                        borderRadius: BorderRadius.circular(12),
                                      ),
                                      filled: true,
                                      fillColor: const Color(0xFFF5F7FA),
                                      prefixIcon: const Icon(Icons.location_on, color: Colors.red),
                                      contentPadding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
                                    ),
                                    style: const TextStyle(fontSize: 13, fontWeight: FontWeight.bold),
                                  ),
                                ),
                                const SizedBox(width: 12),
                                Expanded(
                                  child: TextField(
                                    controller: lngController,
                                    readOnly: true,
                                    decoration: InputDecoration(
                                      labelText: "Longitude (Phone)",
                                      border: OutlineInputBorder(
                                        borderRadius: BorderRadius.circular(12),
                                      ),
                                      filled: true,
                                      fillColor: const Color(0xFFF5F7FA),
                                      prefixIcon: const Icon(Icons.location_on, color: Colors.blue),
                                      contentPadding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
                                    ),
                                    style: const TextStyle(fontSize: 13, fontWeight: FontWeight.bold),
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 12),

                            // File Selection & Print Row
                            /*
                            Row(
                              children: [
                                OutlinedButton.icon(
                                  style: OutlinedButton.styleFrom(
                                    foregroundColor: const Color(0xFFF57F17),
                                    side: const BorderSide(
                                        color: Color(0xFFFFF59D)),
                                    shape: RoundedRectangleBorder(
                                      borderRadius: BorderRadius.circular(12),
                                    ),
                                    padding: const EdgeInsets.symmetric(
                                        horizontal: 12, vertical: 10),
                                  ),
                                  onPressed: () {
                                    _geData("GETDATA");
                                    print(saveItems);
                                  },
                                  icon: const Icon(Icons.folder_open_rounded,
                                      size: 16),
                                  label: const Text("Get Files",
                                      style: TextStyle(
                                          fontSize: 12,
                                          fontWeight: FontWeight.bold)),
                                ),
                                const SizedBox(width: 8),
                                Expanded(
                                  child: Container(
                                    padding: const EdgeInsets.symmetric(
                                        horizontal: 12),
                                    decoration: BoxDecoration(
                                      color: const Color(0xFFF5F7FA),
                                      borderRadius: BorderRadius.circular(12),
                                      border: Border.all(
                                          color: const Color(0xFFE4E7EB)),
                                    ),
                                    child: DropdownButtonHideUnderline(
                                      child: DropdownButton<String>(
                                        isExpanded: true,
                                        hint: const Text("Select Log File",
                                            style: TextStyle(
                                                fontSize: 12,
                                                color: Colors.grey)),
                                        value: _selectedItem,
                                        items: saveItems.map((String value) {
                                          return DropdownMenuItem<String>(
                                            value: value,
                                            child: Text(
                                              value,
                                              style:
                                                  const TextStyle(fontSize: 12),
                                              overflow: TextOverflow.ellipsis,
                                            ),
                                          );
                                        }).toList(),
                                        onChanged: (String? newValue) {
                                          setState(() {
                                            _selectedItem = newValue;
                                          });
                                        },
                                      ),
                                    ),
                                  ),
                                ),
                                const SizedBox(width: 8),
                                ElevatedButton.icon(
                                  style: ElevatedButton.styleFrom(
                                    backgroundColor: const Color(0xFF1565C0),
                                    foregroundColor: Colors.white,
                                    elevation: 0,
                                    padding: const EdgeInsets.symmetric(
                                        horizontal: 14, vertical: 10),
                                    shape: RoundedRectangleBorder(
                                      borderRadius: BorderRadius.circular(12),
                                    ),
                                  ),
                                  onPressed: () {
                                    _onPrintButtonPressed("000E");
                                  },
                                  icon: const Icon(Icons.print_rounded,
                                      size: 16),
                                  label: const Text("Print",
                                      style: TextStyle(
                                          fontWeight: FontWeight.bold,
                                          fontSize: 12)),
                                ),
                              ],
                            ),
                            */
                          ],
                        ),
                      ),
                      const SizedBox(height: 20),

                      // ==========================================
                      // 6. BRANDING FOOTER
                      // ==========================================
                      Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Icon(Icons.verified_rounded,
                              size: 14,
                              color: const Color(0xFF006DA0).withOpacity(0.8)),
                          const SizedBox(width: 6),
                          const Text(
                            "Powered by ",
                            style: TextStyle(
                              color: Color(0xFF78909C),
                              fontSize: 11,
                            ),
                          ),
                          const Text(
                            "PT. Cakrawala Bima Instrument",
                            style: TextStyle(
                              color: Color(0xFF006DA0),
                              fontWeight: FontWeight.bold,
                              fontSize: 11,
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 16),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // ==========================================
  // HELPER WIDGETS FOR MODERN UI
  // ==========================================
  Widget _buildControlButton({
    required String title,
    required String subtitle,
    required IconData icon,
    required Gradient gradient,
    required bool enabled,
    required VoidCallback? onPressed,
  }) {
    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: enabled ? onPressed : null,
        borderRadius: BorderRadius.circular(16),
        child: Opacity(
          opacity: enabled ? 1.0 : 0.45,
          child: Container(
            padding: const EdgeInsets.symmetric(vertical: 14, horizontal: 12),
            decoration: BoxDecoration(
              gradient: enabled
                  ? gradient
                  : const LinearGradient(
                      colors: [Color(0xFF9E9E9E), Color(0xFF757575)]),
              borderRadius: BorderRadius.circular(16),
              boxShadow: enabled
                  ? [
                      BoxShadow(
                        color: Colors.black.withOpacity(0.12),
                        blurRadius: 8,
                        offset: const Offset(0, 3),
                      ),
                    ]
                  : [],
            ),
            child: Row(
              children: [
                Container(
                  padding: const EdgeInsets.all(8),
                  decoration: BoxDecoration(
                    color: Colors.white.withOpacity(0.22),
                    shape: BoxShape.circle,
                  ),
                  child: Icon(icon, color: Colors.white, size: 20),
                ),
                const SizedBox(width: 10),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Text(
                        title,
                        style: const TextStyle(
                          color: Colors.white,
                          fontSize: 14,
                          fontWeight: FontWeight.w800,
                          letterSpacing: 0.5,
                        ),
                      ),
                      Text(
                        subtitle,
                        style: TextStyle(
                          color: Colors.white.withOpacity(0.85),
                          fontSize: 10,
                        ),
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildSensorCard({
    required String label,
    required String value,
    required String unit,
    required IconData icon,
    required Color accentColor,
    required Color bgColor,
  }) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: bgColor,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: accentColor.withOpacity(0.18)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                label,
                style: TextStyle(
                  color: Colors.black.withOpacity(0.7),
                  fontSize: 12,
                  fontWeight: FontWeight.w600,
                ),
              ),
              Icon(icon, color: accentColor, size: 18),
            ],
          ),
          const SizedBox(height: 8),
          Row(
            crossAxisAlignment: CrossAxisAlignment.baseline,
            textBaseline: TextBaseline.alphabetic,
            children: [
              Expanded(
                child: Text(
                  value,
                  style: TextStyle(
                    color: accentColor,
                    fontSize: 18,
                    fontWeight: FontWeight.w800,
                    letterSpacing: 0.2,
                  ),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ),
              const SizedBox(width: 4),
              Text(
                unit,
                style: TextStyle(
                  color: Colors.black.withOpacity(0.55),
                  fontSize: 11,
                  fontWeight: FontWeight.w700,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}
