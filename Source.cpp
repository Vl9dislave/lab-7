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
    void startGenerating() {
        active = true;
        worker = thread([this]() {
            int value = 0;
            while (active) {
                value += 10;
                cout << "\n[" << name << "] Сгенерировал данные: " << value << "\n";

                globalBus.emit("sensor_data", { name, value });

                this_thread::sleep_for(chrono::milliseconds(500));
            }
            });
    }

    void stop() {
        active = false;
        if (worker.joinable()) {
            worker.join();
        }
    }
};

class Logger {
private:
    EventEmitter::ListenerID subId;

public:
    Logger() {
        subId = globalBus.subscribe("sensor_data", [](const Message& msg) {
            cout << "   -> [Logger] Сохранено в лог: " << msg.value << " (источник: " << msg.origin << ")\n";
            });
        cout << "[Logger] Подписался на 'sensor_data' (ID: " << subId << ")\n";
    }

    void stopLogging() {
        globalBus.unsubscribe("sensor_data", subId);
        cout << "[Logger] Отписался от событий.\n";
    }
};

class Controller {
private:
    EventEmitter::ListenerID subId;

public:
    Controller() {
        subId = globalBus.subscribe("sensor_data", [](const Message& msg) {
            if (msg.value >= 30) {
                cout << "   -> [Controller] ВНИМАНИЕ! Критическое значение: " << msg.value << "!\n";
            }
            });
        cout << "[Controller] Подписался на 'sensor_data' (ID: " << subId << ")\n";
    }

    void stopControlling() {
        globalBus.unsubscribe("sensor_data", subId);
        cout << "[Controller] Отписался от событий.\n";
    }
};

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    Logger logger;
    Controller controller;
    Sensor tempSensor("Датчик Температуры");

    tempSensor.startGenerating();

    this_thread::sleep_for(chrono::milliseconds(1200));

    logger.stopLogging();

    this_thread::sleep_for(chrono::milliseconds(1200));

    tempSensor.stop();

    return 0;
}