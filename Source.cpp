#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <atomic>
#include <Windows.h>

using namespace std;

struct Message {
    string origin;
    int value;
};

class EventEmitter {
public:
    using ListenerID = int;
    using Callback = function<void(const Message&)>;

    ListenerID subscribe(const string& eventName, Callback cb) {
        ListenerID id = nextId++;
        listeners[eventName][id] = cb;
        return id;
    }

    void unsubscribe(const string& eventName, ListenerID id) {
        if (listeners.count(eventName)) {
            listeners[eventName].erase(id);
        }
    }

    void emit(const string& eventName, const Message& msg) {
        if (listeners.count(eventName)) {
            auto currentListeners = listeners[eventName];
            for (const auto& pair : currentListeners) {
                pair.second(msg);
            }
        }
    }

private:
    unordered_map<string, map<ListenerID, Callback>> listeners;
    ListenerID nextId = 0;
};

EventEmitter globalBus;

class Sensor {
private:
    string name;
    atomic<bool> active;
    thread worker;

public:
    Sensor(const string& sensorName) : name(sensorName), active(false) {}

    ~Sensor() {
        stop();
    }
