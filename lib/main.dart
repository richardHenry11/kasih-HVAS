import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';
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
  List<BluetoothDiscoveryResult> results = [];
  List<BluetoothDevice> bondedDevices = [];
  bool isDiscovering = false;
  BluetoothConnection? connection;
  BluetoothDevice? selectedDevice;

  @override
  void initState() {
    // TODO: implement initState
    super.initState();
    // startDiscovery();
    // getBondedDevice();
    requestPermissions();
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
      startDiscovery();
      getBondedDevice();
    } else {
      print("Permissions not granted!");
    }
  }

  void startDiscovery() {
    setState(() {
      results.clear();
      isDiscovering = true;
    });

    FlutterBluetoothSerial.instance.startDiscovery().listen((r) {
      setState(() {
        results.add(r);
      });
    }).onDone(() {
      setState(() {
        isDiscovering = false;
      });
    });

    // timeout scanner
    Future.delayed(Duration(seconds: 30)).then((_) {
      if (isDiscovering) {
        FlutterBluetoothSerial.instance.cancelDiscovery();
        setState(() {
          isDiscovering = false;
        });
      }
    });
  }

  void getBondedDevice() async {
    try {
      List<BluetoothDevice> devices =
          await FlutterBluetoothSerial.instance.getBondedDevices();
      setState(() {
        bondedDevices = devices;
      });
    } catch (e) {
      print("Error in getting bonded devices: $e");
    }
  }

  void disconnectDevice() async {
    if (connection != null) {
      await connection!.close();
      setState(() {
        connection = null;
        selectedDevice = null;
      });
      print('Disconnected');
    }
  }

  Future<void> connectDevice(BluetoothDevice device) async {
    // if(connection != null && selectedDevice?.address == device.address){
    //   // disconnected after second tap
    //   await disconnectDevice();
    // }
    try {
      BluetoothConnection connection =
          await BluetoothConnection.toAddress(device.address);
      setState(() {
        this.connection = connection;
        selectedDevice = device;
      });
      print('Connecting to ${device.name}');
      showSerialCodeDialog();
      // Navigator.push(
      //   context,
      //   MaterialPageRoute(
      //       builder: (context) =>
      //           DashboardApp(connection: connection, device: device)),
      // );
    } catch (exception) {
      print("Can't Connect, exception Occured : $exception");
    }
  }

 void showSerialCodeDialog() {
    TextEditingController codeUsername = TextEditingController();
    TextEditingController codePassword = TextEditingController();
    final height = MediaQuery.of(context).size.height;
    final width = MediaQuery.of(context).size.width;

    showDialog(
      context: context,
      builder: (context) {
        return 
          AlertDialog(
            title: 
            Text('login'),
            content: 
            SizedBox(
              width: width * 0.5,
              height: height * 0.25,
              child: Column(
                children: [
                  TextField(
                    controller: codeUsername,
                    decoration: InputDecoration(hintText: "Username"),
                  ),
                  TextField(
                    controller: codePassword,
                    decoration: InputDecoration(hintText: "Password"),
                    obscureText: true,
                  ),
                ],
              ),
            ),
            actions: [
              TextButton(
                child: Text('Cancel'),
                onPressed: () {
                  Navigator.of(context).pop();
                },
              ),
              
              TextButton(
                child: Text('OK'),
                onPressed: () {
                  if (codeUsername.text == 'dlh' && codePassword.text == 'rokanhulu') { // virtual account
                    Navigator.of(context).pop();
                    Navigator.push(
                      context,
                      MaterialPageRoute(
                        builder: (context) => DashboardApp(connection: connection!, device: selectedDevice!),
                      ),
                    );
                    print("Connected");
                  } else {
                    Navigator.of(context).pop();
                    showErrorDialog();
                  }
                },
              ),
            ],
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
      body: 
        Container(
          decoration: BoxDecoration(
            image: DecorationImage(
              image: AssetImage("img/bg.png"),
              fit: BoxFit.cover,
            ),
          ),
          child: 
          Column(
            children: [
              Card(
                color: Color.fromARGB(255, 51, 11, 44),
                child:
                  Column(
                    children: [
                      Padding(
                        padding: const EdgeInsets.only(top: 10.0),
                        child: Text(
                              'Paired Devices',
                              style: TextStyle(color: Color.fromARGB(255, 0, 81, 255), fontSize: 18, fontWeight: FontWeight.bold),
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
                                title: Text(device.name ?? "", style: TextStyle(color: Colors.white),),
                                subtitle: Text(device.address.toString(), style: TextStyle(color: Color.fromARGB(255, 255, 72, 0)),),
                                onTap: () => connectDevice(device),
                              );
                            }
                          ),
                      ),
                      SizedBox(
                width: 160,
                child: ElevatedButton(
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.blue),
                  onPressed: () {
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
                    ] 
                  )
                ), 
                Expanded(
                  child: ListView.builder(
                    itemCount: results.length,
                    itemBuilder: (BuildContext context, index) {
                      BluetoothDiscoveryResult result = results[index];
                      return ListTile(
                        title: Text(result.device.name ?? "Unknown device"),
                        subtitle: Text(result.device.address.toString()),
                        trailing: Text(result.rssi.toString()),
                        onTap: () => connectDevice(result.device),
                      );
                    },
                                ),
                ),
              Padding(
                padding: const EdgeInsets.all(16.0),
                child: ElevatedButton(
                  style: ElevatedButton.styleFrom(
                      side: BorderSide(
                        color: Color.fromARGB(255, 83, 159, 221),
                        width: 2,
                      ),
                      minimumSize: Size(180, 50)
                    ),
                  onPressed: () {
                    startDiscovery();
                  }, 
                  child: Text("Re-Scan")),
              )
            ],
          ),
      ),
    );
  }
}