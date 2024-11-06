#include <iostream>
#include <vector>
#include <random>
#include <fstream>
#include <algorithm>
#include <limits.h>
using namespace std;

struct TaskDetails {
    int taskId;
    int arrivalTime, coreRequirement, memoryRequirement, execTime;
    TaskDetails(int id, int at, int cr, int mr, int et)
        : taskId(id), arrivalTime(at), coreRequirement(cr), memoryRequirement(mr), execTime(et) {}
    int calculateValue() const { return coreRequirement * memoryRequirement * execTime; }
};

struct Server {
    int id, availableCores = 24, availableMemory = 64;
    int totalCores = 24, totalMemory = 64;
    Server(int id) : id(id) {}
    bool allocateResources(TaskDetails& task) {
        if (availableCores >= task.coreRequirement && availableMemory >= task.memoryRequirement) {
            availableCores -= task.coreRequirement;
            availableMemory -= task.memoryRequirement;
            return true;
        }
        return false;
    }
    void releaseResources(TaskDetails& task) {
        availableCores += task.coreRequirement;
        availableMemory += task.memoryRequirement;
    }
    double getCpuUtilization() const {
        return (1.0 - double(availableCores) / totalCores) * 100;
    }
    double getMemoryUtilization() const {
        return (1.0 - double(availableMemory) / totalMemory) * 100;
    }
};

class TaskScheduler {
private:
    vector<Server> servers;
    vector<TaskDetails> taskList;
    const int retryLimit = 5;
    int timeTracker = 0;

    void sortTasksByArrivalTime() {
        sort(taskList.begin(), taskList.end(), [](const TaskDetails& a, const TaskDetails& b) {
            return a.arrivalTime < b.arrivalTime;
        });
    }

    void sortTasksByResourceValue() {
        sort(taskList.begin(), taskList.end(), [](const TaskDetails& a, const TaskDetails& b) {
            return a.calculateValue() < b.calculateValue();
        });
    }

    void sortTasksByDuration() {
        sort(taskList.begin(), taskList.end(), [](const TaskDetails& a, const TaskDetails& b) {
            return a.execTime < b.execTime;
        });
    }

    Server* allocateUsingFirstFit(TaskDetails& task) {
        for (auto& server : servers) {
            if (server.allocateResources(task)) return &server;
        }
        return nullptr;
    }

    Server* allocateUsingBestFit(TaskDetails& task) {
        Server* bestFit = nullptr;
        int leastWaste = INT_MAX;
        for (auto& server : servers) {
            if (server.allocateResources(task)) {
                int waste = (server.availableCores - task.coreRequirement) + (server.availableMemory - task.memoryRequirement);
                if (waste < leastWaste) {
                    leastWaste = waste;
                    bestFit = &server;
                }
                server.releaseResources(task);
            }
        }
        return bestFit;
    }

    Server* allocateUsingWorstFit(TaskDetails& task) {
        Server* worstFit = nullptr;
        int maxWaste = 0;
        for (auto& server : servers) {
            if (server.allocateResources(task)) {
                int waste = (server.availableCores - task.coreRequirement) + (server.availableMemory - task.memoryRequirement);
                if (waste > maxWaste) {
                    maxWaste = waste;
                    worstFit = &server;
                }
                server.releaseResources(task);
            }
        }
        return worstFit;
    }

public:
    TaskScheduler(int numServers) {
        for (int i = 0; i < numServers; i++) servers.emplace_back(i);
    }

    void addTask(TaskDetails task) { taskList.push_back(task); }

    void processTasks(string schedulingOrder, string fittingStrategy, ofstream& output) {
        if (schedulingOrder == "FCFS") {
            sortTasksByArrivalTime();
        } else if (schedulingOrder == "smallest") {
            sortTasksByResourceValue();
        } else if (schedulingOrder == "duration") {
            sortTasksByDuration();
        }

        while (!taskList.empty()) {
            timeTracker++;
            vector<TaskDetails> pendingTasks;

            for (auto& task : taskList) {
                bool scheduled = false;
                for (int retries = 0; retries < retryLimit && !scheduled; retries++) {
                    Server* selectedServer = nullptr;
                    if (fittingStrategy == "first") {
                        selectedServer = allocateUsingFirstFit(task);
                    } else if (fittingStrategy == "best") {
                        selectedServer = allocateUsingBestFit(task);
                    } else if (fittingStrategy == "worst") {
                        selectedServer = allocateUsingWorstFit(task);
                    }
                    if (selectedServer) {
                        cout << "TaskId: " << task.taskId 
                             << " Arrival Day: " << task.arrivalTime / 24 
                             << " Time Hour: " << task.arrivalTime % 24 
                             << " MemReq: " << task.memoryRequirement 
                             << " CPUReq: " << task.coreRequirement 
                             << " ExeTime: " << task.execTime << endl;
                        scheduled = true;
                        break;
                    }
                }
                if (!scheduled) {
                    pendingTasks.push_back(task);
                }
            }

            taskList = pendingTasks;
            recordResourceUtilization(output);
        }
    }

    void recordResourceUtilization(ofstream& output) {
        double totalCpuUsage = 0.0, totalMemUsage = 0.0;
        for (const auto& server : servers) {
            totalCpuUsage += server.getCpuUtilization();
            totalMemUsage += server.getMemoryUtilization();
        }
        output << timeTracker << "," << totalCpuUsage / servers.size() << "," 
               << totalMemUsage / servers.size() << endl;
    }
};

void generateTaskDetails(TaskScheduler& scheduler, int numTasks) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> arrival(0, 10), cores(1, 24), memory(1, 20), execution(1, 5);
    for (int i = 0; i < numTasks; i++) {
        scheduler.addTask(TaskDetails(i, arrival(gen), cores(gen), memory(gen), execution(gen)));
    }
}

int main() {
    TaskScheduler scheduler(128);
    generateTaskDetails(scheduler, 5000);

    ofstream output("utilization.csv");
    output << "Time,CPU Utilization,Memory Utilization\n";

    scheduler.processTasks("FCFS", "first", output);
    scheduler.processTasks("smallest", "best", output);
    scheduler.processTasks("duration", "worst", output);

    output.close();
    
    return 0;
}
