#include "../arbitro/util.h"

void WINAPI receiveLetters(void* t) {
	TDADOS* td = (TDADOS*)t;
	DWORD pos;
	TCHAR cr[MAXLETRAS];

	WaitForSingleObject(td->hMutex, INFINITE);

	for (int i = 0; i < MAXLETRAS; i++) {
		cr[i] = td->pShm->letters[i];
	}

	ReleaseMutex(td->hMutex);

	do {
		WaitForSingleObject(td->hEvent, INFINITE);
		WaitForSingleObject(td->hMutex, INFINITE);

		//pos = td->pShm->in;
		for (int i = 0; i < MAXLETRAS; i++) {
			cr[i] = td->pShm->letters[i];
		}

		ReleaseMutex(td->hMutex);

		if (!*td->isOver) {
			_tprintf_s(_T("\n%c"), cr[0]);
			for (int i = 1; i < MAXLETRAS; i++) {
				_tprintf_s(_T("   %c"), cr[i]);
			}
			_tprintf_s(_T("\n"));
		}

	} while (!*td->isOver);


	ExitThread(0);
}

void WINAPI receiveMessages(void* t) {
	NPDADOS* td = (NPDADOS*)t;

	DWORD type;
	RES res;
	BET bet;
	DWORD ret, sizeRes;

	PLAYER pont;
	PLAYERLIST jogadores;

	OVERLAPPED ovRead;
	HANDLE hEvRead = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvRead == NULL) {
		_tprintf(_T("Erro ao criar evento overlapped! Código: %lu\n"), GetLastError());
		ExitProcess(-1);
	}


	do {


		ZeroMemory(&ovRead, sizeof(OVERLAPPED));
		ovRead.hEvent = hEvRead;
		ret = ReadFile(td->NamedPipe, &type, sizeof(DWORD), &sizeRes, &ovRead);
		if (!ret && GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEvRead, INFINITE);
			GetOverlappedResult(td->NamedPipe, &ovRead, &sizeRes, FALSE);
		}
		else if (!ret) break;


		ZeroMemory(&ovRead, sizeof(OVERLAPPED));
		ovRead.hEvent = hEvRead;
		if (type == RESU) {

			ret = ReadFile(td->NamedPipe, &res, sizeof(RES), &sizeRes, &ovRead);
			if (!ret && GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEvRead, INFINITE);
				GetOverlappedResult(td->NamedPipe, &ovRead, &sizeRes, FALSE);
			}
			else if (!ret) break;

			_tprintf_s(_T("Recebi: %d pontos. Palavra %s\n"), res.points,
				res.valid ? _T("válida") : _T("inválida"));

		}
		else if (type == LISTA) {

			ret = ReadFile(td->NamedPipe, &jogadores, sizeof(PLAYERLIST), &sizeRes, &ovRead);
			if (!ret && GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEvRead, INFINITE);
				GetOverlappedResult(td->NamedPipe, &ovRead, &sizeRes, FALSE);
			}
			else if (!ret) break;

			_tprintf_s(_T("Jogadores:\n"));
			for (int i = 0; i < MAX_PLAYERS; i++) {
				if (_tcscmp(jogadores.jogadores[i].name, _T("null")) != 0)
					_tprintf_s(_T("\t%s - %d\n"), jogadores.jogadores[i].name, jogadores.jogadores[i].points);
			}

		}
		else if (type == SAIR) {
			*td->isOver = TRUE;
			break;

		}
		else if (type == ENTROU) {
			ret = ReadFile(td->NamedPipe, &pont, sizeof(PLAYER), &sizeRes, &ovRead);
			if (!ret && GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEvRead, INFINITE);
				GetOverlappedResult(td->NamedPipe, &ovRead, &sizeRes, FALSE);
			}
			else if (!ret) break;

			_tprintf_s(_T("%s entrou no jogo\n"), pont.name);

		}
		else if (type == SAIU) {
			ret = ReadFile(td->NamedPipe, &pont, sizeof(PLAYER), &sizeRes, &ovRead);
			if (!ret && GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEvRead, INFINITE);
				GetOverlappedResult(td->NamedPipe, &ovRead, &sizeRes, FALSE);
			}
			else if (!ret) break;

			_tprintf_s(_T("%s saiu do jogo\n"), pont.name);

		}
		else if (type == FEZBET) {
			ret = ReadFile(td->NamedPipe, &bet, sizeof(BET), &sizeRes, &ovRead);
			if (!ret && GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEvRead, INFINITE);
				GetOverlappedResult(td->NamedPipe, &ovRead, &sizeRes, FALSE);
			}
			else if (!ret) break;

			_tprintf_s(_T("%s adivinhou a palavra '%s'\n"), bet.playerName, bet.word);

		}
		else if (type == PASSOU) {
			ret = ReadFile(td->NamedPipe, &pont, sizeof(PLAYER), &sizeRes, &ovRead);
			if (!ret && GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEvRead, INFINITE);
				GetOverlappedResult(td->NamedPipe, &ovRead, &sizeRes, FALSE);
			}
			else if (!ret) break;

			_tprintf_s(_T("%s subiu uma posição na scoreboard com %d pontos\n"), pont.name, pont.points);

		}
		else if (type == NOMEINVALIDO) {
			_tprintf_s(_T("O nome é invalido\nEnter para finalizar\n"));
			*td->isOver = TRUE;
			break;
		}
		else if (type == EXPULSO) {
			_tprintf_s(_T("Arbitro me expulsou do jogo\nEnter para finalizar\n"));
			*td->isOver = TRUE;
			break;
		}
		else if (type == ENCERRADO) {
			_tprintf_s(_T("Arbitro foi encerrado\nEnter para finalizar\n"));
			*td->isOver = TRUE;
			break;
		}
		else if (type == INICIANDO) {
			_tprintf_s(_T("JOGO INICIADO\n"));
		}
		else if (type == PAUSANDO) {
			_tprintf_s(_T("JOGO PAUSADO\n"));
		}

	} while (!*td->isOver);


	FlushFileBuffers(td->NamedPipe);
	DisconnectNamedPipe(td->NamedPipe);
	CloseHandle(td->NamedPipe);
	ExitThread(0);
}


void _tmain(DWORD argc, TCHAR* argv[]) {

	HANDLE hMap, letters_thread, message_thread, threads[2];
	SHM* pShm;
	HANDLE hEvent;			// Event
	HANDLE hMutex;			// Mutex

	// Enviar bets
	BET bet;
	DWORD sizeBet;

	HANDLE namedPipe;

	TDADOS td;
	NPDADOS np;

	BOOL iO = FALSE;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif 

	_tcscpy_s(bet.playerName, _countof(bet.word), argv[1]);
	_tcscpy_s(bet.word, _countof(bet.word), _T("bot"));

	hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
	if (hMap == NULL) {
		_tprintf_s(_T("Arbitro nao foi executado\n"));
		ExitProcess(-1);
	}
	pShm = (SHM*)MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, NL_EVENT);
	hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, SHM_MUTEX);
	if (hMutex == NULL) {
		_tprintf_s(_T("Erro ao abrir mutex! Código: %lu\n"), GetLastError());
		ExitProcess(-1);
	}

	// Connecta com Named Pipe
	if (!WaitNamedPipe(NAMEDPIPE, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(_T("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), NAMEDPIPE);
		exit(-1);
	}
	namedPipe = CreateFile(NAMEDPIPE, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);	//Abre named pipe
	if (namedPipe == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Nao foi possivel abrir Named Pipe. Erro: %lu\n"), GetLastError());
		ExitProcess(-1);
	}
	DWORD modo = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(namedPipe, &modo, NULL, NULL);



	td.isOver = &iO;
	td.pShm = pShm;
	td.hMutex = hMutex;
	td.hEvent = hEvent;
	letters_thread = CreateThread(NULL, 0, receiveLetters, (LPVOID)&td, 0, NULL); // Cria thread das letras


	np.isOver = &iO;
	np.hMutex = hMutex;
	np.NamedPipe = namedPipe;
	message_thread = CreateThread(NULL, 0, receiveMessages, (LPVOID)&np, 0, NULL);


	BOOL ret;
	OVERLAPPED ovWrite;
	HANDLE hEvWrite = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvWrite == NULL) {
		_tprintf(_T("Erro ao criar evento overlapped! Código: %lu\n"), GetLastError());
		ExitProcess(-1);
	}

	// Envia nome para o arbitro
	ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
	ovWrite.hEvent = hEvWrite;
	ret = WriteFile(namedPipe, &bet, sizeof(BET), &sizeBet, &ovWrite);
	if (!ret && GetLastError() == ERROR_IO_PENDING) {
		WaitForSingleObject(hEvWrite, INFINITE);
		GetOverlappedResult(namedPipe, &ovWrite, &sizeBet, FALSE);
	}
	if (!ret) {
		_tprintf(_T("Erro ao enviar nome! Código: %lu\n"), GetLastError());
	}

	srand((unsigned int)time(NULL));

	TCHAR input[TAM];
	do {

		Sleep(((rand() % 25) + 5) * 1000);		// intervalo de 5 a 30 segundos

		_tcscpy_s(bet.word, _countof(bet.word), dictionary[rand() % NDICIONARIO]);	// tenta uma das palavras do dicionarioZ

		ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
		ovWrite.hEvent = hEvWrite;
		ret = WriteFile(namedPipe, &bet, sizeof(BET), &sizeBet, &ovWrite);
		if (!ret && GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEvWrite, INFINITE);
			GetOverlappedResult(namedPipe, &ovWrite, &sizeBet, FALSE);
		}
		else if (!ret && !iO) {
			_tprintf(_T("Erro ao enviar mensagem! Código: %lu\n"), GetLastError());
		}



	} while (!iO);

	CloseHandle(namedPipe);




	threads[0] = letters_thread;
	threads[1] = message_thread;
	WaitForMultipleObjects(2, threads, TRUE, INFINITE);

	CloseHandle(letters_thread);
	CloseHandle(message_thread);
	CloseHandle(hEvent);
	CloseHandle(hMutex);
	CloseHandle(hMap);


	ExitProcess(0);
}