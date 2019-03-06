#include <stdio.h>
#include <pthread.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/dispatch.h>
#include <time.h>
#include <signal.h>

// definicje kodow pulsow
#define PULSE_CODE_1 1
#define PULSE_CODE_2 2

// deklaracje id kanalu, polaczenia
int channel_id;
int connection_id;

timer_t t1, t2, t3, t4; // deklaracja timerow
pthread_mutex_t terminal_mutex; // deklaracja mutexu

struct timespec start_time;

// funkcja wypisujaca wiadomosc, przekazana w argumencie
// funkcja korzysta z mutexu, aby kilka watkow nie wypisywalo na terminal w tym samym momencie
// funkcja wyswietla rowniez czas (format- sekundy.nanosekudny od 1970)
void print(char* message){
	struct timespec current_time;
	clock_gettime(CLOCK_REALTIME, &current_time);
	long sec, nsec;
	sec = current_time.tv_sec - start_time.tv_sec;
	nsec = current_time.tv_nsec - start_time.tv_nsec;
	if(nsec < 0){
		nsec += 1000000000;
		sec -= 1;
	}
	pthread_mutex_lock(&terminal_mutex);
	printf("%d.%.9d: %s\n", sec, nsec, message);
	fflush(stdout);
	pthread_mutex_unlock(&terminal_mutex);
}

//watek T1
void thread_print(){
	print("T1");
}

// tworzenie timera t1
void t1_timer(){
	struct sigevent event;
	SIGEV_THREAD_INIT(&event, (void*) &thread_print, NULL, NULL);
	timer_create(CLOCK_REALTIME, &event, &t1);
	struct itimerspec timer = {0,500000000,0,500000000};
	timer_settime(t1, NULL, &timer, NULL);
}

// tworzenie timera t2
void t2_timer(){
	struct sigevent event;
	SIGEV_PULSE_INIT(&event, connection_id, SIGEV_PULSE_PRIO_INHERIT, PULSE_CODE_1, NULL);
	timer_create(CLOCK_REALTIME, &event, &t2);
	struct itimerspec timer = {2,0,2,0};
	timer_settime(t2, NULL, &timer, NULL);
}

// tworzenie timera t3
void t3_timer(){
	struct sigevent event;
	SIGEV_PULSE_INIT(&event, connection_id, SIGEV_PULSE_PRIO_INHERIT, PULSE_CODE_2, NULL);
	timer_create(CLOCK_REALTIME, &event, &t3);
	struct itimerspec timer = {5,0,1,0};
	timer_settime(t3, NULL, &timer, NULL);
}

// tworzenie timera t4
void t4_timer(){
	struct sigevent event;
	SIGEV_SIGNAL_INIT(&event, SIGINT);
	timer_create(CLOCK_REALTIME, &event, &t4);
	struct itimerspec timer = {10,0,0,0};
	timer_settime(t4, NULL, &timer, NULL);
}

// czyszczenie zasobow i konczeine dzialania programu
void handle_signal(){
	timer_delete(t1);
	timer_delete(t2);
	timer_delete(t3);
	timer_delete(t4);
	ConnectDetach(connection_id);
	ChannelDestroy(channel_id);
	pthread_mutex_destroy(&terminal_mutex);
	exit(0);
}

// odbieranie pulsow i wywolywanie wypisania na ekran
void receive_message(){
	struct _pulse received_pulse;
	for(;;){
		MsgReceive(channel_id, (void*) &received_pulse, sizeof(struct _pulse), NULL);
		if(received_pulse.code == PULSE_CODE_1)
			print("T2");
		else
			print("T3");	
	}
}

int main (){
	signal(SIGINT, handle_signal);
	clock_gettime(CLOCK_REALTIME, &start_time);
	pthread_mutex_init(&terminal_mutex, NULL);
	channel_id = ChannelCreate(NULL);
	connection_id = ConnectAttach(ND_LOCAL_NODE, NULL, channel_id, NULL, NULL);
	t1_timer();
	t2_timer();
	t3_timer();
	t4_timer();			
	receive_message();	
}
