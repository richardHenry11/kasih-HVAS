import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';

class ChatScreen extends StatelessWidget {
  final BluetoothConnection connection;
  final BluetoothDevice device;

  ChatScreen({required this.connection, required this.device});

  @override
  Widget build(BuildContext context) {
    return WillPopScope(
      onWillPop: () async {
        await connection.close();
        return true;
      },
      child: Scaffold(
        appBar: AppBar(
          backgroundColor: Color.fromARGB(255, 137, 188, 255),
          title: Text(device.name ?? 'Chat Screen'),
        ),
        body: Chatting(conn: connection),
      ),
    );
  }
}

class Chatting extends StatefulWidget {
  final BluetoothConnection conn;

  Chatting({required this.conn});

  @override
  State<Chatting> createState() => _ChattingState();
}

class _ChattingState extends State<Chatting> {
  BluetoothConnection? connection;
  TextEditingController _controller = TextEditingController();
  List<String> messages = [];
  StreamSubscription<Uint8List>? _subscription;
  bool isListening = false;

  void _sendMessage(String text) {
    if (text.isNotEmpty) {
      widget.conn.output.add(Uint8List.fromList(utf8.encode(text + "\r\n")));
      setState(() {
        messages.add("Me: $text");
      });
      _controller.clear();
    }
  }

  @override
  void initState() {
    super.initState();
    if (!isListening) {
      _subscription = widget.conn.input!.listen((data) {
        setState(() {
          messages.add("Device: ${String.fromCharCodes(data)}");
        });
      });
      isListening = true;
    }
  }

  @override
  void dispose() {
    _subscription?.cancel();
    widget.conn.close();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    // final screenHeight = MediaQuery.of(context).size.height;
    return Scaffold(
      body: Container(
        margin: EdgeInsets.all(8.0),
        child: Column(
          children: [
            // Expanded text history:
            Expanded(
              child: ListView.builder(
                itemCount: messages.length,
                itemBuilder: (context, index) {
                  return ListTile(
                    title: Text(messages[index]),
                  );
                },
              ),
            ),
            Spacer(),
            // Send and type message
            Center(
              child: Padding(
                padding: EdgeInsets.all(16.0),
                child: SingleChildScrollView(
                  scrollDirection: Axis.horizontal,
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
                                labelText: 'Kirim Pesan',
                              ),
                            ),
                          ),
                          IconButton(
                            onPressed: () {
                              final text = _controller.text;
                              _sendMessage(text);
                            },
                            icon: Icon(
                              Icons.send_rounded,
                              size: 30.0,
                              color: Colors.blue,
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
      ),
    );
  }
}
