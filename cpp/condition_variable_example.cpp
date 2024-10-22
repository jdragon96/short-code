/********************************************************************************
* [Producer 쓰레드]
* Thread #1 / Thread #2 / Thread #3 / ... / Thread #N
* 
* notify_one()
*						notify_one()
*												notify_one()
*																					 notify_one()
* -------------------------------------------------------------------------------
* [Consumer 쓰레드]
* - cv.wait()으로 각 쓰레드들을 블로킹
* - Producer에서 notify_one() 실행 새, 임의의 쓰레드 하나가 다시 블로킹 조건 확인
* - notify_all() 실행 시, 모든 쓰레드가 블로킹 조건을 다시 확인
* 
* -------------------------------------------------------------------------------
* [Monitoring 쓰레드]
* - Consumer와 동일한 메커니즘으로 동작
* - notify 수신 시, 조건 확인 후 진행상황을 지속적으로 모니터링
********************************************************************************/

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

void producer(std::queue<std::string>* downloaded_pages, std::mutex* m, int index, std::condition_variable* cv) {
	for (int i = 0; i < 5; i++) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100 * index));
		std::string content = "웹사이트 : " + std::to_string(i) + " from thread(" + std::to_string(index) + ")\n";
		// data 는 쓰레드 사이에서 공유되므로 critical section 에 넣어야 한다.
		m->lock();
		downloaded_pages->push(content);
		m->unlock();
		// cv->wait()으로 대기중인 쓰레드에 일어나라고 알람 발생
		cv->notify_one();
	}
}

void consumer(std::queue<std::string>* downloaded_pages, std::mutex* m, int* num_processed, std::condition_variable* cv) {
	while (*num_processed < 25) {
		std::unique_lock<std::mutex> lock(*m);
		// 탈출조건
		// 1) 다운로드 된 페이지가 1개이상 존재하는 경우
		// 2) 처리된 페이지 개수기 25개인 경우
		cv->wait(lock, [&] {return !downloaded_pages->empty() || *num_processed == 25; });

		if (*num_processed == 25) {
			// 모든 페이지 처리 시, 쓰레드 종료
			lock.unlock();
			return;
		}
		else
		{
			// 아직 처리해야 할 페이지가 남아있는 경우
			std::string content = downloaded_pages->front();
			downloaded_pages->pop();
			(*num_processed)++;
			lock.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}

void monitoring(std::condition_variable* cv, std::mutex* m, int* num_processed)
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(*m);
		cv->wait(lock, [&]()
			{
				std::cout << "Processed number : " << *num_processed << std::endl;
				return *num_processed == 25;
			});

		lock.unlock();
		break;
	}
	std::cout << "모니터링 종료" << std::endl;
}

int main() {
	std::queue<std::string> downloaded_pages;
	std::mutex m;
	std::condition_variable cv;

	std::vector<std::thread> producers;
	for (int i = 0; i < 5; i++) {
		producers.push_back(
			std::thread(producer, &downloaded_pages, &m, i + 1, &cv));
	}

	int num_processed = 0;
	std::vector<std::thread> consumers;
	for (int i = 0; i < 3; i++) {
		consumers.push_back(
			std::thread(consumer, &downloaded_pages, &m, &num_processed, &cv));
	}
	auto monitor = std::thread(monitoring, &cv, &m, &num_processed);

	// 모든 페이지가 다운로드 되기 전 까지 대기
	for (int i = 0; i < 5; i++) {
		producers[i].join();
	}

	// 나머지 자고 있는 쓰레드들을 모두 깨운다.
	cv.notify_all();

	for (int i = 0; i < 3; i++) {
		consumers[i].join();
	}
	monitor.join();
}