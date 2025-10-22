	#include "util.h"	
	#include "time.h"


	TCHAR alphabet[26] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
						  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

	
	void WINAPI generateLetters(void* t) {
		TDADOS* td = (TDADOS*)t;
		DWORD pos;
		TCHAR rc;

		srand((unsigned int)time(NULL));

		WaitForSingleObject(td->hMutex, INFINITE);
		for (int i = 0; i < td->maxletras; i++) {
			td->pShm->letters[i] = '_';
		}
		td->pShm->in = td->maxletras - 1;
		ReleaseMutex(td->hMutex);

		do {
			rc = alphabet[(rand() % 26)];			// Generate letter

			if (*td->nPlayers >= 2) {
				WaitForSingleObject(td->hMutex, INFINITE);

				td->pShm->in = (td->pShm->in + 1) % td->maxletras;

				pos = td->pShm->in;
				td->pShm->letters[pos] = rc;			// Copy letter to Shm

				ReleaseMutex(td->hMutex);
				SetEvent(td->hEvent);

				WaitForSingleObject(td->hEvent, INFINITE);
			}
			ResetEvent(td->hEvent);

			Sleep(*td->ritmo * 1000);					// Delay between letters generation

		} while (!*td->isOver);						// while is not over


		ExitThread(0);
	}






	BOOL isConnected(HANDLE pipe) {			// Verifica se named pipe esta conectado 
		if (pipe == NULL || pipe == INVALID_HANDLE_VALUE)
			return FALSE;

		DWORD flags, outSize, inSize, maxInstances;
		return GetNamedPipeInfo(pipe, &flags, &outSize, &inSize, &maxInstances);
	}

	

	DWORD WINAPI managePlayer(LPVOID a) {
		NPDADOS* t = (NPDADOS*)a;

		DWORD type;

		DWORD sizeRcv, sizeSend, pontos = 0, continua = 1;
		TCHAR dPalavra[TAM];
		BET rcv;
		RES send;

		PLAYER pont, aux;
		PLAYERLIST jogadores;

		BOOL ret;

		SHM copy;

		OVERLAPPED ovWrite, ovRead;
		HANDLE hEvWrite = CreateEvent(NULL, TRUE, FALSE, NULL);
		HANDLE hEvRead = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hEvWrite == NULL || hEvRead == NULL) {
			_tprintf(_T("Erro ao criar evento overlapped! Código: %lu\n"), GetLastError());
			ExitThread(-1);
		}

		ZeroMemory(&ovRead, sizeof(OVERLAPPED));
		ovRead.hEvent = hEvRead;
		ret = ReadFile(t->NamedPipe, &rcv, sizeof(BET), &sizeRcv, &ovRead);
		if (!ret && GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEvRead, INFINITE);
			GetOverlappedResult(t->NamedPipe, &ovRead, &sizeRcv, FALSE);
		}

		// Verifica se nome == null
		if (_tcscmp(rcv.playerName, _T("null")) == 0) {
			// Avisa jogoUI que nome nao e valido
			type = NOMEINVALIDO;
			ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
			ovWrite.hEvent = hEvWrite;
			ret = WriteFile(t->NamedPipe, &type, sizeof(DWORD), &sizeSend, &ovWrite);
			if (!ret) {
				DWORD err = GetLastError();
				if (err == ERROR_IO_PENDING) {
					WaitForSingleObject(hEvWrite, INFINITE);
					GetOverlappedResult(t->NamedPipe, &ovWrite, &sizeSend, FALSE);
				}
			}
			FlushFileBuffers(t->NamedPipe);
			DisconnectNamedPipe(t->NamedPipe);
			CloseHandle(t->NamedPipe);
			ExitThread(-1);
		}
		// Verifica se nome ja existe no jogo
		WaitForSingleObject(t->hMutex, INFINITE);
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (_tcscmp(t->lista->jogadores[i].name, rcv.playerName) == 0) {
				// Avisa jogoUI que nome nao e valido
				type = NOMEINVALIDO;
				ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
				ovWrite.hEvent = hEvWrite;
				ret = WriteFile(t->NamedPipe, &type, sizeof(DWORD), &sizeSend, &ovWrite);
				if (!ret) {
					DWORD err = GetLastError();
					if (err == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvWrite, INFINITE);
						GetOverlappedResult(t->NamedPipe, &ovWrite, &sizeSend, FALSE);
					}
				}
				FlushFileBuffers(t->NamedPipe);
				DisconnectNamedPipe(t->NamedPipe);
				CloseHandle(t->NamedPipe);
				ExitThread(-1);
			}
		}
		ReleaseMutex(t->hMutex);

		_tprintf_s(_T("Conectei com %s\n"), rcv.playerName);
		// Adiciona jogador a lista de jogadores
		WaitForSingleObject(t->hMutex, INFINITE);
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (_tcscmp(t->lista->jogadores[i].name, _T("null")) == 0) {
				_tcscpy_s(t->lista->jogadores[i].name, _countof(t->lista->jogadores[i].name), rcv.playerName);
				t->lista->jogadores[i].points = 0;
				break;
			}
		}
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (_tcscmp(t->scoreboard->jogadores[i].name, _T("null")) == 0) {
				_tcscpy_s(t->scoreboard->jogadores[i].name, _countof(t->scoreboard->jogadores[i].name), rcv.playerName);
				t->scoreboard->jogadores[i].points = 0;
				break;
			}
		}
		
		ReleaseMutex(t->hMutex);

		// Enviar aviso a todos os players que alguem se conectou
		_tcscpy_s(pont.name, _countof(pont.name), rcv.playerName);
		type = ENTROU;
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (t->nPipes[i] != NULL && t->nPipes[i] != t->NamedPipe) {
				ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
				ovWrite.hEvent = hEvWrite;
				ret = WriteFile(t->nPipes[i], &type, sizeof(DWORD), &sizeSend, &ovWrite);
				if (!ret && GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEvWrite, INFINITE);
					GetOverlappedResult(t->nPipes[i], &ovWrite, &sizeSend, FALSE);
				}
				ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
				ovWrite.hEvent = hEvWrite;
				ret = WriteFile(t->nPipes[i], &pont, sizeof(PLAYER), &sizeSend, &ovWrite);
				if (!ret && GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEvWrite, INFINITE);
					GetOverlappedResult(t->nPipes[i], &ovWrite, &sizeSend, FALSE);
				}
			}
		}

		(*t->nPlayers)++;
		if (*t->nPlayers == 2) {
			_tprintf_s(_T("Iniciando jogo\n"));
			type = INICIANDO;
			for (int i = 0; i < MAX_PLAYERS; i++) {
				if (t->nPipes[i] != NULL && t->nPipes[i]) {
					ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
					ovWrite.hEvent = hEvWrite;
					ret = WriteFile(t->nPipes[i], &type, sizeof(DWORD), &sizeSend, &ovWrite);
					if (!ret && GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvRead, INFINITE);
						GetOverlappedResult(t->nPipes[i], &ovRead, &sizeSend, FALSE);
					}
					else if (!ret) break;
				}
			}
		}


		do {
			pontos = 0;
			ZeroMemory(dPalavra, sizeof(dPalavra));

			// Recebe mensagem
			ZeroMemory(&ovRead, sizeof(OVERLAPPED));
			ovRead.hEvent = hEvRead;
			ret = ReadFile(t->NamedPipe, &rcv, sizeof(BET), &sizeRcv, &ovRead);
			if (!ret && GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEvRead, INFINITE);
				GetOverlappedResult(t->NamedPipe, &ovRead, &sizeRcv, FALSE);
			}
			else if (!ret) break;



			// Envia resposta
			ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
			ovWrite.hEvent = hEvWrite;
			if (rcv.word[0] == ':') {
				

				if (_tcscmp(rcv.word, _T(":pont")) == 0) {		// pontuacao do jogador
					WaitForSingleObject(t->hMutex, INFINITE);
					for (int i = 0; i < MAX_PLAYERS; i++)
						if (_tcscmp(t->lista->jogadores[i].name, rcv.playerName) == 0) {
							_tcscpy_s(pont.name, _countof(pont.name), rcv.playerName);
							pont.points = t->lista->jogadores[i].points;
							break;
						}
					ReleaseMutex(t->hMutex);

					type = PONTOS;
					ret = WriteFile(t->NamedPipe, &type, sizeof(DWORD), &sizeSend, &ovWrite);
					if (!ret && GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvRead, INFINITE);
						GetOverlappedResult(t->NamedPipe, &ovRead, &sizeSend, FALSE);
					}
					else if (!ret) break;
					ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
					ovWrite.hEvent = hEvWrite;
					ret = WriteFile(t->NamedPipe, &pont, sizeof(PLAYER), &sizeSend, &ovWrite);
					if (!ret && GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvRead, INFINITE);
						GetOverlappedResult(t->NamedPipe, &ovRead, &sizeSend, FALSE);
					}
					else if (!ret) break;


				}
				else if (_tcscmp(rcv.word, _T(":jogs")) == 0) {		// lista de jogadores
					WaitForSingleObject(t->hMutex, INFINITE);
					for (int i = 0; i < MAX_PLAYERS; i++) {
						jogadores.jogadores[i] = t->scoreboard->jogadores[i];
					}
					ReleaseMutex(t->hMutex);

					type = LISTA;
					ret = WriteFile(t->NamedPipe, &type, sizeof(DWORD), &sizeSend, &ovWrite);
					if (!ret && GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvRead, INFINITE);
						GetOverlappedResult(t->NamedPipe, &ovRead, &sizeSend, FALSE);
					}
					else if (!ret) break;
					ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
					ovWrite.hEvent = hEvWrite;
					ret = WriteFile(t->NamedPipe, &jogadores, sizeof(PLAYERLIST), &sizeSend, &ovWrite);
					if (!ret && GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvRead, INFINITE);
						GetOverlappedResult(t->NamedPipe, &ovRead, &sizeSend, FALSE);
					}
					else if (!ret) break;


				}
				else if (_tcscmp(rcv.word, _T(":sair")) == 0) {		// jogador saiu
					type = SAIR;
					ret = WriteFile(t->NamedPipe, &type, sizeof(DWORD), &sizeSend, &ovWrite);
					if (!ret && GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvRead, INFINITE);
						GetOverlappedResult(t->NamedPipe, &ovRead, &sizeSend, FALSE);
					}
					else if (!ret) break;

					continua = 0;


				}
			}
			else {
				if (*t->nPlayers >= 2) {

					WaitForSingleObject(t->hMutex, INFINITE);
					for (int i = 0; i < MAXLETRAS; i++) {
						copy.letters[i] = t->pShm->letters[i];
					}
					ReleaseMutex(t->hMutex);



					for (int i = 0; i < NDICIONARIO; i++) {
						if (_tcscmp(rcv.word, dictionary[i]) == 0) {	// Palavra esta no dicionario
							_tcscpy_s(dPalavra, _countof(dPalavra), dictionary[i]);

							for (int j = 0; j < _tcslen(dPalavra); j++) {
								for (int k = 0; k < MAXLETRAS; k++) {			// Verificar se letras da palavra estao na SHM;
									if (toupper(dPalavra[j]) == copy.letters[k]) {
										copy.letters[k] = '_';
										pontos++;
										break;
									}
								}
							}

						}
					}



					// Verifica validade da bet
					if (pontos > 0 && pontos == _tcslen(dPalavra)) {				// Se o numero de letras verificadas for igual ao numero de letras da palavra
						send.valid = TRUE;
						send.points = pontos;						// A resposta e valida


						// Tirar letras no SHM
						WaitForSingleObject(t->hMutex, INFINITE);
						for (int i = 0; i < _tcslen(dPalavra); i++) {
							for (int j = 0; j < MAXLETRAS; j++) {
								if (toupper(dPalavra[i]) == t->pShm->letters[j]) {
									t->pShm->letters[j] = '_';
									break;
								}
							}
						}
						ReleaseMutex(t->hMutex);


						// Avisar todos os players a bet do jogador
						type = FEZBET;
						for (int i = 0; i < MAX_PLAYERS; i++) {
							if (t->nPipes[i] != NULL && t->nPipes[i] != t->NamedPipe) {
								ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
								ovWrite.hEvent = hEvWrite;
								ret = WriteFile(t->nPipes[i], &type, sizeof(DWORD), &sizeSend, &ovWrite);
								if (!ret && GetLastError() == ERROR_IO_PENDING) {
									WaitForSingleObject(hEvRead, INFINITE);
									GetOverlappedResult(t->nPipes[i], &ovRead, &sizeSend, FALSE);
								}
								else if (!ret) break;
								ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
								ovWrite.hEvent = hEvWrite;
								ret = WriteFile(t->nPipes[i], &rcv, sizeof(BET), &sizeSend, &ovWrite);
								if (!ret && GetLastError() == ERROR_IO_PENDING) {
									WaitForSingleObject(hEvWrite, INFINITE);
									GetOverlappedResult(t->nPipes[i], &ovWrite, &sizeSend, FALSE);
								}
								else if (!ret) break;


							}
						}
					}
					else {
						send.valid = FALSE;
						send.points = (DWORD)(_tcslen(rcv.word) / 2) * (-1);				// A resposta e invalida
					}
					// Adiciona pontos do jogador a lista de jogadores
					WaitForSingleObject(t->hMutex, INFINITE);
					for (int i = 0; i < MAX_PLAYERS; i++) {
						if (_tcscmp(t->scoreboard->jogadores[i].name, rcv.playerName) == 0) {
							t->scoreboard->jogadores[i].points += send.points;
							t->lista->jogadores[i].points += send.points;
						}
					}
					// Reorganiza scoreboard
					type = PASSOU;
					for (int i = MAX_PLAYERS; i > 0; i--) {
						if (t->scoreboard->jogadores[i].points > t->scoreboard->jogadores[i - 1].points) {
							if (_tcscmp(t->scoreboard->jogadores[i - 1].name, _T("null")) != 0) {
								aux = t->scoreboard->jogadores[i];
								t->scoreboard->jogadores[i] = t->scoreboard->jogadores[i - 1];
								t->scoreboard->jogadores[i - 1] = aux;

								// Avisa que um jogador passou a frente na scoreboard
								for (int i = 0; i < MAX_PLAYERS; i++) {
									if (t->nPipes[i] != NULL) {
										ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
										ovWrite.hEvent = hEvWrite;
										ret = WriteFile(t->nPipes[i], &type, sizeof(DWORD), &sizeSend, &ovWrite);
										if (!ret && GetLastError() == ERROR_IO_PENDING) {
											WaitForSingleObject(hEvRead, INFINITE);
											GetOverlappedResult(t->nPipes[i], &ovRead, &sizeSend, FALSE);
										}
										else if (!ret) break;
										ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
										ovWrite.hEvent = hEvWrite;
										ret = WriteFile(t->nPipes[i], &aux, sizeof(PLAYER), &sizeSend, &ovWrite);
										if (!ret && GetLastError() == ERROR_IO_PENDING) {
											WaitForSingleObject(hEvWrite, INFINITE);
											GetOverlappedResult(t->nPipes[i], &ovWrite, &sizeSend, FALSE);
										}
										else if (!ret) break;

									}
								}


							}
						}
					}
					ReleaseMutex(t->hMutex);




					// Envia resultado da bet para jogoUI (cliente)
					type = RESU;
					ret = WriteFile(t->NamedPipe, &type, sizeof(DWORD), &sizeSend, &ovWrite);
					if (!ret && GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvRead, INFINITE);
						GetOverlappedResult(t->NamedPipe, &ovRead, &sizeSend, FALSE);
					}
					else if (!ret) break;
					ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
					ovWrite.hEvent = hEvWrite;
					ret = WriteFile(t->NamedPipe, &send, sizeof(RES), &sizeSend, &ovWrite);
					if (!ret && GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvRead, INFINITE);
						GetOverlappedResult(t->NamedPipe, &ovRead, &sizeSend, FALSE);
						_tprintf_s(_T("%s enviou: %s - recebeu %d pontos\n"), rcv.playerName, rcv.word, send.points);
					}
					else if (!ret) break;
					else {
						_tprintf_s(_T("%s enviou: %s - recebeu %d pontos\n"), rcv.playerName, rcv.word, send.points);
					}
				}

			}


		} while (!*t->isOver && continua == 1);


		// Enviar aviso a todos os players que se desconectou
		type = SAIU;
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (t->nPipes[i] != NULL && t->nPipes[i] != t->NamedPipe) {
				ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
				ovWrite.hEvent = hEvWrite;
				ret = WriteFile(t->nPipes[i], &type, sizeof(DWORD), &sizeSend, &ovWrite);
				if (!ret && GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEvWrite, INFINITE);
					GetOverlappedResult(t->nPipes[i], &ovWrite, &sizeSend, FALSE);
				}
				else if (!ret) break;
				ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
				ovWrite.hEvent = hEvWrite;
				ret = WriteFile(t->nPipes[i], &pont, sizeof(PLAYER), &sizeSend, &ovWrite);
				if (!ret && GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEvWrite, INFINITE);
					GetOverlappedResult(t->nPipes[i], &ovWrite, &sizeSend, FALSE);
				}
				else if (!ret) break;
			}
		}


		_tprintf_s(_T("%s saiu\n"), rcv.playerName);
		// Apaga nome da lista de jogadores
		WaitForSingleObject(t->hMutex, INFINITE);
		for (int i = 0; i < MAX_PLAYERS; i++) {		
			if (_tcscmp(t->lista->jogadores[i].name, rcv.playerName) == 0) {
				_tcscpy_s(t->lista->jogadores[i].name, _countof(t->lista->jogadores[i].name), _T("null"));
				t->lista->jogadores[i].points = 0;
			}

			if (_tcscmp(t->scoreboard->jogadores[i].name, rcv.playerName) == 0) {
				_tcscpy_s(t->scoreboard->jogadores[i].name, _countof(t->scoreboard->jogadores[i].name), _T("null"));
				t->scoreboard->jogadores[i].points = 0;
			}
		}
		// Avisa para jogadores que jogo foi pausado
		if (*t->nPlayers == 2) {
			_tprintf_s(_T("Pausando jogo\n"));
			type = PAUSANDO;
			for (int i = 0; i < MAX_PLAYERS; i++) {
				if (t->nPipes[i] != NULL && t->nPipes[i] != t->NamedPipe) {
					ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
					ovWrite.hEvent = hEvWrite;
					ret = WriteFile(t->nPipes[i], &type, sizeof(DWORD), &sizeSend, &ovWrite);
					if (!ret && GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvRead, INFINITE);
						GetOverlappedResult(t->nPipes[i], &ovRead, &sizeSend, FALSE);
					}
					else if (!ret) break;
				}
			}
		}
		(*t->nPlayers)--;

		ReleaseMutex(t->hMutex);

		// Finaliza handles e fecha thread
		FlushFileBuffers(t->NamedPipe);
		DisconnectNamedPipe(t->NamedPipe);
		CloseHandle(t->NamedPipe);
		CloseHandle(hEvRead);
		CloseHandle(hEvWrite);
		ExitThread(0);
	}














	void WINAPI createNamedPipes(void* a) {
		TDADOS* t = (TDADOS*)a;

		BOOL iO = FALSE;
		NPDADOS npSt[MAX_PLAYERS];

		HANDLE thread_players[MAX_PLAYERS];
		HANDLE np;
		DWORD type;

		// Inicializa variaveis das threads
		for (int i = 0; i < MAX_PLAYERS; i++) {
			npSt[i].pShm = t->pShm;
			npSt[i].isOver = t->isOver;
			npSt[i].hMutex = t->hMutex;
			npSt[i].nPlayers = t->nPlayers;
			npSt[i].lista = t->lista;
			npSt[i].scoreboard = t->scoreboard;
		}
		
		// Inicia ciclo de criação e conexão dos named pipes
		do {
			if (*t->nPlayers <= MAX_PLAYERS) {
				np = CreateNamedPipe(NAMEDPIPE, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, MAX_PLAYERS, sizeof(RES), sizeof(BET), 1000, NULL);
				if (np == INVALID_HANDLE_VALUE) {
					_tprintf_s(_T("[ERRO] Criar Named Pipe!"));
					exit(-1);
				}

				if (!ConnectNamedPipe(np, NULL)) {
					_tprintf_s(_T("[ERRO] Ligação ao leitor!\n"));
					exit(-1);
				}

				if (!*t->isOver) {
					for (int i = 0; i < MAX_PLAYERS; i++) {
						if (t->nPipes[i] == NULL || !isConnected(t->nPipes[i])) {
							npSt[i].NamedPipe = np;
							t->nPipes[i] = np;
							for (int j = 0; j < MAX_PLAYERS; j++) {
								for (int k = 0; k < MAX_PLAYERS; k++) {
									npSt[j].nPipes[k] = t->nPipes[k];
								}
							}
							thread_players[i] = CreateThread(NULL, 0, managePlayer, (LPVOID)&npSt[i], 0, NULL);
							if (t->nPipes[i] == NULL) {
								_tprintf_s(_T("Erro ao criar thread do jogador\n"));
							}

							break;
						}
					}
				}
				
			}

		} while (!*t->isOver);	// while is not over

		
		

		// Desconecta e finaliza named pipes
		for (int i = 0; i < MAX_PLAYERS; i++) {
			FlushFileBuffers(npSt[i].NamedPipe);
			if (isConnected(npSt[i].NamedPipe)) {
				if (!DisconnectNamedPipe(npSt[i].NamedPipe)) {
					break;
				}
			}
			CloseHandle(npSt[i].NamedPipe);
		}

		ExitThread(0);
	}








	int _tmain(int argc, TCHAR* argv[]) {

		HKEY key = HKEY_CURRENT_USER, hKey = NULL;
		TCHAR nome_chave[TAM] = REGISTRY;

		DWORD res, r, status;
		DWORD val = 0, tam = sizeof(DWORD), ty;
		DWORD max = MAXLETRAS;
		DWORD ritmo = RITMO;


		HANDLE hMap, letters_thread, players_thread;
		SHM* pShm;				// Shm
		HANDLE hEvent;			// Event
		HANDLE hMutex;			// Mutex

		HANDLE hThreads[2];

		TCHAR cons[TAM], user[TAM];

		TDADOS td;
		DWORD nPlayers = 0;	// Envia para createNamedPipes (n_players <= MAX_PLAYERS)
		PLAYERLIST lista, scoreboard;

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		TCHAR cmd[TAM];
		HANDLE destrava;

		BOOL ret;
		DWORD type, sizeSend;
		OVERLAPPED ovWrite;
		HANDLE hEvWrite = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hEvWrite == NULL) {
			_tprintf_s(_T("Erro ao criar evento\n"));
			ExitProcess(-1);
		}

		// Inicializa lista de jogadores e scoreboard
		for (int i = 0; i < MAX_PLAYERS; i++) {
			_tcscpy_s(lista.jogadores[i].name, _countof(lista.jogadores[i].name), _T("null"));
			lista.jogadores[i].points = 0;
			_tcscpy_s(scoreboard.jogadores[i].name, _countof(scoreboard.jogadores[i].name), _T("null"));
			scoreboard.jogadores[i].points = 0;
		}

		// UNICODE
	#ifdef UNICODE
		_setmode(_fileno(stdin), _O_WTEXT);
		_setmode(_fileno(stdout), _O_WTEXT);
		_setmode(_fileno(stderr), _O_WTEXT);
	#endif 

		// Cria SHM
		hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SHM), SHM_NAME);
		if (hMap != NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
			_tprintf_s(_T("Arbitro ja foi executado\n"));
			ExitProcess(-1);
		}
		pShm = (SHM*)MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

		// Cria Mutex de acesso ao SHM
		hMutex = CreateMutex(NULL, FALSE, SHM_MUTEX);
		if (hMutex == NULL) {
			_tprintf_s(_T("Erro ao criar mutex\n"));
			ExitProcess(-1);
		}
		// Cria Evento de geração de letras
		hEvent = CreateEvent(NULL, TRUE, FALSE, NL_EVENT);
		if (hEvent == NULL) {
			_tprintf_s(_T("Erro ao criar evento\n"));
			ExitProcess(-1);
		}

		
		// Faz Query ou inicializa dados do registry (MAXLETRAS e RITMO)
		res = RegCreateKeyEx(key, nome_chave, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &status);
		if (res != 0) {
			_tprintf_s(_T("Erro ao criar chave: %lu\n"), res);
			return;
		}
		if (status == REG_CREATED_NEW_KEY) {
			RegSetValueEx(hKey, _T("MAXLETRAS"), 0, REG_DWORD, (LPBYTE)&max, sizeof(DWORD));
			RegSetValueEx(hKey, _T("RITMO"), 0, REG_DWORD, (LPBYTE)&ritmo, sizeof(DWORD));
			td.maxletras = MAXLETRAS;
			r = RITMO;
			td.ritmo = &r;
		}
		else if (status == REG_OPENED_EXISTING_KEY) {

			tam = sizeof(DWORD);
			res = RegQueryValueEx(hKey, _T("MAXLETRAS"), 0, &ty, (LPBYTE)&val, &tam);
			if (res != 0 || val <= 0 || val > MAXLETRAS) {
				_tprintf_s(_T("Valor de MAXLETRAS inválido. Redefinindo para %d.\n"), MAXLETRAS);
				val = MAXLETRAS;
				RegSetValueEx(hKey, _T("MAXLETRAS"), 0, REG_DWORD, (LPBYTE)&val, sizeof(DWORD));
			}
			td.maxletras = val;

			tam = sizeof(DWORD);
			res = RegQueryValueEx(hKey, _T("RITMO"), 0, &ty, (LPBYTE)&val, &tam);
			if (res != 0 || val <= 0) {
				_tprintf_s(_T("Valor de RITMO inválido. Redefinindo para %d.\n"), RITMO);
				val = RITMO;
				RegSetValueEx(hKey, _T("RITMO"), 0, REG_DWORD, (LPBYTE)&val, sizeof(DWORD));
			}
			r = val;
			td.ritmo = &r;
		}
		//_tprintf_s(_T("maxletras = %d\nritmo = %d\n"), td.maxletras, *td.ritmo);



		// Inicializa estrutura para as threads
		BOOL iO = FALSE;
		td.isOver = &iO;
		td.nPlayers = &nPlayers;
		td.pShm = pShm;
		td.hMutex = hMutex;
		td.hEvent = hEvent;
		td.lista = &lista;		// lista de jogadores
		td.scoreboard = &scoreboard;
		for (int i = 0; i < MAX_PLAYERS; i++) {
			td.nPipes[i] = NULL;
		}
		// Cria threads
		letters_thread = CreateThread(NULL, 0, generateLetters, (LPVOID)&td, 0, NULL);
		players_thread = CreateThread(NULL, 0, createNamedPipes, (LPVOID)&td, 0, NULL);
		hThreads[0] = letters_thread;
		hThreads[1] = players_thread;


		

		// Espera por mensagens do administrador (arbitro)
		do {
			_tscanf_s(_T("%s"), cons, (unsigned)_countof(cons));
			if (_tcscmp(cons, _T("excluir")) == 0 || _tcscmp(cons, _T("iniciarbot")) == 0) {
				_tscanf_s(_T("%s"), user, (unsigned)_countof(user));
			}

			if (_tcscmp(cons,  _T("listar")) == 0) {					// Mostra scoreboard de jogadores e seus pontos
				_tprintf_s(_T("Lista de jogadores:\n"));
				WaitForSingleObject(hMutex, INFINITE);
				for (int i = 0; i < MAX_PLAYERS; i++) {
					if (_tcscmp(scoreboard.jogadores[i].name, _T("null")) != 0)
						_tprintf_s(_T("\t%s - %d\n"), scoreboard.jogadores[i].name, scoreboard.jogadores[i].points);
				}
				ReleaseMutex(hMutex);
			}
			else if (_tcscmp(cons, _T("excluir")) == 0) {				// Expulsa jogador ou bot
				type = EXPULSO;
				for (int i = 0; i < MAX_PLAYERS; i++) {
					if (_tcscmp(user, lista.jogadores[i].name) == 0) {
						ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
						ovWrite.hEvent = hEvWrite;
						ret = WriteFile(td.nPipes[i], &type, sizeof(DWORD), &sizeSend, &ovWrite);
						if (!ret && GetLastError() == ERROR_IO_PENDING) {
							WaitForSingleObject(hEvWrite, INFINITE);
							GetOverlappedResult(td.nPipes[i], &ovWrite, &sizeSend, FALSE);
						}

						break;
					}
				}

				if (nPlayers <= 2) {
					SetEvent(hEvent);
					ResetEvent(hEvent);
				}
				
			}
			else if (_tcscmp(cons, _T("iniciarbot")) == 0) {			// Inicia bot
				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));
				_stprintf_s(cmd, TAM, _T("cmd.exe /c start cmd.exe /k bot.exe BOT%s"), user);
				if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {	// Abre outro cmd
					_tprintf(_T("Erro ao criar bot\n"));
				}
			}
			else if (_tcscmp(cons, _T("acelerar")) == 0) {				// Geração de letras mais rapida
				if (r > 1)
					r--;
			}
			else if (_tcscmp(cons, _T("travar")) == 0) {				// Geração de letras mais devagar
				r++;
			}	
			else if (_tcscmp(cons, _T("encerrar")) == 0) {				// Finaliza processo arbitro
				type = ENCERRADO;
				for (int i = 0; i < MAX_PLAYERS; i++) {
					ZeroMemory(&ovWrite, sizeof(OVERLAPPED));
					ovWrite.hEvent = hEvWrite;
					ret = WriteFile(td.nPipes[i], &type, sizeof(DWORD), &sizeSend, &ovWrite);
					if (!ret && GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEvWrite, INFINITE);
						GetOverlappedResult(td.nPipes[i], &ovWrite, &sizeSend, FALSE);
					}
				}

				iO = TRUE;
				_tprintf_s(_T("FINALIZANDO\n"));
				if (nPlayers <= 2) {
					SetEvent(hEvent);
					ResetEvent(hEvent);
				}
				destrava = CreateFile(NAMEDPIPE, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
				if (destrava != INVALID_HANDLE_VALUE) {
					CloseHandle(destrava);
				}
			}

		} while (!iO);

		// Espera fim das threads
		WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);


		// Finaliza handles e sai do processo
		CloseHandle(letters_thread);
		CloseHandle(players_thread);
		CloseHandle(hEvWrite);
		CloseHandle(hEvent);
		CloseHandle(hMutex);
		CloseHandle(hMap);
		ExitProcess(0);
	}
