#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define QUEUE_MAX_SIZE 4 
#define ITER_MAX 15 // liczba iteracji producenta i konsumenta

struct producer_args {
	int queue[QUEUE_MAX_SIZE];
	int queue_start;
	int queue_size;
	char name;
	pthread_cond_t not_full;
} producerA, producerB;

pthread_mutex_t mutex;
pthread_t producerA_thread, producerB_thread, consumerA_thread, consumerB_thread;

void push(int element, struct producer_args* prod){ // dodawanie na koniec kolejki
	prod->queue[(prod->queue_start + prod->queue_size++) % QUEUE_MAX_SIZE] = element;
}

int pop(struct producer_args* prod){ // usuwanie i zwracanie pierwszego elementu z kolejki
	int element = prod->queue[prod->queue_start];
	prod->queue_start = (prod->queue_start+1) % QUEUE_MAX_SIZE;
	prod->queue_size--;
	return element;
}

void* producer(void* args){
	struct producer_args *prod = (struct producer_args*) args;	
	for(;;){
		pthread_mutex_lock(&mutex); // blokowanie mutexu, aby konsument nie pobieral w momencie dodawania przez producenta
	
		if (prod->queue_size == QUEUE_MAX_SIZE){ 
			printf("Producent %c: nie dodano produktu - bufor pelny\n", prod->name);
			pthread_cond_wait(&prod->not_full, &mutex); // oczekiwanie na wolne miejsce w kolejce, po sygnale mutex zostanie opuszczony
		}
		int value = rand()%100; // losowanie liczby z przedzialu 0-99, wybor przedzialu ze wzgledu na czytelnosc
		push(value, prod);
		printf("Producent %c: dodano produkt (%d)\n", prod->name, value);
		
		pthread_mutex_unlock(&mutex); // zwolnienie mutexu, konsument i producent ponownie moga miec dostep do kolejki
		//sleep(1);
	}
}

void* consumer(void* args){
	char* name = (char*) args;
	for(;;){
		pthread_mutex_lock(&mutex);  // blokowanie mutexu, aby producent nie produkowal w momencie pobierania przez konsumenta
		
		if (producerA.queue_size > 0 || producerB.queue_size > 0){ // sprawdzanie czy obie kolejki sa niepuste
			int elementA = pop(&producerA);
			int elementB = pop(&producerB);
			
			printf("Konsument %s: pobrano produkt (%d) z kolejki A oraz pobrano produkt (%d) z kolejki B\n", name, elementA, elementB);
			
			if (producerA.queue_size == QUEUE_MAX_SIZE - 1){
				pthread_cond_signal(&producerA.not_full); // sygnalizowanie ze kolejka nie jest pelna (budzenie czekajacego producenta A)
			}
			if (producerB.queue_size == QUEUE_MAX_SIZE - 1){
				pthread_cond_signal(&producerB.not_full); // sygnalizowanie ze kolejka nie jest pelna (budzenie czekajacego producenta B)
			}
		}
		else{
			printf("Konsument %s: nie pobrano produktu - bufor pusty\n", name);
		}		
		pthread_mutex_unlock(&mutex); // zwolnienie mutexu, konsumenti i producenci ponownie moga miec dostep do kolejki
		//sleep(3);
	}
}

int main(){
	srand(time(NULL));
	producerA.name = 'A';
	producerB.name = 'B';
	
	pthread_mutex_init(&mutex, NULL); // inicjalizacja mutexu, atrybut NULL = default
	
	// inicjalizacja zmiennych warunkowych
	pthread_cond_init(&producerA.not_full, NULL);
	pthread_cond_init(&producerB.not_full, NULL);
	
	pthread_create(&producerA_thread, NULL, &producer, &producerA); // stworzenie i uruchomienie watku producenta
	pthread_create(&producerB_thread, NULL, &producer, &producerB); // stworzenie i uruchomienie watku producenta
	pthread_create(&consumerA_thread, NULL, &consumer, "A"); // stworzenie i uruchomienie watku konsumenta
	pthread_create(&consumerB_thread, NULL, &consumer, "B"); // stworzenie i uruchomienie watku konsumenta
	
	//program bedzie czekal na zakonczenie tych watkow
	pthread_join(producerA_thread, NULL);
	pthread_join(producerB_thread, NULL); 
	pthread_join(consumerA_thread, NULL);
	pthread_join(consumerB_thread, NULL);
	
	//zwolnienie mutexu
	pthread_mutex_destroy(&mutex);	
	return 0;
}