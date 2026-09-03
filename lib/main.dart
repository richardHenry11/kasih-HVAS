import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'dart:async';
import 'package:hvas/home.dart';
import 'package:permission_handler/permission_handler.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: HomeScreen(),
    );
  }
}

class HomeScreen extends StatefulWidget {
  @override
  _HomeScreenState createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  List<ScanResult> results = [];
  List<BluetoothDevice> bondedDevices = [];
  bool isDiscovering = false;
  BluetoothDevice? selectedDevice;
  StreamSubscription<List<ScanResult>>? _scanResultsSubscription;
  StreamSubscription<bool>? _isScanningSubscription;

  @override
  void initState() {
    super.initState();
    requestPermissions();
    _scanResultsSubscription = FlutterBluePlus.scanResults.listen((r) {
      if (mounted) setState(() { results = r; });
    });
    _isScanningSubscription = FlutterBluePlus.isScanning.listen((scanning) {
      if (mounted) setState(() { isDiscovering = scanning; });
    });
  }

  @override
  void dispose() {
    _scanResultsSubscription?.cancel();
    _isScanningSubscription?.cancel();
    super.dispose();
  }

  Future<void> requestPermissions() async {
    var status = await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.bluetooth,
      Permission.location
    ].request();

    if (status[Permission.bluetoothScan]!.isGranted &&
        status[Permission.bluetoothConnect]!.isGranted &&
        status[Permission.bluetooth]!.isGranted &&
        status[Permission.location]!.isGranted) {
      
      BluetoothAdapterState bluetoothState = await FlutterBluePlus.adapterState.first;
      if (bluetoothState == BluetoothAdapterState.off) {
        await FlutterBluePlus.turnOn();
      }

      startDiscovery();
      getBondedDevice();
    } else {
      print("Permissions not granted!");
    }
  }

  void startDiscovery() async {
    results.clear();
    setState(() {});
    await FlutterBluePlus.startScan(timeout: Duration(seconds: 15));
  }

  void getBondedDevice() async {
    try {
      List<BluetoothDevice> devices = await FlutterBluePlus.bondedDevices;
      setState(() {
        bondedDevices = devices;
      });
    } catch (e) {
      print("Error in getting bonded devices: $e");
    }
  }

  void disconnectDevice() async {
    if (selectedDevice != null) {
      await selectedDevice!.disconnect();
      setState(() {
        selectedDevice = null;
      });
      print('Disconnected');
    }
  }

  Future<void> connectDevice(BluetoothDevice device) async {
    try {
      await device.connect(timeout: Duration(seconds: 5), autoConnect: false);
      setState(() {
        selectedDevice = device;
      });
      print('Connecting to ${device.platformName}');
      showSerialCodeDialog();
      // Navigator.push(
      //   context,
      //   MaterialPageRoute(
      //       builder: (context) =>
      //           DashboardApp(connection: connection, device: device)),
      // );
    } catch (exception) {
      print("Can't Connect, exception Occured : $exception");
      showOfflineDialog();
    }
  }

  void showOfflineDialog() {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return Dialog(
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(20.0),
          ),
          elevation: 0,
          backgroundColor: Colors.transparent,
          child: Container(
            padding: EdgeInsets.all(20),
            decoration: BoxDecoration(
              color: Colors.white,
              shape: BoxShape.rectangle,
              borderRadius: BorderRadius.circular(20),
              boxShadow: [
                BoxShadow(
                  color: Colors.black26,
                  blurRadius: 10.0,
                  offset: const Offset(0.0, 10.0),
                ),
              ],
            ),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: <Widget>[
                Container(
                  padding: EdgeInsets.all(16),
                  decoration: BoxDecoration(
                    color: Colors.redAccent.withOpacity(0.1),
                    shape: BoxShape.circle,
                  ),
                  child: Icon(
                    Icons.bluetooth_disabled_rounded,
                    size: 60,
                    color: Colors.redAccent,
                  ),
                ),
                SizedBox(height: 24.0),
                Text(
                  "Device Offline",
                  style: TextStyle(
                    fontSize: 22.0,
                    fontWeight: FontWeight.w700,
                    color: Colors.black87,
                  ),
                ),
                SizedBox(height: 16.0),
                Text(
                  "Device offline, silahkan nyalakan bluetooth device untuk scanning.",
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    fontSize: 15.0,
                    color: Colors.black54,
                    height: 1.5,
                  ),
                ),
                SizedBox(height: 24.0),
                SizedBox(
                  width: double.infinity,
                  height: 50,
                  child: ElevatedButton(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.blueAccent,
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(12),
                      ),
                      elevation: 0,
                    ),
                    onPressed: () {
                      Navigator.of(context).pop();
                    },
                    child: Text(
                      "Mengerti",
                      style: TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.bold,
                        color: Colors.white,
                      ),
                    ),
                  ),
                ),
              ],
            ),
          ),
        );
      },
    );
  }

  void showSerialCodeDialog() {
    TextEditingController codeUsername = TextEditingController();
    TextEditingController codePassword = TextEditingController();
    
    showGeneralDialog(
      context: context,
      barrierDismissible: true,
      barrierLabel: MaterialLocalizations.of(context).modalBarrierDismissLabel,
      barrierColor: Colors.black54,
      transitionDuration: const Duration(milliseconds: 300),
      pageBuilder: (context, anim1, anim2) {
        return const SizedBox.shrink(); 
      },
      transitionBuilder: (context, anim1, anim2, child) {
        return Transform.scale(
          scale: Curves.easeOutBack.transform(anim1.value),
          child: Opacity(
            opacity: anim1.value,
            child: Dialog(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(24.0),
              ),
              elevation: 8,
              backgroundColor: Colors.transparent,
              child: Container(
                padding: const EdgeInsets.all(24),
                decoration: BoxDecoration(
                  color: Colors.white,
                  borderRadius: BorderRadius.circular(24),
                  boxShadow: const [
                    BoxShadow(
                      color: Colors.black26,
                      blurRadius: 20.0,
                      offset: Offset(0.0, 10.0),
                    ),
                  ],
                ),
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Container(
                      padding: const EdgeInsets.all(16),
                      decoration: BoxDecoration(
                        color: Colors.blueAccent.withOpacity(0.1),
                        shape: BoxShape.circle,
                      ),
                      child: const Icon(
                        Icons.lock_person_rounded,
                        size: 50,
                        color: Colors.blueAccent,
                      ),
                    ),
                    const SizedBox(height: 20),
                    const Text(
                      'Authentication',
                      style: TextStyle(
                        fontSize: 24,
                        fontWeight: FontWeight.bold,
                        color: Colors.black87,
                      ),
                    ),
                    const SizedBox(height: 8),
                    const Text(
                      'Please enter your credentials to connect',
                      textAlign: TextAlign.center,
                      style: TextStyle(
                        fontSize: 14,
                        color: Colors.black54,
                      ),
                    ),
                    const SizedBox(height: 30),
                    TextField(
                      controller: codeUsername,
                      decoration: InputDecoration(
                        labelText: 'Username',
                        prefixIcon: const Icon(Icons.person_outline),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(16),
                        ),
                        contentPadding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
                      ),
                    ),
                    const SizedBox(height: 16),
                    TextField(
                      controller: codePassword,
                      obscureText: true,
                      decoration: InputDecoration(
                        labelText: 'Password',
                        prefixIcon: const Icon(Icons.lock_outline),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(16),
                        ),
                        contentPadding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
                      ),
                    ),
                    const SizedBox(height: 32),
                    Row(
                      children: [
                        Expanded(
                          child: TextButton(
                            style: TextButton.styleFrom(
                              padding: const EdgeInsets.symmetric(vertical: 16),
                              shape: RoundedRectangleBorder(
                                borderRadius: BorderRadius.circular(12),
                              ),
                            ),
                            onPressed: () {
                              Navigator.of(context).pop();
                            },
                            child: const Text(
                              'Cancel',
                              style: TextStyle(
                                fontSize: 16,
                                fontWeight: FontWeight.bold,
                                color: Colors.black54,
                              ),
                            ),
                          ),
                        ),
                        const SizedBox(width: 16),
                        Expanded(
                          child: ElevatedButton(
                            style: ElevatedButton.styleFrom(
                              backgroundColor: Colors.blueAccent,
                              padding: const EdgeInsets.symmetric(vertical: 16),
                              shape: RoundedRectangleBorder(
                                borderRadius: BorderRadius.circular(12),
                              ),
                              elevation: 0,
                            ),
                            onPressed: () {
                              if (codeUsername.text == 'cbi' && codePassword.text == 'cbi1') {
                                Navigator.of(context).pop();
                                Navigator.push(
                                  context,
                                  MaterialPageRoute(
                                    builder: (context) => DashboardApp(
                                        device: selectedDevice!),
                                  ),
                                );
                                print("Connected");
                              } else {
                                Navigator.of(context).pop();
                                showErrorDialog();
                              }
                            },
                            child: const Text(
                              'Login',
                              style: TextStyle(
                                fontSize: 16,
                                fontWeight: FontWeight.bold,
                                color: Colors.white,
                              ),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ),
        );
      },
    );
  }

  void showErrorDialog() {
    showDialog(
        context: context,
        builder: (context) {
          return AlertDialog(
            title: Text('Error'),
            content: Text("Username or Password invalid!"),
            actions: [
              TextButton(
                onPressed: () {
                  disconnectDevice();
                  Navigator.of(context).pop();
                },
                child: Text("OK"),
              )
            ],
          );
        });
  }

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;

    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          image: DecorationImage(
            image: AssetImage("img/bg.png"),
            fit: BoxFit.cover,
          ),
        ),
        child: Column(
          children: [
            Card(
                color: Color.fromARGB(255, 51, 11, 44),
                child: Column(children: [
                  Padding(
                    padding: const EdgeInsets.only(top: 10.0),
                    child: Text(
                      'Paired Devices',
                      style: TextStyle(
                          color: Color.fromARGB(255, 0, 81, 255),
                          fontSize: 18,
                          fontWeight: FontWeight.bold),
                    ),
                  ),
                  SizedBox(
                    width: screenWidth * 1,
                    height: screenHeight * 0.4,
                    child: ListView.builder(
                        itemCount: bondedDevices.length,
                        itemBuilder: (BuildContext context, index) {
                          BluetoothDevice device = bondedDevices[index];
                          return ListTile(
                            title: Text(
                              device.platformName.isEmpty ? "Unknown device" : device.platformName,
                              style: TextStyle(color: Colors.white),
                            ),
                            subtitle: Text(
                              device.remoteId.toString(),
                              style: TextStyle(
                                  color: Color.fromARGB(255, 255, 72, 0)),
                            ),
                            onTap: () => connectDevice(device),
                          );
                        }),
                  ),
                  SizedBox(
                    width: 160,
                    child: ElevatedButton(
                      style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.blue),
                      onPressed: () async {
                        BluetoothAdapterState bluetoothState = await FlutterBluePlus.adapterState.first;
                        if (bluetoothState == BluetoothAdapterState.off) {
                          await FlutterBluePlus.turnOn();
                        }
                        getBondedDevice();
                      },
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Text(
                            "Reset Paired |",
                            style: TextStyle(color: Colors.white),
                          ),
                          Icon(
                            Icons.bluetooth_connected,
                            color: Colors.white,
                          ),
                        ],
                      ),
                    ),
                  ),
                ])),
            Expanded(
              child: ListView.builder(
                itemCount: results.length,
                itemBuilder: (BuildContext context, index) {
                  ScanResult result = results[index];
                  return ListTile(
                    title: Text(result.device.platformName.isEmpty ? "Unknown device" : result.device.platformName),
                    subtitle: Text(result.device.remoteId.toString()),
                    trailing: Text(result.rssi.toString()),
                    onTap: () => connectDevice(result.device),
                  );
                },
              ),
            ),
            Padding(
              padding: const EdgeInsets.all(16.0),
              child: Column(
                children: [
                  ElevatedButton(
                      style: ElevatedButton.styleFrom(
                          side: BorderSide(
                            color: Color.fromARGB(255, 83, 159, 221),
                            width: 2,
                          ),
                          minimumSize: Size(180, 50)),
                      onPressed: () async {
                        BluetoothAdapterState bluetoothState = await FlutterBluePlus.adapterState.first;
                        if (bluetoothState == BluetoothAdapterState.off) {
                          await FlutterBluePlus.turnOn();
                        }
                        startDiscovery();
                      },
                      child: Text("Re-Scan")),
                  SizedBox(height: 10),
                  ElevatedButton(
                      style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.green,
                          minimumSize: Size(180, 50)),
                      onPressed: () {
                        Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (context) => DashboardApp(),
                          ),
                        );
                      },
                      child: Text("Free Access", style: TextStyle(color: Colors.white))),
                ],
              ),
            )
          ],
        ),
      ),
    );
  }
}
