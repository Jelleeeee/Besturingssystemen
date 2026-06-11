#include <Arduino.h>
#include <EEPROM.h>

const byte FILENAME_SIZE = 12;
const byte MAX_FILES = 10;

struct Command {
  const char *name;
  void(*func)();
};

struct FATEntry {
  char name[FILENAME_SIZE];
  int start;
  int length;
};

const byte BUFSIZE = 12;
const int FILE_COUNT_ADDRESS = MAX_FILES * sizeof(FATEntry);  
const int DATA_START = FILE_COUNT_ADDRESS + 1;

EERef noOfFiles = EEPROM[FILE_COUNT_ADDRESS];

const int MEMORY_SIZE = 256;
const byte MAX_VARS = 25;
const byte STACK_SIZE = 32;

const byte TYPE_CHAR = 1;
const byte TYPE_INT = 2;
const byte TYPE_FLOAT = 4;
const byte TYPE_STRING = 5;

byte memory[MEMORY_SIZE];


bool readToken(char *buffer, byte bufferSize);
void processToken(const char *token);
void printPrompt();
void printHelp();

int findFreeSpace(int fileSize);
bool createFileEntry(const char *filename, int fileSize, int *startAddress);
void retrieveFile(const char *filename);
bool eraseFile(const char *filename);
void listFiles();
int getFreeSpace();

void pushByte(byte value);
byte popByte();
void pushInt(int value);
int popInt();
void pushChar(char value);
char popChar();
void pushFloat(float value);
float popFloat();
void pushString(const char *text);
char *popString();
void testTypedStack();
void testVariables();

int findVariable(byte name, int processId);
void removeVariableAt(byte index);
int findFreeMemory(byte size);
bool storeVariable(byte name, int processId);
bool retrieveVariable(byte name, int processId);
void eraseVariablesForProcess(int processId);

// Command stubs
void storeCommand();  // Sla een bestand op in het bestandssysteem
void retrieveCommand(); // Vraag een bestand op uit het bestandssysteem
void eraseCommand();  // Wis een bestand
void filesCommand();  // Print een lijst van bestanden
void freeSpaceCommand();  // print de beschikbare ruimte in het bestandssysteem
void runCommand();  // Start een programma
void listCommand(); // Print een lijst van processen
void suspendCommand();  // Pauzeer een proces
void resumeCommand(); // Hervat een proces
void killCommand(); // Stop een proces

void runProcesses();

Command commands[] = {
  {"STORE", storeCommand},
  {"RETRIEVE", retrieveCommand},
  {"ERASE", eraseCommand},
  {"FILES", filesCommand},
  {"FREESPACE", freeSpaceCommand},
  {"RUN", runCommand},
  {"LIST", listCommand},
  {"SUSPEND", suspendCommand},
  {"RESUME", resumeCommand},
  {"KILL", killCommand}
};

const int commandCount = sizeof(commands) / sizeof(commands[0]);

void setup() {
  Serial.begin(9600);
  noOfFiles = 0;
  printPrompt();
}

void loop() {
  static char tokenBuffer[BUFSIZE];

  runProcesses();

  if(readToken(tokenBuffer, BUFSIZE)) {
    processToken(tokenBuffer);
    printPrompt();
  }
}

void printPrompt() {
  Serial.println("ArduinOS 1.0 Ready");
}

void processToken(const char *token) {
  for(int i = 0; i < commandCount; i++) {
    if(strcmp(token, commands[i].name) == 0) {
      commands[i].func();
      return;
    }
  }

  Serial.print("Unknown command: ");
  Serial.println(token);
  printHelp();
}

void printHelp() {
  Serial.println("Available commands:");
  for(int i = 0; i < commandCount; i++) {
    Serial.println(commands[i].name);
  }
}

bool readToken(char *buffer, byte bufferSize) {
  static byte index = 0;

  while(Serial.available() > 0) {
    char c = Serial.read();

    if(c == '\r') {
      continue;
    }

    Serial.print(c);

    if(c == ' ' || c == '\n') {
      if(index > 0) {
        buffer[index] = '\0';
        index = 0;
        return true;
      }
    } else {
      if(index < bufferSize - 1) {
        buffer[index] = toupper(c);
        index++;
      }
    }
  }

  return false;
}

void runProcesses() {
  // TO DO
}



//--------- Commands ---------
void storeCommand() {
  //Serial.println("STORE called");
  int startAddress = 0;

  if(createFileEntry("TEST", 5, &startAddress)) {
    EEPROM.write(startAddress + 0, 'H');
    EEPROM.write(startAddress + 1, 'E');
    EEPROM.write(startAddress + 2, 'L');
    EEPROM.write(startAddress + 3, 'L');
    EEPROM.write(startAddress + 4, 'O');

    Serial.println("File stored");
  }
}
void retrieveCommand() {
  //Serial.println("RETRIEVE called");
  retrieveFile("TEST");
}
void eraseCommand() {
  //Serial.println("ERASE called");
  eraseFile("TEST");
}
void filesCommand() {
  //Serial.println("FILES called");
  listFiles();
}
void freeSpaceCommand() {
  //Serial.println("FREESPACE called");
  Serial.print("Free space: ");
  Serial.print(getFreeSpace());
  Serial.println(" bytes");
}
void runCommand() {
  Serial.println("RUN called");
}
void listCommand() {
  //Serial.println("LIST called");
  //testTypedStack();
  testVariables();
}
void suspendCommand() {
  Serial.println("SUSPEND called");
}
void resumeCommand() {
  Serial.println("RESUME called");
}
void killCommand() {
  Serial.println("KILL called");
}



// -------- FAT ----------
void writeFATEntry(byte index, FATEntry entry) {
  int address = index * sizeof(FATEntry);
  EEPROM.put(address, entry);
}

FATEntry readFATEntry(byte index) {
  FATEntry entry; 
  int address = index * sizeof(FATEntry);
  EEPROM.get(address, entry);
  return entry;
}

int findFile(const char *filename) {
  for(byte i = 0; i < noOfFiles; i++) {
    FATEntry entry = readFATEntry(i);

    if(strcmp(entry.name, filename) == 0) {
      return i;
    }
  }

  return -1;
}

bool fatIsFull() {
  return noOfFiles >= MAX_FILES;
}

int findFreeSpace(int fileSize) {
  int freeStart = DATA_START;

  for(byte i = 0; i < noOfFiles; i++) {
    FATEntry entry = readFATEntry(i);
    int fileEnd = entry.start + entry.length;

    if(fileEnd > freeStart) {
      freeStart = fileEnd;
    }
  }

  if(freeStart + fileSize <= (int)EEPROM.length()) {
    return freeStart;
  }

  return -1;
}

bool createFileEntry(const char *filename, int fileSize, int *startAddress) {
  if(fatIsFull()) {
    Serial.println("ERROR: FAT is full");
    return false;
  }

  if(findFile(filename) != -1) {
    Serial.println("ERROR: file already exists");
    return false;
  }

  int start = findFreeSpace(fileSize);

  if(start == -1) {
    Serial.println("ERROR: not enough EEPROM space");
    return false;
  }

  FATEntry entry;
  strncpy(entry.name, filename, FILENAME_SIZE);
  entry.name[FILENAME_SIZE -1] = '\0';
  entry.start = start;
  entry.length = fileSize;

  writeFATEntry(noOfFiles, entry);
  noOfFiles++;

  *startAddress = start;
  return true;
}

void retrieveFile(const char *filename) {
  int fileIndex = findFile(filename);

  if(fileIndex == -1) {
    Serial.println("ERROR: file not found");
    return;
  }

  FATEntry entry = readFATEntry(fileIndex);

  for(int i = 0; i < entry.length; i++) {
    char c = EEPROM.read(entry.start + i);
    Serial.print(c);
  }

  Serial.println();
}

bool eraseFile(const char *filename) {
  int fileIndex = findFile(filename);
  
  if(fileIndex == -1) {
    Serial.println("ERROR: file not found");
    return false;
  }

  for(int i = fileIndex; i < noOfFiles - 1; i++) {
    FATEntry nextEntry = readFATEntry(i + 1);
    writeFATEntry(i, nextEntry);
  }

  noOfFiles--;

  Serial.println("File erased");
  return true;
}

void listFiles() {
  if(noOfFiles == 0) {
    Serial.println("No files found");
    return;
  }

  Serial.println("Files:");

  for(byte i = 0; i < noOfFiles; i++) {
    FATEntry entry = readFATEntry(i);
    
    Serial.print(entry.name);
    Serial.print(" - ");
    Serial.print(entry.length);
    Serial.println(" bytes");
  }
}

int getFreeSpace() {
  int biggestFreeSpace = 0;
  int currentPosition = DATA_START;

  bool checked[MAX_FILES] = {false};

  for(byte count = 0; count < noOfFiles; count++) {
    int smallestStart = EEPROM.length();
    int smallestIndex = -1;

    for(byte i = 0; i < noOfFiles; i++) {
      FATEntry entry = readFATEntry(i);

      if(!checked[i] && entry.start < smallestStart) {
        smallestStart = entry.start;
        smallestIndex = i;
      }
    }

    FATEntry entry = readFATEntry(smallestIndex);

    int gap = entry.start - currentPosition;

    if(gap > biggestFreeSpace) {
      biggestFreeSpace = gap;
    }

    currentPosition = entry.start + entry.length;
    checked[smallestIndex] = true;
  }

  int lastGap = (int)EEPROM.length() - currentPosition;

  if(lastGap > biggestFreeSpace) {
    biggestFreeSpace = lastGap;
  }

  return biggestFreeSpace;
}



//---------- MEMORY -----------
struct MemoryEntry {
  byte name;
  byte type; 
  byte address;
  byte size;
  int processId;
};

MemoryEntry memoryTable[MAX_VARS];
byte noOfVars = 0;

byte stack[STACK_SIZE];
byte sp = 0;

void pushByte(byte value) {
  if(sp >= STACK_SIZE) {
    Serial.println("ERROR: Stack overflow");
    return;
  }

  stack[sp] = value;
  sp++;
}

byte popByte() {
  if(sp == 0) {
    Serial.println("ERROR: Stack underflow");
    return 0;
  }

  sp--;
  return stack[sp];
}

void pushInt(int value) {
  pushByte(highByte(value));
  pushByte(lowByte(value));
  pushByte(TYPE_INT);
}

int popInt() {
  byte low = popByte();
  byte high = popByte();
  
  return word(high, low);
}

void pushChar(char value) {
  pushByte((byte)value);
  pushByte(TYPE_CHAR);
}

char popChar() {
  return (char)popByte();
}

void pushFloat(float value) {
  byte *bytes = (byte *)&value;

  for(int i = 3; i >= 0; i--) {
    pushByte(bytes[i]);
  }

  pushByte(TYPE_FLOAT);
}

float popFloat() {
  float value; 
  byte *bytes = (byte *)&value;

  for(int i = 0; i < 4; i++) {
    bytes[i] = popByte();
  }

  return value;
}

void pushString(const char *text) {
  byte length = strlen(text) + 1; // +1 voor '\0'

  for(byte i = 0; i < length; i++) {
    pushByte(text[i]);
  }

  pushByte(length);
  pushByte(TYPE_STRING);
}

char *popString() {
  byte length = popByte();

  sp -= length;

  return (char *)&stack[sp];
}

int findVariable(byte name, int processId) {
  for(byte i = 0; i < noOfVars; i++) {
    if(memoryTable[i].name == name && memoryTable[i].processId == processId) {
      return i;
    }
  }

  return -1;
}

void removeVariableAt(byte index) {
  if(index >= noOfVars) {
    return;
  }

  for(byte i = index; i < noOfVars - 1; i++) {
    memoryTable[i] = memoryTable[i + 1];
  }

  noOfVars--;
}

int findFreeMemory(byte size) {
  int freeStart = 0;

  for(byte i = 0; i < noOfVars; i++) {
    int endAddress = memoryTable[i].address + memoryTable[i].size;

    if(endAddress > freeStart) {
      freeStart = endAddress;
    }
  }

  if(freeStart + size <= MEMORY_SIZE) {
    return freeStart;
  }

  return -1;
}

bool storeVariable(byte name, int processId) {
  if(noOfVars >= MAX_VARS) {
    Serial.println("ERROR: memory table full");
    return false;
  }

  int existingIndex = findVariable(name, processId);

  if(existingIndex != -1) {
    removeVariableAt(existingIndex);
  }

  byte type = popByte();
  byte size = type;

  if(type == TYPE_STRING) {
    size = popByte();
  }

  int address = findFreeMemory(size);

  if(address == -1) {
    Serial.println("ERROR: not enough memory");
    return false;
  }

  MemoryEntry entry;
  entry.name = name;
  entry.type = type;
  entry.address = address;
  entry.size = size;
  entry.processId = processId;

  memoryTable[noOfVars] = entry;

  for(int i = size - 1; i >= 0; i--) {
    memory[address + i] = popByte();
  }

  noOfVars++;
  return true;
}

bool retrieveVariable(byte name, int processId) {
  int index = findVariable(name, processId);

  if(index == -1) {
    Serial.println("ERROR: variable not found");
    return false;
  }

  MemoryEntry entry = memoryTable[index];

  for(byte i = 0; i < entry.size; i++) {
    pushByte(memory[entry.address + i]);
  }

  if(entry.type == TYPE_STRING) {
    pushByte(entry.size);
  }

  pushByte(entry.type);
  return true;
}

void eraseVariablesForProcess(int processId) {
  byte i = 0;

  while(i < noOfVars) {
    if(memoryTable[i].processId == processId) {
      removeVariableAt(i);
    } else {
      i++;
    }
  }
}

// tijdelijke test voor listCommand
void testTypedStack() {
  sp = 0;

  pushChar('A');
  pushInt(300);
  pushFloat(12.34f);
  pushString("HELLO");

  byte type = popByte();

  if(type == TYPE_STRING) {
    char *text = popString();

    Serial.print("STRING: ");
    Serial.println(text);
  }

  type = popByte();

  if(type == TYPE_FLOAT) {
    float value = popFloat();

    Serial.print("FLOAT: ");
    Serial.println(value);
  }

  type = popByte();

  if(type == TYPE_INT) {
    int value = popInt();

    Serial.print("INT: ");
    Serial.println(value);
  }

  type = popByte();

  if(type == TYPE_CHAR) {
    char value = popChar();

    Serial.print("CHAR: ");
    Serial.println(value);
  }
}

void testVariables() {
  sp = 0;
  noOfVars = 0;

  pushInt(100);
  storeVariable('A', 0);

  pushInt(200);
  storeVariable('A', 0);

  retrieveVariable('A', 0);
  byte type = popByte();

  if(type == TYPE_INT) {
    int value = popInt();
    Serial.print("Overwrite A: ");
    Serial.println(value);
  }

  pushString("HELLO");

  if(storeVariable('S', 0)) {
    Serial.println("Stored string");
  }

  if(retrieveVariable('S', 0)) {
    type = popByte();

    if(type == TYPE_STRING) {
      char *text = popString();

      Serial.print("Retrieved string: ");
      Serial.println(text);
    }
  }

  eraseVariablesForProcess(0);

  Serial.print("Variables left: ");
  Serial.println(noOfVars);
}
