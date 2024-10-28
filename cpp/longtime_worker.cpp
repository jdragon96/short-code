#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

class LongtimeWorkers {
public:
  static std::atomic<bool> closeFlag;

  LongtimeWorkers() { LongtimeWorkers::closeFlag.store(false); }

  static LongtimeWorkers &instance() {
    static LongtimeWorkers inst;
    return inst;
  }

  static LongtimeWorkers Close() { LongtimeWorkers::closeFlag.store(true); }

  bool IsClosed() { return !LongtimeWorkers::closeFlag.load(); }

  void Make(std::function<void()> func) {
    std::thread t(func);
    t.detach();
  }
};
std::atomic<bool> LongtimeWorkers::closeFlag;

int main() {
  std::string name = "Hello";
  LongtimeWorkers::instance().Make([name]() {
    while (LongtimeWorkers::instance().IsClosed()) {
      std::cout << "Longtime thread running" << name << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << name << " : "
              << "종료" << std::endl;
  });
  name = "안녕";
  LongtimeWorkers::instance().Make([name]() {
    while (LongtimeWorkers::instance().IsClosed()) {
      std::cout << "Longtime thread running" << name << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << name << " : "
              << "종료" << std::endl;
  });

  std::this_thread::sleep_for(std::chrono::seconds(3));
  return 0;
}