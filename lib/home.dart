import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';
import 'package:hvas/main.dart';
import 'package:path_provider/path_provider.dart';
import 'dart:io';
import 'package:permission_handler/permission_handler.dart';
import 'package:intl/intl.dart';
import 'dart:math';

class DashboardApp extends StatelessWidget {
  final BluetoothConnection connection;
  final BluetoothDevice device;

  const DashboardApp({required this.connection, required this.device});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Fitur(conn: connection, device: device),
    );
  }
}

class Fitur extends StatefulWidget {
  final BluetoothConnection conn;
  final BluetoothDevice device;

  const Fitur({super.key, required this.conn, required this.device});

  @override
  State<Fitur> createState() => _FiturState();
}

class _FiturState extends State<Fitur> {
  BluetoothState _btState = BluetoothState.UNKNOWN;
  BluetoothConnection? connection;
  TextEditingController _controller = TextEditingController();
  List<String> messages = [];
  BluetoothDevice? selectedDevice;
  StreamSubscription<Uint8List>? _subscription;
  bool isListening = false;
  final random = Random();
  String humidity = "";
  String temperature = "";
  String airFlow = "";
  String pressure = "";
  String clock = "";
  String setar = "";
  
  // bool isRunning = false;
  // Timer? dummyTimer;
  

  late Stream<DateTime> _timeStream;
  late StreamSubscription<DateTime> _timeSubscription;
  String _currentTime = "";

  // realtime date Vars
  String currentDateTime = DateFormat('yyyy-MM-dd – kk:mm:ss').format(DateTime.now());

  // parameter vars
  final TextEditingController humidityController = TextEditingController();
  final TextEditingController temperatureController = TextEditingController();
  final TextEditingController airFlowController = TextEditingController();
  final TextEditingController pressureController = TextEditingController();
  final TextEditingController setarController = TextEditingController();
  final TextEditingController intervalController = TextEditingController();
  // final TextEditingController _controll = TextEditingController();
  // TextEditingController _timerController = TextEditingController();
  Timer? _timer;

  //Button State
  bool isActivated = true;
  bool isDeactivateButtonEnabled = false;

  //Interval timer autosave variable
  final Duration autoSaveDuration = Duration(minutes: 1);

  // Timer Start Sample
  TextEditingController _sampController = TextEditingController();
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

    connection = widget.conn;

    // state initialization bluetooth
    FlutterBluetoothSerial.instance.state.then((state) {
      setState(() {
        _btState = state;
      });
    });

    FlutterBluetoothSerial.instance
        .onStateChanged()
        .listen((BluetoothState state) {
      setState(() {
        _btState = state;
      });
    });

    // Start Listening to HC-05
    if (!isListening) {
      _subscription = widget.conn.input?.listen((data) {
        setState(() {
          messages.add("Device: ${String.fromCharCodes(data)}");
          _processData(data);
          _processDataFile(data);
          // _geData("GETDATA");
        });
      });
      isListening = true;
    }
    _timeStream =
        Stream<DateTime>.periodic(Duration(seconds: 1), (_) => DateTime.now());
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
    String currentDateTime = DateFormat('yyyy-MM-dd – kk:mm:ss').format(DateTime.now());
    String autoSaveLog = 
        "==============autosave Logs==================\n\n"
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
    while (true) {
      await Future.delayed(Duration(seconds: 5)); // Check every 5 seconds
      if (widget.conn.isConnected) {
        print("Connected");
      } else {
        print('Disconnected');
        disconnected();
        break;
      }
    }
  }

  void _processDataFile(Uint8List data) {
  try {
    // Tambahkan data baru ke buffer
    _buffer += String.fromCharCodes(data);
    print('Buffer sebelum diproses: $_buffer'); // Log buffer sebelum diproses

    // Cek apakah buffer mengandung objek JSON lengkap
    int startIndex = _buffer.indexOf('{');
    int endIndex = _buffer.lastIndexOf('}');

    if (startIndex != -1 && endIndex != -1 && endIndex > startIndex) {
      // Ekstrak string JSON lengkap dari buffer
      String jsonString = _buffer.substring(startIndex, endIndex + 1);

      // Hapus string JSON lengkap dari buffer
      _buffer = _buffer.substring(endIndex + 1);

      // Parsing JSON
      Map<String, dynamic> jsonData = jsonDecode(jsonString);
      List<String> fetchedItems = List<String>.from(jsonData['file']);

      // JSON Logs Structure
      print('Parsed JSON: $fetchedItems');

      // Access dan set nilai sensor
      setState(() {
        saveItems = fetchedItems;
        print('Updated saveItems: $saveItems'); // Updated log saveItems
      });
    }
  } catch (e) {
    print('Error parsing data: $e');
  }
}

  void disconnected() {
    showDialog(
      context: context,
       builder: (context) {
        return AlertDialog(
          title: Column(
            children: [
              Text("Disconnected"),
              Icon(Icons.close_rounded, size: 40, color: Colors.red,)
            ],
          ),
          actions: [
            ElevatedButton(
              style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
              onPressed: (){
                Navigator.push(context, MaterialPageRoute(builder: (context) => HomeScreen()));
              },
              child: Text("OK", style: TextStyle(color: Colors.white),)
              )
          ],
        );
       }
      );
  }

  Future<Directory> _getDownloadDirectory() async {
    Directory? directory = await getExternalStorageDirectory();
    if (directory != null) {
      final downloadDir = Directory('${directory.path}/Download');
      if(await downloadDir.exists()){
        return downloadDir;
      } else{
        return directory;
      }
    } else {
      return getApplicationDocumentsDirectory();
    }
  }

  void _showSnackbar(String message) {
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(
      content: Center(child: Text(message)),
      duration: Duration(seconds: 3),
    ));
  }

  @override
  void dispose() {
    _timer?.cancel();
    _subscription?.cancel();
    _timeSubscription.cancel();
    widget.conn.close();
    // _stopAutoSave();
    super.dispose();
  }

  void _disconnected() async {
    await widget.conn.close();
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
  void _activate(String text) {
    widget.conn.output.add(Uint8List.fromList(utf8.encode(text + "")));
    setState(() {
      messages.add("000C");
      isActivated = false;
    });
    _controller.clear();
    _showSnackbar('Starting...');
    _controller.clear();
    print('starting...');
    print(messages);
    // randomVal();
  }

  void _deactivate(String text) {
    widget.conn.output.add(Uint8List.fromList(utf8.encode(text + "")));
    setState(() {
      messages.add("STOP");
      _stopAutoSave();
    });
    _showSnackbar('Stop Sampling...');
    _controller.clear();
    print('stopping');
    print(messages);
    // randomValNull();
  }

  void _enter(String text)
  {
    widget.conn.output.add(Uint8List.fromList(utf8.encode(text + "")));
    setState(() {
      messages.add("ENT");
    });
    _controller.clear();
    print('save sampling');
    print(messages);
  }

  void _geData(String text) {
    widget.conn.output.add(Uint8List.fromList(utf8.encode(text + "")));
    setState(() {
      messages.add("GETDATA");
    });
    _controller.clear();
    print('getting');
    print(messages);
  }

  void _print(String text) {
    widget.conn.output.add(Uint8List.fromList(utf8.encode(text + "")));
    setState(() {
      messages.add("GETDATA");
      messages.add("000E");
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
      // mix command with selectedIndex
      String printData = command + selectedIndex.toString();
      widget.conn.output.add(Uint8List.fromList(utf8.encode(printData)));

      // add messages to list message
      setState(() {
        messages.add("$command$selectedIndex");
      });

      _showSnackbar("Printing file at index $selectedIndex");
    } else {
      _showSnackbar("Please select a file to print.");
    }
  });
}

  

  void _processData(Uint8List data) {
 try {
      String dataString = String.fromCharCodes(data);
      print('Received data: $dataString'); // received data logging

      // Parsing JSON
      Map<String, dynamic> sensorData = jsonDecode(dataString)['sensors'];
      

      // JSON Structure llogging
      print('Parsed JSON: $sensorData');

      // access and sets sensors values
      setState(() {
        humidity = sensorData['humidity'].toString();
        temperature = sensorData['temperature'].toString();
        airFlow = sensorData['flow'].toString();
        pressure = sensorData['pressure'].toString();
        setar = sensorData['START'].toString();
      });
    } catch (e) {
      print('Error parsing data: $e');
  }
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
      // mix command with selectedIndex
      String printData = command + timer.toString();
      widget.conn.output.add(Uint8List.fromList(utf8.encode(printData)));

      // add messages to list message
      setState(() {
        messages.add("$command$timer");
      });

      _showSnackbar("start time $timer minutes");
    
  });
}


//   void _onPrintButtonPressed(String text) {
//     widget.conn.output.add(Uint8List.fromList(utf8.encode(text + "")));
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
            title: Column(
              children: [
                Icon(Icons.check_circle_rounded,
                    color: Colors.green, size: 40.0),
                Padding(
                  padding: const EdgeInsets.all(12.0),
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
                child: Text(
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
            title: Column(
              children: [
                Icon(Icons.close_rounded, color: Colors.red, size: 40.0),
                Padding(
                  padding: const EdgeInsets.all(12.0),
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
                child: Text(
                  "OK",
                  style: TextStyle(color: Colors.white),
                )),
          );
        });
  }

  void _save(String text) {
    widget.conn.output.add(Uint8List.fromList(utf8.encode(text + "")));
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
    final screenWidth = MediaQuery.of(context).size.width;
    // final screenHeight = MediaQuery.of(context).size.width;
    // BluetoothDevice? dev;

    return Scaffold(
      // appBar: AppBar(
      //   title: Text("Connected to ${dev}"),
      // ),
      // resizeToAvoidBottomInset: false,
       body: SafeArea(
          child: Container(
            decoration: BoxDecoration(
              gradient: LinearGradient(
                begin: const FractionalOffset(-0.44, -1.0), // Corresponds to 263deg
                end: const FractionalOffset(0.44, 1.0),
                colors: [
                  Color.fromARGB(255, 88, 143, 0), // #DEF6B6
                  Color.fromARGB(255, 222, 252, 178), // #F4FCE8
                ],
                stops: [
                  0.5014, // 50.14%
                  0.8679, // 86.79%
                ],
                tileMode: TileMode.clamp,
              ),
            ),
            child: Column(
              children: [
                // Realtime clock section
                Container(
                  padding: EdgeInsets.only(left: 20, right: 20),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      Padding(
                        padding: EdgeInsets.all(0.0),
                        child: Column(
                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                          children: [
                            SizedBox(
                              width: 150,
                              height: 150,
                              child: Image.asset(
                                "img/LogoHVAS.png",
                                // color: Colors.white,
                              ),
                            ),
                            Container(
                              margin: EdgeInsets.only(left: 16.0),
                              child: Text(
                                "High Volume air",
                                style: TextStyle(fontSize: 15, color: Colors.white),
                              ),
                            ),
                            Text(
                              "Sampler",
                              style: TextStyle(fontSize: 15, color: Colors.white),
                            ),
                          ],
                        ),
                      ),
                      Padding(
                        padding: EdgeInsets.all(8.0),
                        child: Padding(
                          padding: const EdgeInsets.only(right: 16.0),
                          child: Column(
                            children: [
                              Text("Time"),
                              // Text(":"),
                              Text(
                                _currentTime,
                                style: TextStyle(
                                    color: Colors.white,
                                    fontSize: 45,
                                    fontWeight: FontWeight.w100),
                              ),
                            ],
                          ),
                        ),
                      ),
                    ],
                  ),
                ),
                
                Text("v 1.0", style: TextStyle(fontSize: 13.0),),
                Divider(),

                // Scrollable content section
                Expanded(
                  child: SingleChildScrollView(
                    child: Column(
                      children: [
                        // debug textLog
                          // SizedBox(
                          //   width: screenWidth * 0.5,
                          //   height: screenHeight * 0.5  ,
                          //   child: Container(
                          //     child: ListView.builder(
                          //       itemCount: messages.length,
                          //       itemBuilder: (context, index){
                          //         return ListTile(
                          //           title: Text(messages[index]),
                          //           );
                          //         }
                          //       ),
                          //     ),
                          //   ),

                        // Timer Sampling                       
                            Column(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                FittedBox(
                                  child: Row(
                                  mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  SizedBox(
                                    width: screenWidth * 0.7,
                                    child: TextField(
                                      controller: _sampController,
                                      decoration: InputDecoration(
                                        border: OutlineInputBorder(borderRadius: BorderRadius.circular(10)),
                                      labelText: "Start Your Sampling on ('minute')",
                                      filled: true,
                                      fillColor: const Color.fromARGB(255, 223, 223, 223)
                                      ),
                                      keyboardType: TextInputType.number,
                                    ),
                                  ),
                                  Padding(
                                    padding: const EdgeInsets.only(left: 16.0),
                                    child: ElevatedButton(
                                      style: ElevatedButton.styleFrom(backgroundColor: Colors.blue, minimumSize: Size(50, 60),
                                       shape: RoundedRectangleBorder(
                                        borderRadius: BorderRadius.circular(10)),),
                                      onPressed: (){
                                        _onSampTimer("TIME");
                                      },
                                      child: Text("Set", style: TextStyle(color: Colors.white),)
                                    
                                      ),
                                  )
                                ],
                                ),
                                )
                              ],
                            ),
                        Padding(
                          padding: const EdgeInsets.only(top: 20),
                          child: Column(
                            children: [
                              Padding(
                                padding: const EdgeInsets.all(20.0),
                                child: Card(
                                  child: Padding(
                                    padding: const EdgeInsets.all(3.0),
                                    child: Column(
                                      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                                      children: [
                                        Row(
                                          mainAxisAlignment: MainAxisAlignment.spaceAround,
                                          children: [
                                            Container(
                                              margin: EdgeInsets.only(right: 8.0, bottom: 8.0),
                                              child: ElevatedButton(
                                                style: ElevatedButton.styleFrom(
                                                  minimumSize: Size(50, 60),
                                                  shape: RoundedRectangleBorder(
                                                    borderRadius: BorderRadius.circular(10),
                                                  ),
                                                  backgroundColor: Colors.green,
                                                ),
                                                onPressed: isActivated ? () {
                                                  _disabledButton();
                                                  _activate("000C");
                                                } : null,
                                                child: Padding(
                                                  padding: const EdgeInsets.all(8.0),
                                                  child: Row(
                                                    mainAxisAlignment: MainAxisAlignment.center,
                                                    children: [
                                                      Icon(Icons.power, color: Colors.white),
                                                      Text(
                                                        "|",
                                                        style: TextStyle(
                                                            color: Colors.white,
                                                            fontSize: 40,
                                                            fontWeight: FontWeight.bold),
                                                      ),
                                                      Text(
                                                        " Start",
                                                        style: TextStyle(
                                                            color: Colors.white,
                                                            fontWeight: FontWeight.bold),
                                                      ),
                                                    ],
                                                  ),
                                                ),
                                              ),
                                            ),
                                            Container(
                                              margin: EdgeInsets.only(left: 8.0, bottom: 8.0),
                                              child: ElevatedButton(
                                                style: ElevatedButton.styleFrom(
                                                  minimumSize: Size(50, 60),
                                                  shape: RoundedRectangleBorder(
                                                    borderRadius: BorderRadius.circular(10),
                                                  ),
                                                  backgroundColor: Colors.red,
                                                ),
                                                onPressed: isDeactivateButtonEnabled ? () {
                                                  // if (isRunning) {
                                                  //   randomValNull(); // OFF
                                                  // }
                                                  isDeactivateButtonEnabled = false;
                                                  _activateActive();
                                                  _deactivate("STOP");
                                                  // randomValNull();
                                                  // print("is running: $isRunning");
                                                }: null,
                                                child: Padding(
                                                  padding: const EdgeInsets.all(8.0),
                                                  child: Row(
                                                    children: [
                                                      Icon(Icons.power_off_rounded, color: Colors.white),
                                                      Text(
                                                        "|",
                                                        style: TextStyle(
                                                            color: Colors.white,
                                                            fontSize: 40,
                                                            fontWeight: FontWeight.bold),
                                                      ),
                                                      Text(
                                                        " Off   ",
                                                        style: TextStyle(
                                                            color: Colors.white,
                                                            fontWeight: FontWeight.bold),
                                                      ),
                                                    ],
                                                  ),
                                                ),
                                              ),
                                            ),
                                          ],
                                        ),
                                        Padding(
                                          padding: const EdgeInsets.all(8.0),
                                          child: Row(
                                            mainAxisAlignment: MainAxisAlignment.spaceAround,
                                            children: [
                                              Container(
                                                margin: EdgeInsets.only(right: 8.0),
                                                child: ElevatedButton(
                                                  style: ElevatedButton.styleFrom(
                                                    minimumSize: Size(50, 60),
                                                    shape: RoundedRectangleBorder(
                                                      borderRadius: BorderRadius.circular(10),
                                                    ),
                                                    backgroundColor: Colors.blue,
                                                  ),
                                                  onPressed: () {
                                                    _enter("ENT");
                                                  },
                                                  child: Padding(
                                                    padding: const EdgeInsets.all(8.0),
                                                    child: 
                                                    // Row(
                                                    //   children: [
                                                    //     Icon(Icons.print_rounded, color: Colors.white),
                                                    //     Text(
                                                    //       "|",
                                                    //       style: TextStyle(
                                                    //           color: Colors.white,
                                                    //           fontSize: 40,
                                                    //           fontWeight: FontWeight.bold),
                                                    //     ),
                                                        Text(
                                                        " Save Samp",
                                                          style: TextStyle(
                                                              color: Colors.white,
                                                              fontWeight: FontWeight.bold),
                                                        ),
                                                      // ],
                                                    // ),
                                                  ),
                                                ),
                                              ),
                                              Container(
                                                margin: EdgeInsets.only(left: 8.0),
                                                child: ElevatedButton(
                                                  style: ElevatedButton.styleFrom(
                                                    minimumSize: Size(50, 60),
                                                    shape: RoundedRectangleBorder(
                                                      borderRadius: BorderRadius.circular(10),
                                                    ),
                                                    backgroundColor: Colors.orange,
                                                  ),
                                                  onPressed: () {
                                                    _save("000G");
                                                  },
                                                  child: Padding(
                                                    padding: const EdgeInsets.all(8.0),
                                                    child: Row(
                                                      children: [
                                                        Icon(Icons.save_rounded, color: Colors.white),
                                                        Text(
                                                          "|",
                                                          style: TextStyle(
                                                              color: Colors.white,
                                                              fontSize: 40,
                                                              fontWeight: FontWeight.bold),
                                                        ),
                                                        Text(
                                                          " Save",
                                                          style: TextStyle(
                                                              color: Colors.white,
                                                              fontWeight: FontWeight.bold),
                                                        ),
                                                      ],
                                                    ),
                                                  ),
                                                ),
                                              ),
                                            ],
                                          ),
                                        ),
                                      ],
                                    ),
                                  ),
                                ),
                              ),
                              Padding(
                                padding: const EdgeInsets.all(20.0),
                                child: Container(
                                  child: Card(
                                    color: Colors.white,
                                    child: Column(
                                      children: [
                                        Padding(
                                          padding: const EdgeInsets.all(10.0),
                                          child: 

                                          //Temperature
                                            FittedBox(
                                            child: Row(
                                                mainAxisAlignment: MainAxisAlignment.start,
                                                children: [
                                                  SizedBox(
                                                    width: screenWidth * 0.3,
                                                    child: TextField(
                                                      decoration: InputDecoration(
                                                        border: OutlineInputBorder(
                                                            borderRadius: BorderRadius.circular(20.0)),
                                                        labelText: 'Temperature',
                                                        labelStyle: TextStyle(color: const Color.fromARGB(255, 53, 53, 53), 
                                                        fontWeight: FontWeight.bold),
                                                        filled: true,
                                                        fillColor: const Color.fromARGB(255, 224, 224, 224)
                                                      ),
                                                      enabled: false,
                                                    ),
                                                  ),
                                                  Padding(padding: EdgeInsets.only(left: 16.0),
                                                    child: SizedBox(
                                                    width: screenWidth * 0.6,
                                                    child: TextField(
                                                      controller: temperatureController,
                                                      decoration: InputDecoration(
                                                        border: OutlineInputBorder(
                                                            borderRadius: BorderRadius.circular(20.0)),
                                                        labelText: "$temperature C",
                                                        labelStyle: TextStyle(color: Colors.blue),
                                                        filled: true,
                                                        fillColor: Color.fromARGB(255, 235, 235, 235)
                                                      ),
                                                      enabled: false,
                                                    ),
                                                  ),
                                                )
                                              ]
                                            ),
                                          )
                                        ),

                                        Padding(
                                          padding: const EdgeInsets.all(8.0),
                                          child: 

                                          //Humidity
                                            FittedBox(
                                            child: Row(
                                                mainAxisAlignment: MainAxisAlignment.start,
                                                children: [
                                                  SizedBox(
                                                    width: screenWidth * 0.3,
                                                    child: TextField(
                                                      decoration: InputDecoration(
                                                        border: OutlineInputBorder(
                                                            borderRadius: BorderRadius.circular(20.0)),
                                                        labelText: 'Humidity',
                                                        labelStyle: TextStyle(color: const Color.fromARGB(255, 53, 53, 53), fontWeight: FontWeight.bold),
                                                        filled: true,
                                                        fillColor: const Color.fromARGB(255, 224, 224, 224)
                                                      ),
                                                      enabled: false,
                                                    ),
                                                  ),
                                                  Padding(padding: EdgeInsets.only(left: 16.0),
                                                    child: SizedBox(
                                                    width: screenWidth * 0.6,
                                                    child: TextField(
                                                      controller: humidityController,
                                                      decoration: InputDecoration(
                                                        border: OutlineInputBorder(
                                                            borderRadius: BorderRadius.circular(20.0)),
                                                        labelText: "$humidity %RH",
                                                        labelStyle: TextStyle(color: Colors.blue),
                                                        filled: true,
                                                        fillColor: Color.fromARGB(255, 235, 235, 235)
                                                      ),
                                                      enabled: false,
                                                    ),
                                                  ),
                                                )
                                              ]
                                            ),
                                          )
                                        ),

                                        Padding(
                                          padding: const EdgeInsets.all(8.0),
                                          child: 

                                          //Pressure
                                            FittedBox(
                                            child: Row(
                                                mainAxisAlignment: MainAxisAlignment.start,
                                                children: [
                                                  SizedBox(
                                                    width: screenWidth * 0.3,
                                                    child: TextField(
                                                      decoration: InputDecoration(
                                                        border: OutlineInputBorder(
                                                            borderRadius: BorderRadius.circular(20.0)),
                                                        labelText: 'Pressure',
                                                        labelStyle: TextStyle(color: const Color.fromARGB(255, 53, 53, 53), fontWeight: FontWeight.bold),
                                                        filled: true,
                                                        fillColor: const Color.fromARGB(255, 224, 224, 224)
                                                      ),
                                                      enabled: false,
                                                    ),
                                                  ),
                                                  Padding(padding: EdgeInsets.only(left: 16.0),
                                                    child: SizedBox(
                                                    width: screenWidth * 0.6,
                                                    child: TextField(
                                                      controller: pressureController,
                                                      decoration: InputDecoration(
                                                        border: OutlineInputBorder(
                                                            borderRadius: BorderRadius.circular(20.0)),
                                                        labelText: "$pressure hPa",
                                                        labelStyle: TextStyle(color: Colors.blue),
                                                        filled: true,
                                                        fillColor: Color.fromARGB(255, 235, 235, 235)
                                                      ),
                                                      enabled: false,
                                                    ),
                                                  ),
                                                )
                                              ]
                                            ),
                                          )
                                        ),

                                        Padding(
                                          padding: const EdgeInsets.all(8.0),
                                          child: 

                                          //Air Flow
                                            FittedBox(
                                            child: Row(
                                                mainAxisAlignment: MainAxisAlignment.start,
                                                children: [
                                                  SizedBox(
                                                    width: screenWidth * 0.3,
                                                    child: TextField(
                                                      decoration: InputDecoration(
                                                        border: OutlineInputBorder(
                                                            borderRadius: BorderRadius.circular(20.0)),
                                                        labelText: 'Air Flow',
                                                        labelStyle: TextStyle(color: const Color.fromARGB(255, 53, 53, 53), fontWeight: FontWeight.bold),
                                                        filled: true,
                                                        fillColor: const Color.fromARGB(255, 224, 224, 224)
                                                      ),
                                                      enabled: false,
                                                    ),
                                                  ),
                                                  Padding(padding: EdgeInsets.only(left: 16.0),
                                                    child: SizedBox(
                                                    width: screenWidth * 0.6,
                                                    child: TextField(
                                                      controller: airFlowController,
                                                      decoration: InputDecoration(
                                                        border: OutlineInputBorder(
                                                            borderRadius: BorderRadius.circular(20.0)),
                                                        labelText: "$airFlow L/min",
                                                        labelStyle: TextStyle(color: Colors.blue),
                                                        filled: true,
                                                        fillColor: Color.fromARGB(255, 235, 235, 235)
                                                      ),
                                                      enabled: false,
                                                    ),
                                                  ),
                                                )
                                              ]
                                            ),
                                          )
                                        ),
                                      ],
                                    ),
                                  ),
                                ),
                              ),

                              // Autosaving settings
                              Column(
                                children: [
                                  Text(
                                    "Input your autosaving timer",
                                    style: TextStyle(
                                        color: const Color.fromARGB(255, 139, 139, 139)),
                                  ),
                                  Center(
                                    child: Padding(
                                      padding: EdgeInsets.all(1.0),
                                      child: SingleChildScrollView(
                                        scrollDirection: Axis.vertical,
                                        child: Center(
                                          child: Container(
                                            child: Row(
                                              mainAxisAlignment: MainAxisAlignment.center,
                                              children: [
                                                SizedBox(
                                                  width: screenWidth * 0.7,
                                                  child: TextField(
                                                    controller: _controller,
                                                    decoration: InputDecoration(
                                                      border: OutlineInputBorder(
                                                          borderRadius: BorderRadius.circular(20.0)),
                                                      labelText: 'interval',
                                                    ),
                                                    keyboardType: TextInputType.number,
                                                  ),
                                                ),
                                                Padding(
                                                  padding: const EdgeInsets.only(left: 8.0),
                                                  child: ElevatedButton(
                                                    style: ElevatedButton.styleFrom(
                                                      minimumSize: Size(50, 60),
                                                      backgroundColor: Colors.green,
                                                      shape: RoundedRectangleBorder(
                                                          borderRadius: BorderRadius.circular(10)),
                                                    ),
                                                    onPressed: () {
                                                      int? minutes = int.tryParse(_controller.text);
                                                      if (minutes != null) {
                                                        _startAutoSave(minutes);
                                                        _showSnackbar("Autosave set to every $minutes minutes");
                                                      } else {
                                                        _showSnackbar("Please enter a valid number");
                                                      }
                                                    },
                                                    child: Padding(
                                                      padding: const EdgeInsets.all(8.0),
                                                      child: Text(
                                                        "Set",
                                                        style: TextStyle(color: Colors.white),
                                                      ),
                                                    ),
                                                  ),
                                                ),
                                              ],
                                            ),
                                          ),
                                        ),
                                      ),
                                    ),
                                  ),
                                ],
                              ),

                            //   // Get Data
                            //   ElevatedButton(
                            //     style: ElevatedButton.styleFrom(
                            //       shape: RoundedRectangleBorder(
                            //         borderRadius: BorderRadius.circular(10),
                            //       ),
                            //       backgroundColor: Colors.yellow,
                            //     ),
                            //     onPressed: () {
                            //       _geData("GETDATA");
                            //       print(saveItems);
                            //     },
                            //     child: Text("Get Data file"),
                            //   ),

                            //   // Dropdown List
                            //  SizedBox(
                            //     width: screenWidth * 0.8,
                            //     child: DropdownButton<String>(
                            //       hint: Text("Pilih File"),
                            //       value: _selectedItem,
                            //       items: saveItems.map((String value) {
                            //         return DropdownMenuItem<String>(
                            //           value: value,
                            //           child: Text(value),
                            //         );
                            //       }).toList(),
                            //       onChanged: (String? newValue) {
                            //         setState(() {
                            //           _selectedItem = newValue;
                            //         });
                            //       },
                            //     ),
                            //   ),
                            //   ElevatedButton(
                            //     style: ElevatedButton.styleFrom(
                            //       shape: RoundedRectangleBorder(
                            //         borderRadius: BorderRadius.circular(10)
                            //       ),
                            //       backgroundColor: Colors.blue,
                            //     ),
                            //     onPressed: () {
                            //       _onPrintButtonPressed("000E");
                            //     },
                            //     child: Text("Print", style: TextStyle(color: Colors.white),),
                            //   ),
                              // Copyright
                              Padding(
                                padding: const EdgeInsets.only(top: 4.0, right: 25.0, left: 25.0),
                                child: Divider(
                                  color: Color.fromARGB(255, 100, 105, 150),
                                ),
                              ),
                              Container(
                                margin: EdgeInsets.only(bottom: 1.0),
                                child: Text(
                                  "Powered By",
                                  style: TextStyle(color: const Color.fromARGB(255, 139, 139, 139)),
                                ),
                              ),
                              Text(
                                "Dinas Lingkungan Hidup Kab. Rokan Hulu",
                                style: TextStyle(
                                    color: Color.fromARGB(255, 0, 109, 160),
                                    fontWeight: FontWeight.bold),
                              ),
                            ],
                          ),
                        ),
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
}
