#ifndef WORK_QUEUE_H_
#define WORK_QUEUE_H_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <memory>

/*
 * this class represent a queue that hold works for worker threads.
 */
template <class T>
class WorkQueue{
public:
	//constructor
	WorkQueue(){}
	
	//add work to the queue
	void add(T work){
		
	   //add this work to the end of the queue
	   std::lock_guard<std::mutex> guard(_qMutex);
	  _workQueue.push(work); 
		  
	   //_condition.notify_all();
	   _condition.notify_one();
	}
	
	//get the next available work on the queue
	T get(){
		
		std::lock_guard<std::mutex> guard(_qMutex);
		  
		if(_workQueue.empty()) return nullptr;
		  
		T newWork=_workQueue.front();
	        _workQueue.pop();
		  
		return newWork;
	}
	
	//check if the queue is empty
	bool isEmpty(){return _workQueue.empty();}
	
	void waitForWork(){
		
	     std::unique_lock<std::mutex> lk(_qMutex);
             _condition.wait(lk, [this]{return !isEmpty();});
	}
	
	int size(){
		return (int)_workQueue.size();
	}

	T top(){
		
		std::lock_guard<std::mutex> guard(_qMutex);
		  
		if(_workQueue.empty()) return nullptr;
		return _workQueue.front();
	}
	
  void close(){_condition.notify_all();}

  void clear(){
      std::unique_lock<std::mutex> lk(_qMutex);
      while(_workQueue.empty()==false){
	  _workQueue.pop();
      }
  }

private:
	std::queue<T>                _workQueue;
	std::condition_variable      _condition;
	std::mutex                   _qMutex;
};

class Work;
using Work_ptr=std::shared_ptr<Work>;
typedef std::shared_ptr<WorkQueue <Work_ptr> >       WorkQueue_ptr;


#endif  // WORK_QUEUE_H_
