#include<iostream>
#include<pthread.h>
#include<errno.h>
#include<unistd.h>
#include<semaphore.h>
#include<time.h>
#include<vector>
#include<unordered_set>
#include<unordered_map>
#include<cstring>
#include<stdlib.h>
#include<chrono>
#include<random>
#include<algorithm>

using namespace std;

#define NO_OF_EVENTS 110
#define CAPACITY 500
#define NUM_THREADS 20
#define MAX 5
#define T 60

int seats_available(int,int);
bool book_k_tickets(int,int,vector<vector<int>>,int);
int cancel_ticket(int,int,int);

unsigned int generateSeed() {	//generate a seed based on current time
    auto now = std::chrono::high_resolution_clock::now();
    return static_cast<unsigned int>(now.time_since_epoch().count());
}

int counter=0;

struct ThreadManager {
	int active_queries;
	pthread_mutex_t lock;
	sem_t semaphore;

} manager = {0, PTHREAD_MUTEX_INITIALIZER};

pthread_mutex_t mssglock;
/*struct event {
	int capacity;
	event() {}
	event(int cap) : capacity(cap) {}
} events[] ;
*/
int seats[NO_OF_EVENTS][CAPACITY+1];

//pthread_mutex_t eventlock;

struct thread_info {
	int tid;
	vector<pair<int,vector<int>>> bookings;
	//int running_time=0;
} ;
enum query {R,W};

struct EventManager {
	int e,tid; query q;
	EventManager() : e(0),tid(0) {}
	EventManager(int e,int tid,query q) : e(e),tid(tid),q(q) {}
} shared_table[MAX] ;
//struct timespec timeout[5];

pthread_rwlock_t rwlock; //int tableptr=0;

int seats_available(int e,int tid){
	bool status=1; int available=-1,tableptr=-1;
	
	pthread_rwlock_wrlock(&rwlock);
	EventManager r_entry={e+1,tid,R};
	for(int i=0;i<MAX;i++){
		if(shared_table[i].tid==0){
			shared_table[i]=r_entry;
			tableptr=i;
			break;
		}
	}
	pthread_rwlock_unlock(&rwlock);
	
	pthread_rwlock_rdlock(&rwlock);
	//srand(time(NULL));
	mt19937 rng(generateSeed());
	uniform_int_distribution<int> distribution(1, 3);
	int t=distribution(rng);
	
	usleep(t*100000);
	for(int i=0;i<MAX;i++){
		if(shared_table[i].e==e+1 && shared_table[i].q==W && shared_table[i].tid!=tid){ status=0; break; }
	}
	//tableptr--;
	if(status) available=seats[e][CAPACITY];
	pthread_rwlock_unlock(&rwlock);
	
	pthread_rwlock_wrlock(&rwlock);
	shared_table[tableptr]=EventManager();
	pthread_rwlock_unlock(&rwlock);
	
	return available;
}
bool book_k_tickets(int e,int k,vector<pair<int,vector<int>>>& bookings,int tid){
	bool status=1; int tableptr=-1;
	pthread_rwlock_wrlock(&rwlock);
	EventManager r_entry={e+1,tid,R};
	for(int i=0;i<MAX;i++){
		if(shared_table[i].tid==0){
			shared_table[i]=r_entry;
			tableptr=i;
			break;
		}
	}
	pthread_rwlock_unlock(&rwlock);
	
	pthread_rwlock_rdlock(&rwlock);
	
	mt19937 rng(generateSeed());
	uniform_int_distribution<int> distribution(1, 3);
	int t=distribution(rng);
	usleep(t*100000);
	
	for(int i=0;i<MAX;i++){
		if(shared_table[i].e==e+1 && (shared_table[i].q==R || shared_table[i].q==W) && shared_table[i].tid!=tid){ status=0; break; }
	}
	if(status){
		if(seats[e][CAPACITY]<k) status=0;
	}
	pthread_rwlock_unlock(&rwlock);
	
	pthread_rwlock_wrlock(&rwlock);
	EventManager w_entry={e+1,tid,W};
	shared_table[tableptr]=(status ? w_entry : EventManager());
	pthread_rwlock_unlock(&rwlock);
	
	if(!status) return 0;
	
	rng.seed(generateSeed());
	distribution = uniform_int_distribution<int>(1, 5);
	t=distribution(rng);
	usleep(t*100000);
	
	//for(int t : tickets) seats[e][t-1]=1;
	seats[e][CAPACITY]-=k;
	int j=-1;
	for(j=0;j<bookings.size();j++){
		if(bookings[j].first==e+1) break;
	}
	if(j==bookings.size()){
		pair<int,vector<int>> booking;
		booking.first=e+1;
		bookings.push_back(booking);
	}
	for(int s=0;s<CAPACITY && k;s++){ if(!seats[e][s]){ seats[e][s]=1; bookings[j].second.push_back(s+1); k--; } } 
	
	pthread_rwlock_wrlock(&rwlock);
	shared_table[tableptr]=EventManager();
	pthread_rwlock_unlock(&rwlock);
	
	return 1;
}
bool cancel_ticket(int e,int s,vector<pair<int,vector<int>>>& bookings,int tid){
	bool status=1; int tableptr=-1;
	pthread_rwlock_wrlock(&rwlock);
	EventManager r_entry={e,tid,R};
	for(int i=0;i<MAX;i++){
		if(shared_table[i].tid==0){
			shared_table[i]=r_entry;
			tableptr=i;
			break;
		}
	}
	pthread_rwlock_unlock(&rwlock);
	
	pthread_rwlock_rdlock(&rwlock);
	
	mt19937 rng(generateSeed());
	uniform_int_distribution<int> distribution(1, 3);
	int t=distribution(rng);
	usleep(t*100000);
	
	for(int i=0;i<MAX;i++){
		if(shared_table[i].e==e && (shared_table[i].q==R || shared_table[i].q==W) && shared_table[i].tid!=tid){ status=0; break; }
	}
	pthread_rwlock_unlock(&rwlock);
	
	pthread_rwlock_wrlock(&rwlock);
	EventManager w_entry={e,tid,W};
	shared_table[tableptr]=(status ? w_entry : EventManager());
	pthread_rwlock_unlock(&rwlock);
	
	if(!status) return 0;
	
	rng.seed(generateSeed());
	distribution = uniform_int_distribution<int>(1, 5);
	t=distribution(rng);
	usleep(t*100000);
	
	seats[e-1][s-1]=0;
	seats[e-1][CAPACITY]+=1;
	
	for(int i=0;i<bookings.size();i++){
		if(bookings[i].first==e){ 
			bookings[i].second.erase(find(bookings[i].second.begin(),bookings[i].second.end(),s));
			if(bookings[i].second.empty())	bookings.erase(bookings.begin()+i);
			break;
		}
	}
	pthread_rwlock_wrlock(&rwlock);
	shared_table[tableptr]=EventManager();
	pthread_rwlock_unlock(&rwlock);
	
	return 1;
}
void* thread_worker(void* arg){
	
	thread_info* info = (thread_info*)arg;
	 
	int tid=info->tid; vector<pair<int,vector<int>>> bookings=info->bookings;
	
	struct timespec start_time, current_time, timeout;
    clock_gettime(CLOCK_REALTIME, &timeout); // Record start time
	
    // Calculate timeout for sem_timedwait
    timeout.tv_sec += T;
    //timeout[tid-1].tv_sec=time(NULL)+5;
    
    /*pthread_mutex_lock(&mssglock);
    cout<<tid-1<<" "<<timeout.tv_sec<<endl;
    pthread_mutex_unlock(&mssglock);*/
    //timeout.tv_nsec = start_time.tv_nsec;
	
	while(1){
		pthread_mutex_lock(&mssglock);
		//cout<<"Active queries "<<manager.active_queries<<endl;
		if(manager.active_queries==MAX) cout<<"\nThread "<<tid<<" waiting..\n";
		pthread_mutex_unlock(&mssglock);
		
		//sem_wait(&manager.semaphore);
		if(sem_timedwait(&manager.semaphore,&timeout)==-1){
			if (errno == ETIMEDOUT){
			pthread_mutex_lock(&mssglock);
			cout<<"\nThread "<<tid<<" timed out!\n";
			pthread_mutex_unlock(&mssglock);
			break;
			}
			else perror("sem_timedwait");
		}
		clock_gettime(CLOCK_REALTIME, &current_time);
		if(current_time.tv_sec>=timeout.tv_sec){ 
			pthread_mutex_lock(&mssglock);
			cout<<"\nThread "<<tid<<" timed out!\n";
			pthread_mutex_unlock(&mssglock);
			break;
		}
		//cout<<"Thread "<<*tid<<" working\n";
		pthread_mutex_lock(&manager.lock);
		manager.active_queries++;
		pthread_mutex_unlock(&manager.lock);
		if(manager.active_queries>MAX) cout<<"limit reached\n";
		//cout<<manager.active_queries<<endl;
		srand(generateSeed());
		int qtype=rand()%3;
		switch(qtype){
			case 0:  
			{
				int e=rand()%NO_OF_EVENTS; 
				int available=seats_available(e,tid);
				pthread_mutex_lock(&mssglock);
				cout<<"\nThread "<<tid<<" query : No of seats available in event "<<(e+1)<<endl;
				//cout<<seats[e][CAPACITY]<<endl;
				cout<<"Query output : ";
				if(available==-1) cout<<"Failed due to read/write conflict!"<<endl;
				else cout<<available<<" seats available\n";
				pthread_mutex_unlock(&mssglock);
				break;
			}	
			case 1:
			{  
				int e=rand()%NO_OF_EVENTS; int k=5+rand()%6;
				bool booked = book_k_tickets(e,k,bookings,tid);
				pthread_mutex_lock(&mssglock);
				cout<<"\nThread "<<tid<<" query : Book "<<k<<" tickets in event "<<(e+1)<<endl;
				//cout<<seats[100][CAPACITY]<<endl;
				cout<<"Query output : ";
				if(booked) cout<<k<<" tickets booked in event "<<(e+1)<<endl;
				else cout<<"Failed due to read/write or write/write conflict!"<<endl;
				pthread_mutex_unlock(&mssglock);
				break;
			}
			case 2:
			{
				if(bookings.empty()) break;
				int i=rand()%bookings.size();
				if(bookings[i].second.empty()){ bookings.erase(bookings.begin()+i); break; }
				int j=rand()%(bookings[i].second.size());
				bool canceled = cancel_ticket(bookings[i].first,bookings[i].second[j],bookings,tid);
				pthread_mutex_lock(&mssglock);
				cout<<"\nThread "<<tid<<" query : Cancel booking for seat "<<bookings[i].second[j]<<" in event "<<bookings[i].first<<endl;
				cout<<"Query output : ";
				if(canceled) cout<<" ticket for seat "<<bookings[i].second[j]<<" in event "<<bookings[i].first<<" canceled\n";
				else cout<<"Failed due to read/write or write/write conflict!"<<endl;
				pthread_mutex_unlock(&mssglock);
				break;
			}
		}
		/*srand(time(NULL));
		usleep((rand()%5+1)*1000);*/
		
		pthread_mutex_lock(&manager.lock);
		manager.active_queries--;
		pthread_mutex_unlock(&manager.lock);
		sem_post(&manager.semaphore);
		sleep(1);
	}
	return NULL;
}
int main(){
	pthread_rwlock_init(&rwlock, NULL);
	pthread_mutex_init(&mssglock, NULL);	
	//int events[10];
	
	memset(seats,0,sizeof(seats));
	
	for(int e=0;e<NO_OF_EVENTS;e++)
		seats[e][CAPACITY]=CAPACITY;
	
	//cout<<seats[100][CAPACITY]<<endl;
	pthread_t threads[NUM_THREADS];
	
	thread_info info[NUM_THREADS];
	
    sem_init(&manager.semaphore, 0, MAX);
 	
 	for (int i = 0; i < NUM_THREADS; i++) {
		info[i].tid=i+1;
        int status = pthread_create(&threads[i], NULL, thread_worker, &info[i]);
		if(status){
			errno=status;
			perror("Error creating thread!"); 
			exit(EXIT_FAILURE);
		}
    }
    for(int i = 0; i < NUM_THREADS;i++)
    	pthread_join(threads[i],NULL);

	sem_destroy(&manager.semaphore);
	    
    return 0;
}
