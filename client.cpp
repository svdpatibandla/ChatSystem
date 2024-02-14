#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <jsoncpp/json/json.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFF_SIZE 1024

using namespace std;

void receive(int clientSocket) {
    char buffer[BUFF_SIZE] = {0};
    while (true) {
        int valread = recv(clientSocket, buffer, BUFF_SIZE, 0);
        if (valread <= 0) {
            cout << "Server disconnected" << endl;
            break;
        }
        cout << buffer << endl;
        memset(buffer, 0, sizeof(buffer));
    }
}

void write(int clientSocket) {
    string user;
    cout << "Enter your name: ";
    cin >> user;
    string message;
    while (true) {
        cout << user << ": ";
        getline(cin, message);
        send(clientSocket, message.c_str(), message.length(), 0);
    }
}

int main() {
    int clientSocket = 0;
    struct sockaddr_in serv_addr;
    Json::Value rooms;
    Json::Reader reader;
    ifstream file("rooms.json", ifstream::binary);
    bool parsingSuccessful = reader.parse(file, rooms, false);
    if (!parsingSuccessful) {
        cout << "Failed to parse JSON file." << endl;
        return 1;
    }
    file.close();

    cout << "Available Rooms:" << endl;
    for (const auto& room : rooms.getMemberNames()) {
        cout << room << endl;
    }

    string room_name;
    cout << "Enter the room you want to enter: ";
    cin >> room_name;

    bool validflag = false;
    for (int i = 0; i < 3; ++i) {
        string room_pwd;
        cout << "Enter the password for " << room_name << ": ";
        cin >> room_pwd;
        if (room_pwd == rooms[room_name]["password"].asString()) {
            cout << "\nEntering " << room_name << "...\n" << endl;
            validflag = true;
            break;
        } else {
            cout << "Access Denied! Try again." << endl;
        }
    }

    if (!validflag) {
        cout << "Try again with valid password" << endl;
        exit(EXIT_FAILURE);
    }

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Socket creation error" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(rooms[room_name]["port"].asString().c_str()));

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        cout << "Invalid address/ Address not supported" << endl;
        return -1;
    }

    if (connect(clientSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Connection Failed" << endl;
        return -1;
    }

    thread receiveThread(receive, clientSocket);
    thread writeThread(write, clientSocket);

    receiveThread.join();
    writeThread.join();

    close(clientSocket);
    return 0;
}
