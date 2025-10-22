# WordsGame

WordsGame is a project developed for the Operating Systems 2 (2024/25) course at ISEC-DEIS, Computer Engineering. The objective is to implement a multi-process word identification game, where several players compete by forming valid words from a set of letters that appear on the screen, applying advanced operating system concepts such as inter-process communication, shared memory, and synchronization.

## Summary

- [Game Description](#game-description)
- [Implementation Details](#implementation-details)
- [Data Structures](#data-structures)
- [Architecture](#architecture)
- [How to Play](#how-to-play)
- [Build and Run](#build-and-run)
- [Authors](#authors)
- [Implemented Requirements](#implemented-requirements)

## Game Description

In WordsGame, letters are randomly drawn and displayed at fixed intervals. Players (each running in their own console) try to form valid words using the visible letters. Valid words score points and remove the used letters; invalid attempts incur penalties. The game supports up to 20 concurrent players (human or bot) and ends automatically when only one player remains.

The project explores:
- **Named Pipes** for inter-process communication.
- **Shared memory** for managing the visible letters.
- **Process synchronization** for consistent game state.
- Efficient data structures for managing players, scores, and rules.

---

## Implementation Details

### Referee (arbitro)
- **Threads**:
  - `generateLetters`: Responsible for generating random letters and placing them in Shared Memory (SHM).
  - `managePlayer`: Created for each connected `jogoui` via Named Pipe, receives and processes player messages and commands.
  - `createNamedPipes`: Continuously creates and connects Named Pipes to new `jogoui` instances and spawns threads for new players.
- **Main Function** (`_tmain`):
  - Initializes Shared Memory, event, mutex.
  - Queries MAXLETRAS and RITMO from Windows registry.
  - Creates threads and waits for admin commands.
- **Shared Memory**:
  - Contains a character list (the visible letters) and an index indicating the most recently added letter.
- **Named Pipes**:
  - Configured in message mode, using asynchronous overlapped I/O for reading and writing.
- **Synchronization**:
  - Mutex protects SHM and player list access.
  - Event signals whenever a new letter is generated and placed in SHM.

### Player Interface (jogoui)
- **Threads**:
  - `receiveLetters`: Waits for event signaling to print current letters from SHM.
  - `receiveMessages`: Receives and processes messages and responses from the referee, acting based on the received type.
  - Main loop: Opens resources, creates threads, waits for player input and sends words/commands to the referee.

### Bot
- **Threads**:
  - `receiveLetters` and `receiveMessages`: Functionally similar to those in `jogoui`.
- **Main Function**:
  - After initialization, enters a loop where it randomly selects a dictionary word and submits it as a guess to the referee, waiting a random interval (5–30 seconds) between attempts.
  - Bots are started by the referee in a new CMD window.
  - Bots can only be terminated via referee commands (`excluir`) or when the referee ends the game.

---

## Data Structures

- **SHM**: Structure for the shared memory, containing a character list and entry index.
- **PLAYER**: Contains player name and score; exchanged via Named Pipes.
- **PLAYERLIST**: List of PLAYERs, used for the main player list and scoreboard in the referee.
- **BET**: Used for submitting guessed words; contains player name and guessed word.
- **RES**: Response to BET, containing points won/lost and a boolean for word validity.
- **TDADOS/JTDADOS**: Thread argument structures for passing relevant data to threads in referee and jogoui/bot.
- **NPDADOS/JNPDADOS**: Thread argument structures for passing relevant data to player management and message threads.
- **Global dictionary**: List of valid words, defined in `util.h`.

---

## Architecture

The system consists of the following main executables:

- **arbitro**: The central referee process. It manages the game state, draws letters, validates words, manages scores, launches bots, and processes admin commands.
- **jogoui**: Console interface for human players. Accepts player input and communicates with the referee.
- **bot**: Automated player launched by the referee, attempting to form words at random intervals.
- **painel** (optional): A Win32 GUI program that displays the current game state, including visible letters, the last successful word, and the scoreboard.

---

## How to Play

1. **Start the referee** (`arbitro`) in one console.
2. **Open a new console per player** and run `jogoui` with the desired username.
3. **Player commands in jogoui:**
    - Type a word and press ENTER to submit.
    - Use commands:
        - `:pont` — show your score
        - `:jogs` — show the list of players
        - `:sair` — leave the game

4. **Admin commands in the referee console:**
    - `listar` — list players and scores
    - `excluir username` — remove a player
    - `iniciarbot username` — launch a bot
    - `acelerar` — increase the letter cadence
    - `travar` — decrease the letter cadence
    - `encerrar` — end the game

---

## Build and Run

This project is written in C for Windows. You can build it using Microsoft Visual Studio or compatible tools.

**Typical folder structure:**
```
WordsGame/
├── arbitro/
│   ├── arbitro.c
│   └── ficheiro.txt
├── jogoui/
│   └── jogoui.c
├── bot/
│   └── bot.c
├── painel/
│   └── painel.c (optional)
├── util/
│   └── util.h
```

1. Build each program separately.
2. First, run `arbitro`.
3. Launch multiple instances of `jogoui` for each player.

---

## Technical Notes

- Visible letters and synchronization are managed through shared memory, protected by a mutex and signaled by an event.
- Communication between referee, jogoui, and bots is performed via **Named Pipes**, using message type indicators followed by the content.
- All processes must be run by the same OS user.
- The letter dictionary is provided in `arbitro/ficheiro.txt`.
- Bots are started in new CMD windows by the referee, and can only be shut down by referee commands.
- Duplicate player names are not allowed; the referee enforces this rule.

---

## Implemented Requirements

| ID  | Requirement/Feature                                                                              | Status       |
|-----|--------------------------------------------------------------------------------------------------|--------------|
| 1   | Referee: Draw letters and place them in Shared Memory                                            | Implemented  |
| 2   | Referee: Manage players and scores                                                               | Implemented  |
| 3   | Referee: Only one instance allowed                                                               | Implemented  |
| 4   | jogoui: Sends words and commands to referee                                                      | Implemented  |
| 5   | jogoui: Receives responses and notifications from referee                                        | Implemented  |
| 6   | bot: Plays automatically and randomly                                                            | Implemented  |
| 7   | Communication between jogoui/bot and referee via Named Pipes                                     | Implemented  |
| 8   | Letters generated by referee stored in Shared Memory                                             | Implemented  |
| 9   | jogoui retrieves letters via Shared Memory                                                       | Implemented  |
| 10  | Synchronization of letter creation and reception via event                                       | Implemented  |
| 11  | Shared Memory access controlled by mutex                                                         | Implemented  |
| 12  | Game starts with 2+ players, pauses otherwise                                                    | Implemented  |
| 13  | Players score points for correct words                                                           | Implemented  |
| 14  | Letters used in correct words are erased from SHM                                                | Implemented  |
| 15  | MAXLETRAS and RITMO values are read from registry                                                | Implemented  |
| 16  | Registry is corrected if values are invalid                                                      | Implemented  |
| 17  | jogoui commands implemented (:pont, :jogs, :sair)                                                | Implemented  |
| 18  | Admin commands for referee implemented                                                           | Implemented  |
| 19  | bot is started/removed by referee                                                                | Implemented  |
| 20  | Duplicate player names are not allowed                                                           | Implemented  |
| 21  | Referee sends notifications to jogoui/bot for score changes, joins, leaves, and correct words    | Implemented  |
| 22  | Game ends safely                                                                                 | Implemented  |

---

## Authors

**Artur Silvano Capelossi**
ISEC-DEIS, 2024/25

```
