#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <random>
#include <queue>

using namespace std;

constexpr int NUM_SENSORS = 8;
constexpr int READINGS_PER_HOUR = 60; // 1 reading per minute
constexpr int HOURS_IN_DAY = 24;
constexpr int SIMULATED_MINUTES_PER_SECOND = 60;

// structure to hold temperature reading
struct TemperatureReading {
    double temperature;
    chrono::system_clock::time_point timestamp;
};

// queue for storing temperature readings
class ReadingQueue {
private:
    queue<TemperatureReading> readings;
    mutex mtx;

public:
    // add a temperature reading to the queue
    void addReading(const TemperatureReading& reading) {
        lock_guard<mutex> lock(mtx);
        readings.push(reading);
    }

    vector<TemperatureReading> getReadingsForHour(int hour) {
        lock_guard<mutex> lock(mtx);
        vector<TemperatureReading> hourReadings;
        while (!readings.empty()) {
            auto frontHour = chrono::time_point_cast<chrono::hours>(readings.front().timestamp).time_since_epoch().count();
            if (frontHour == hour) {
                hourReadings.push_back(readings.front());
            }
            readings.pop();
        }
        // restore remaining readings to the original queue
        queue<TemperatureReading> tempQueue;
        swap(readings, tempQueue);
        while (!tempQueue.empty()) {
            readings.push(tempQueue.front());
            tempQueue.pop();
        }
        return hourReadings;
    }
};

// function to generate random temperature readings between -100 and 70
double generateTemperature() {
    static default_random_engine generator(chrono::system_clock::now().time_since_epoch().count());
    static uniform_real_distribution<double> distribution(-100.0, 70.0);
    return distribution(generator);
}

// function to simulate temperature sensor thread
void temperatureSensorThread(int sensorId, ReadingQueue& queue) {
    while (true) {
        TemperatureReading reading;
        reading.temperature = generateTemperature();
        reading.timestamp = chrono::system_clock::now();
        queue.addReading(reading);
        // Sleep for 1 minutue
        this_thread::sleep_for(chrono::seconds(1));
    }
}

// function to compile hourly report
void hourlyReportThread(ReadingQueue& queue) {

    int reportCount = 0;

    while (reportCount < 10) {
        // get current hour
        auto currentHour = chrono::time_point_cast<chrono::hours>(chrono::system_clock::now()).time_since_epoch().count();
        
        // get all readings for the current hour
        auto hourReadings = queue.getReadingsForHour(currentHour);
        
        // sort readings by temperature
        sort(hourReadings.begin(), hourReadings.end(), [](const TemperatureReading& a, const TemperatureReading& b) {
            return a.temperature < b.temperature;
        });

        // get top 5 highest and lowest temperatures
        vector<TemperatureReading> top5Highest(hourReadings.begin(), hourReadings.begin() + min<size_t>(5, hourReadings.size()));
        vector<TemperatureReading> top5Lowest(hourReadings.end() - min<size_t>(5, hourReadings.size()), hourReadings.end());

        // find 10-second interval with largest temperature difference
        double maxDifference = 0.0;
        chrono::system_clock::time_point startTime, endTime;
        for (size_t i = 0; i < hourReadings.size() - 1; ++i) {
            auto difference = abs(hourReadings[i + 1].temperature - hourReadings[i].temperature);
            if (difference > maxDifference) {
                maxDifference = difference;
                startTime = hourReadings[i].timestamp;
                endTime = hourReadings[i + 1].timestamp;
            }
        }

        // print the hourly report
        cout << "Hourly Report for Hour " << currentHour << ":\n";
        cout << "Top 5 Highest Temperatures:\n";
        for (const auto& reading : top5Highest) {
            cout << "Temperature: " << reading.temperature << "F, Timestamp: " 
                 << chrono::system_clock::to_time_t(reading.timestamp) << "\n";
        }
        cout << "Top 5 Lowest Temperatures:\n";
        for (const auto& reading : top5Lowest) {
            cout << "Temperature: " << reading.temperature << "F, Timestamp: " 
                 << chrono::system_clock::to_time_t(reading.timestamp) << "\n";
        }
        cout << "10-Minute Interval with Largest Temperature Difference:\n";
        cout << "Start Time: " << chrono::system_clock::to_time_t(startTime) << ", End Time: " 
             << chrono::system_clock::to_time_t(endTime) << "\n";

        // sleep for 1 second
        this_thread::sleep_for(chrono::seconds(1));

        reportCount++;

        if (reportCount == 10)
            exit(0);
    }
}

int main() {
    // create queue for storing temperature readings
    ReadingQueue queue;

    // create threads for temperature sensors
    vector<thread> sensorThreads;
    for (int i = 0; i < NUM_SENSORS; ++i) {
        sensorThreads.emplace_back(temperatureSensorThread, i + 1, ref(queue));
    }

    // create thread for hourly report
    thread reportThread(hourlyReportThread, ref(queue));

    // join sensor threads and report thread
    for (auto& t : sensorThreads) {
        t.join();
    }
    reportThread.join();

    return 0;
}
