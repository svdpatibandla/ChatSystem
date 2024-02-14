#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <unistd.h>
#include <random>
#include <jsoncpp/json/json.h>

#define HOST "127.0.0.1"
#define BUFF_SIZE 1024

using namespace std;

void setupRooms(int nrooms, vector<int>& ports) {
    Json::Value rooms;
    vector<int> availablePorts(1000);
    iota(availablePorts.begin(), availablePorts.end(), 5000);
    random_device rd;
    mt19937 g(rd());
    shuffle(availablePorts.begin(), availablePorts.end(), g);

    for (int i = 0; i < nrooms; ++i) {
        string room_name = "Room" + to_string(i + 1);
        int room_port = availablePorts[i];
        string room_pwd = room_name + "@123";

        rooms[room_name]["port"] = room_port;
        rooms[room_name]["password"] = room_pwd;
    }

    ofstream file("rooms.json", ofstream::binary);
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "\t";
    unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(rooms, &file);
    cout << nrooms << " Rooms created" << endl;

    for (int i = 0; i < nrooms; ++i)
        ports.push_back(rooms["Room" + to_string(i + 1)]["port"].asInt());
}

void broadcast(const char* message, vector<int>& clients) {
    for (int client : clients) {
        send(client, message, strlen(message), 0);
    }
}

void handle(int client, vector<int>& clients, vector<string>& users) {
    char buffer[BUFF_SIZE] = {0};
    while (true) {
        int valread = recv(client, buffer, BUFF_SIZE, 0);
        if (valread <= 0) {
            int index = distance(clients.begin(), find(clients.begin(), clients.end(), client));
            clients.erase(clients.begin() + index);
            string user = users[index];
            users.erase(users.begin() + index);
            close(client);
            char left_message[BUFF_SIZE];
            strcpy(left_message, (user + " left!").c_str());
            broadcast(left_message, clients);
            break;
        }
        broadcast(buffer, clients);
        memset(buffer, 0, sizeof(buffer));
    }
}

void receive(int port) {
    vector<int> clients;
    vector<string> users;

    int serverSocket;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cerr << "Socket creation error" << endl;
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Bind failed" << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 5) < 0) {
        cerr << "Listen failed" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Room Initialized..." << endl;

    while (true) {
        int clientSocket;
        if ((clientSocket = accept(serverSocket, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            cerr << "Accept failed" << endl;
            exit(EXIT_FAILURE);
        }

        cout << "Connected with " << inet_ntoa(client_addr.sin_addr) << endl;

        send(clientSocket, "NICK", strlen("NICK"), 0);
        char user_buffer[BUFF_SIZE] = {0};
        recv(clientSocket, user_buffer, BUFF_SIZE, 0);
        string user(user_buffer);
        users.push_back(user);
        clients.push_back(clientSocket);

        cout << "Name is " << user << endl;

        char join_message[BUFF_SIZE];
        strcpy(join_message, (user + " joined!").c_str());
        broadcast(join_message, clients);
        send(clientSocket, "Connected to server!", strlen("Connected to server!"), 0);

        thread clientThread(handle, clientSocket, ref(clients), ref(users));
        clientThread.detach();
    }

    close(serverSocket);
}

int main() {
    int rooms;
    cout << "Enter the number of rooms you want to create: ";
    cin >> rooms;

    vector<int> ports;
    setupRooms(rooms, ports);

    vector<thread> threads;
    for (int port : ports) {
        threads.emplace_back(receive, port);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}
