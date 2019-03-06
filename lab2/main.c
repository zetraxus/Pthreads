#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h> // strlen()
#include <sys/neutrino.h> // MsgSend()
#include <sys/dispatch.h> // name_attach()

char* in_channel_name; // nazwa kanalu wejsciowego
char* out_channel_name; // nazwa kanalu wyjsciowego
name_attach_t* in_channel_info; // struktura z informacjami o utworzonym kanale

pthread_t send_t; // struktury z informacjami o watku wysylajacym
pthread_t receive_t; // struktury z informacjami o watku odbierajacym
int out_id; // id polaczenia w kanale wysylajacym

const char* prompt = "? "; // znak zachety
const unsigned BufferSize = 100; // maksymalna wiadomosc = 99 znakow
const unsigned ReplySize = 1; // rozmiar odpowiedzi (char= 1 bajt)

//funkcja wywolywana po odebrania sygnalu ctl+c lub pulse DISCONNECT
void handle_signal(){
	//zabijanie watkow
	pthread_kill(send_t, 9);	
	pthread_kill(receive_t, 9);
	
	// zamykanie kanalow
	name_detach(in_channel_info, NULL);
	name_close(out_id);
}

//funkcja odbierajaca wiadomosci (w osobnym watku)
void* receive(){
	//tworzenie kanalu
	in_channel_info = name_attach(NULL, in_channel_name, NULL);

	char buffer[BufferSize];
	char reply = 1;
	int message_id;

	//oczekiwanie i odbieranie wiadomosci
	for(;;){
		// odebranie wiadomosci
		message_id = MsgReceive(in_channel_info->chid, (void*) buffer, BufferSize, NULL);
					
		if(message_id > 0){ // otrzymano wiadomosc (message_id = id wiadomosci)
			printf("\n => %s\n", buffer); // wypisanie wiadomosci
			printf("%s", prompt); // ponowne wypisanie znaku zachety
			fflush(stdout);
			MsgReply(message_id, NULL, (const void*) &reply, ReplySize); // wyslanie odpowiedzi
		}
		else if (message_id == 0){ // odebrano pulse
			if (((struct _pulse*) buffer)->code == _PULSE_CODE_DISCONNECT) // jesli pulse jest DISCONNECT to konczymy dzialanie
				handle_signal();
		}
	}
}

// funkcja obslugujaca wysylanie wiadomosci
void* send(){
	out_id = -1;
	while(out_id < 0) // proby poleczenia z kanalem
		out_id = name_open(out_channel_name, NULL);
	
	char buffer[BufferSize];
	char reply;

	//oczekiwanie na wiadomosc uzytkownika, wiadomosc konczona jest znakiem ENTER lub po 99 znakach 
	// (limit bufora, wtedy jest dzielona na wiecej wiadomosci)
	for(;;){
		printf("%s", prompt);
		fflush(stdout);
		scanf(" %99[^\n]", buffer);
		MsgSend(out_id, (void*) buffer, strlen(buffer)+1, (void*) &reply, ReplySize); // wyslanie wiadomosci
	}
}

int main(int argv, char** argc){
	if(argv	!= 3){ // oczekujemy na nazwy obu kanalow
		printf("Bledne argumenty wywolania.");
		return 1;
	}
		
	// przypisanie nazw kanalow z argumentow wywolania programu
	in_channel_name = argc[1];
	out_channel_name = argc[2];

	//utworzenie watkow
	pthread_create(&send_t, NULL, &send, NULL);
	pthread_create(&receive_t, NULL, &receive, NULL);
	
	//przechwytywanie ctrl+c
	signal(SIGINT, handle_signal);
	
	//oczekiwanie na zakonczenie watkow
	pthread_join(send_t, NULL);
	pthread_join(receive_t, NULL);

	return 0;
}
