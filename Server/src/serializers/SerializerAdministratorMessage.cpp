//
// Created by paolo on 15.05.2020.
//

#include "SerializerAdministratorMessage.h"

using namespace std;

SerializerAdministratorMessage::SerializerAdministratorMessage(IModelForAdministrator* model){
    this->model = model;
}

int SerializerAdministratorMessage::analyzeMessage(int clientId, vector<char> message) {
    if (message.empty()) {
        return -1;
    }
    int readingBegin = 0;
//    int paramQuantity = ByteToInt(message, readingBegin);
//    readingBegin += 4;
//    printf("Param quantity: %d\n", paramQuantity);

    int sizeOfCommandType = ByteToInt(message, readingBegin);
    readingBegin += 4;
    printf("Size of command type: %d\n", sizeOfCommandType);

    int commandType = ByteToInt(message, readingBegin);
    readingBegin += 4;

    if (commandType == GET_ALL_SENSORS) { //GET_ALL_SENSORS
        printf("Command: GetAllSensors\n");
        model->administratorCommandGetAllSensors(clientId);
        return GET_ALL_SENSORS;
    } else if (commandType == UPDATE_SENSOR_NAME) { //UPDATE_SENSOR_NAME
        printf("Command: UpdateSensorName\n");
        int sizeOfId = ByteToInt(message, readingBegin);
        readingBegin += 4;

        int id = ByteToInt(message, readingBegin);
        readingBegin += 4;

        int sizeOfName = ByteToInt(message, readingBegin);
        readingBegin += 4;

        string name;
        for (int i = 0; i < sizeOfName; i++) {
            name += message[readingBegin + i];
        }

        printf("SizeOfId: %d; Id: %d; SizeOfName: %d; Name: %s\n", sizeOfId, id, sizeOfName, name.c_str());
        model->administratorCommandUpdateSensorName(clientId, id, name);
        return UPDATE_SENSOR_NAME;
    } else if (commandType == REVOKE_SENSOR) { //REVOKE_SENSOR
        printf("Command: RevokeSensor\n");

        int sizeOfId = ByteToInt(message, readingBegin);
        readingBegin += 4;

        int id = ByteToInt(message, readingBegin);
        readingBegin += 4;

        printf("SizeOfId: %d; Id: %d\n", sizeOfId, id);
        model->administratorCommandRevokeSensor(clientId, id);
        return REVOKE_SENSOR;
    } else if (commandType == DISCONNECT_SENSOR) { //DISCONNECT_SENSOR
        printf("Command: DisconnectSensor\n");

        int sizeOfId = ByteToInt(message, readingBegin);
        readingBegin += 4;

        int id = ByteToInt(message, readingBegin);
        readingBegin += 4;

        printf("SizeOfId: %d; Id: %d\n", sizeOfId, id);
        model->administratorCommandDisconnectSensor(clientId, id);
        return DISCONNECT_SENSOR;
    } else if (commandType == GENERATE_TOKEN) { //GENERATE_TOKEN
        printf("Command: GenerateToken\n");

        int sizeOfName = ByteToInt(message, readingBegin);
        readingBegin += 4;

        string name; //TODO fix name - check how works sending string;;; W javie String jeden znak jest zapisany na 2 Byte trzeba zrobić konwersje (moze z javy wysyałac jako tablica char?)
        for (int i = 0; i < sizeOfName; i++) {
            //printf("%c|", message[readingBegin+i]);
            name += message[readingBegin + i];
        }

        printf("SizeOfName: %d; Name: %s\n", sizeOfName, name.c_str());
        model->administratorCommandGenerateToken(clientId, name);
        return GENERATE_TOKEN;
    } else {
        printf("Error command type unrecogized");
    }
    return -1;
}

vector<char> SerializerAdministratorMessage::constructGetAllSensorsMessage(vector<Sensor> sensors) {
    vector<char> message = IntToByte(GET_ALL_SENSORS);
    vector<char> sensorsMessage = constructSensorListMessage(sensors);
    message.insert(message.end(), sensorsMessage.begin(), sensorsMessage.end());
    return message;
}

vector<char> SerializerAdministratorMessage::constructUpdateSensorNameMessage(int updatedSensorId) {
    ///Example: 1 50
    vector<char> message = IntToByte(UPDATE_SENSOR_NAME);
    vector<char> sensorIdMessage = IntToByte(updatedSensorId);
    message.insert(message.end(), sensorIdMessage.begin(), sensorIdMessage.end());
    return message;
}

vector<char> SerializerAdministratorMessage::constructRevokeSensorMessage(int revokedSensorId) {
    ///Example: 2 50
    vector<char> message = IntToByte(REVOKE_SENSOR);
    vector<char> sensorIdMessage = IntToByte(revokedSensorId);
    message.insert(message.end(), sensorIdMessage.begin(), sensorIdMessage.end());
    return message;
}

vector<char> SerializerAdministratorMessage::constructDisconnectSensorMessage(int disconnectedSensorId) {
    ///Example: 3 50
    vector<char> message = IntToByte(DISCONNECT_SENSOR);
    vector<char> sensorIdMessage = IntToByte(disconnectedSensorId);
    message.insert(message.end(), sensorIdMessage.begin(), sensorIdMessage.end());
    return message;
}

vector<char> SerializerAdministratorMessage::constructGenerateTokenMessage(string token) {
    ///Example: 4 12 tokenContent
    vector<char> message = IntToByte(GENERATE_TOKEN);
    vector<char> tokenMessage = constructStringMessageWithSize(token);
    message.insert(message.end(), tokenMessage.begin(), tokenMessage.end());
    return message;
}


vector<char> SerializerAdministratorMessage::constructSensorMessage(Sensor sensor) {
    vector<char> message;
    //id
    vector<char> tmp = constructIntMessageWithSize(sensor.id);
    message.insert(message.end(), tmp.begin(), tmp.end());
    //name
    tmp = constructStringMessageWithSize(sensor.name);
    message.insert(message.end(), tmp.begin(), tmp.end());
    //ip
    tmp = constructStringMessageWithSize(sensor.ip);
    message.insert(message.end(), tmp.begin(), tmp.end());
    //port
    tmp = constructIntMessageWithSize(sensor.port);
    message.insert(message.end(), tmp.begin(), tmp.end());
    //connected
    tmp = constructBoolMessageWithSize(sensor.connected);
    message.insert(message.end(), tmp.begin(), tmp.end());

    return message;
}

vector<char> SerializerAdministratorMessage::constructSensorListMessage(vector<Sensor> sensors) {
    vector<char> message = IntToByte(sensors.size());

    for (int i = 0; i < sensors.size(); i++) {
        vector<char> sensorMessage = constructSensorMessage(sensors[i]);
        vector<char> sensorMessageLength = IntToByte(sensorMessage.size());
        message.insert(message.end(), sensorMessageLength.begin(), sensorMessageLength.end());
        message.insert(message.end(), sensorMessage.begin(), sensorMessage.end());
    }

    return message;
}