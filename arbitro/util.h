#pragma once


#define UNICODE
#define _UNICODE
#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <stdio.h>
#include <fcntl.h> 
#include <math.h>


#define TAM			80

#define MAXLETRAS	12
#define RITMO		3

#define MAX_PLAYERS	20

#define NDICIONARIO 220

#define NAMEDPIPE	_T("\\\\.\\pipe\\namedpipe")

#define SHM_NAME	_T("memory")
#define SHM_MUTEX	_T("mem_mutex")
#define NL_EVENT	_T("newLetterEvent")

#define REGISTRY	_T("Software\\TrabSO2");


#define RESU	101
#define PONTOS	102
#define LISTA	103
#define SAIR	104

#define ENTROU	105
#define SAIU	106
#define FEZBET  107
#define PASSOU	108

#define NOMEINVALIDO	109
#define EXPULSO			110
#define ENCERRADO		111

#define INICIANDO		112
#define PAUSANDO		113



typedef struct {
	TCHAR letters[MAXLETRAS];
	DWORD in;
} SHM;				// Shared Memory



typedef struct {
	TCHAR name[TAM];
	DWORD points;	// Player
} PLAYER;

typedef struct {
	PLAYER jogadores[MAX_PLAYERS];
}PLAYERLIST;


typedef struct {
	TCHAR playerName[TAM];
	TCHAR word[TAM];

} BET;
typedef struct {
	BOOL valid;
	DWORD points;
} RES;





typedef struct {
	BOOL* isOver;
	DWORD maxletras;
	DWORD* nPlayers, *ritmo;
	SHM* pShm;
	HANDLE hMutex, hEvent;
	PLAYERLIST* lista, *scoreboard;
	HANDLE nPipes[MAX_PLAYERS];	// lista dos namedPipes

} TDADOS;			// arbitro data shared between threads

typedef struct {
	BOOL* isOver;
	SHM* pShm;
	HANDLE hMutex, hEvent;

} JTDADOS;			// jogoUI data shared between threads 

typedef struct {
	BOOL* isOver;
	SHM* pShm;
	HANDLE hMutex;
	DWORD* nPlayers;
	PLAYERLIST* lista, *scoreboard;
	HANDLE NamedPipe;
	HANDLE nPipes[MAX_PLAYERS];

} NPDADOS;			

typedef struct {
	BOOL* isOver;
	HANDLE hMutex;
	HANDLE NamedPipe;

} JNPDADOS;




TCHAR dictionary[NDICIONARIO][TAM] = {
	_T("arroz"), _T("feijao"), _T("carne"), _T("batata"), _T("ovo"),
	_T("peixe"), _T("frango"), _T("massa"), _T("pao"), _T("sopa"),
	_T("copo"), _T("mesa"), _T("porta"), _T("janela"), _T("livro"),
	_T("folha"), _T("papel"), _T("caneta"), _T("lapis"), _T("cadeira"),
	_T("manta"), _T("lenha"), _T("banco"), _T("bolsa"), _T("blusa"),
	_T("roupa"), _T("sapato"), _T("meias"), _T("calca"), _T("casaco"),
	_T("vento"), _T("nuvem"), _T("chuva"), _T("frio"), _T("calor"),
	_T("sol"), _T("lua"), _T("estrela"), _T("terra"), _T("areia"),
	_T("rio"), _T("lago"), _T("mar"), _T("ilha"), _T("pedra"),
	_T("campo"), _T("floresta"), _T("tronco"), _T("galho"), _T("folha"),
	_T("abeto"), _T("planta"), _T("raiz"), _T("verde"), _T("flor"),
	_T("abelha"), _T("urso"), _T("cobra"), _T("gato"), _T("cachorro"),
	_T("pato"), _T("galinha"), _T("cavalo"), _T("porco"), _T("boi"),
	_T("olhos"), _T("ouvido"), _T("boca"), _T("dente"), _T("mao"),
	_T("dedo"), _T("nariz"), _T("pe"), _T("perna"), _T("braco"),
	_T("rosto"), _T("pescoço"), _T("ombro"), _T("joelho"), _T("unha"),
	_T("agua"), _T("leite"), _T("suco"), _T("cafe"), _T("cha"),
	_T("bolo"), _T("doce"), _T("mel"), _T("fruta"), _T("salada"),
	_T("ar"), _T("fogo"), _T("ferro"), _T("madeira"), _T("vidro"),
	_T("papel"), _T("plastico"), _T("couro"), _T("pedra"), _T("cimento"),
	_T("areia"), _T("tinta"), _T("prego"), _T("corda"), _T("linha"),
	_T("amigo"), _T("barco"), _T("campo"), _T("cinza"), _T("dente"),
	_T("dedos"), _T("falar"), _T("gente"), _T("honra"), _T("irmao"),
	_T("justo"), _T("letra"), _T("livre"), _T("mundo"), _T("nuvem"),
	_T("obvio"), _T("pente"), _T("queda"), _T("ruido"), _T("sabor"),
	_T("tempo"), _T("uniao"), _T("velho"), _T("xispa"), _T("zelar"),
	_T("acido"), _T("brisa"), _T("cofre"), _T("dança"), _T("ecoar"),
	_T("festa"), _T("grito"), _T("haste"), _T("idade"), _T("janio"),
	_T("lento"), _T("massa"), _T("noite"), _T("olhar"), _T("pardo"),
	_T("quase"), _T("rente"), _T("sinal"), _T("trigo"), _T("urubu"),
	_T("vapor"), _T("zebra"), _T("agudo"), _T("baixo"), _T("calmo"),
	_T("digno"), _T("enfim"), _T("fardo"), _T("gasto"), _T("haste"),
	_T("ideal"), _T("jeito"), _T("kilos"), _T("limpo"), _T("manso"),
	_T("nobre"), _T("opcao"), _T("pacto"), _T("quieto"), _T("risco"),
	_T("salto"), _T("tecla"), _T("unico"), _T("vazio"), _T("zelos"),
	_T("aluno"), _T("bicho"), _T("criar"), _T("dizer"), _T("exato"),
	_T("firme"), _T("girar"), _T("humor"), _T("insta"), _T("jogar"),
	_T("lazer"), _T("mural"), _T("nasce"), _T("orden"), _T("prato"),
	_T("quero"), _T("rolar"), _T("surge"), _T("turma"), _T("usar"),
	_T("valor"), _T("wager"), _T("xique"), _T("zelar"), _T("andar"),
	_T("bloco"), _T("cinto"), _T("dados"), _T("esqui"), _T("faixa"),
	_T("ganho"), _T("hotel"), _T("idade"), _T("jarra"), _T("leito"),
	_T("a"), _T("b"), _T("c"), _T("d"), _T("e"), _T("f"), _T("g"), _T("h"), _T("i"), _T("j"),
};

