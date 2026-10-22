#define MyAdress 37

static void _send(void* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        MySerial.write(((uint8_t*)data)[i]);
    }
}

void send(void* data, PacketType type, size_t size) {
    uint8_t myAdress = (uint8_t)MyAdress;
    MySerial.write('H');
    _send(&myAdress, sizeof(myAdress));
    _send(&type, sizeof(type));
    _send(data, size);
}

void sendString(String str) {
    send((void*)str.c_str(), PacketType::String, strlen(str.c_str()));
}

void sendData() {
    uint8_t data[13] = { (uint8_t)mode_G, r_G, g_G, b_G, color_G, distance_G,
          (uint8_t)(pitch_G >> 8), (uint8_t)pitch_G, (uint8_t)(roll_G >> 8),
        (uint8_t)roll_G, (uint8_t)(yaw_G >> 8), (uint8_t)yaw_G, isStacking_G };
    send((void*)data, PacketType::Data, sizeof(data));
}

bool receive()
{
    bool wasReceived = false;
    if (MySerial.available() > 2) {
        char firstByte = MySerial.read();
        while (firstByte != 'H') {
            firstByte = MySerial.read();
            if (MySerial.available() == 0){
                return false;
            }
        }
        uint8_t myAdress = (uint8_t)MyAdress;
        uint8_t receivedAdress = MySerial.read();
        if (receivedAdress != myAdress) {
            return false;
        }
        wasReceived = true;
        PacketType type = (PacketType)MySerial.read();
        switch (type) {
            case PacketType::Data: {
                sendData();
                break;
            }
            default:
                break;
        }
    }
    return wasReceived;
}



