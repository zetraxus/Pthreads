#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define QUEUE_MAX_SIZE 4 
#define ITER_MAX 15 // liczba iteracji producenta i konsumenta

// definicja kolejki FIFO (bufor cykliczny)
int queue[QUEUE_MAX_SIZE];
int queue_start = 0;
int queue_size = 0;

pthread_mutex_t mutex;
pthread_t producer_thread, consumer_thread;

void push(int element){ // dodawanie na koniec kolejki
	queue[(queue_start + queue_size++) % QUEUE_MAX_SIZE] = element;
}

int pop(){ // usuwanie i zwracanie pierwszego elementu z kolejki
	int element = queue[queue_start];
	queue_start = (queue_start+1) % QUEUE_MAX_SIZE;
	queue_size--;
	return element;
}

void* producer(void* args){	
	for(;;){
		pthread_mutex_lock(&mutex); // blokowanie mutexu, aby konsument nie pobieral w momencie dodawania przez producenta
	
		if (queue_size < QUEUE_MAX_SIZE){ 
			int value = rand()%100; // losowanie liczby z przedzialu 0-99, wybor przedzialu ze wzgledu na czytelnosc
			push(value);
			printf("dodano produkt (%d)\n", value);
		}
		else {
			printf("nie dodano produktu - bufor pelny\n");
		}
		
		pthread_mutex_unlock(&mutex); // zwolnienie mutexu, konsument i producent ponownie moga miec dostep do kolejki
	//	sleep(1);
	}
}

void* consumer(void* args){
	for(;;){
		pthread_mutex_lock(&mutex);  // blokowanie mutexu, aby producent nie produkowal w momencie pobierania przez konsumenta
		
		if (queue_size > 0){
			int element = pop();
			printf("pobrano produkt (%d)\n", element);
		}
		else {
			printf ("nie pobrano produktu - bufor pusty\n");
		}
				
		pthread_mutex_unlock(&mutex); // zwolnienie mutexu, konsument i producent ponownie moga miec dostep do kolejki
		//sleep(3);
	}
}

int main(){
	srand(time(NULL));
	
	pthread_mutex_init(&mutex, NULL); // inicjalizacja mutexu, atrybut NULL = default
	
	pthread_create(&producer_thread, NULL, &producer, NULL); // stworzenie i uruchomienie watku producenta
	pthread_create(&consumer_thread, NULL, &consumer, NULL); // stworzenie i uruchomienie watku konsumenta
	
	//program bedzie czekal na zakonczenie tych watkow
	pthread_join(producer_thread, NULL); 
	pthread_join(consumer_thread, NULL);
	
	//zwolnienie mutexu
	pthread_mutex_destroy(&mutex);	
	return 0;
}