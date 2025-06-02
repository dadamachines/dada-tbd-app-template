bool core1_separate_stack = true;

#include "Midi.h"
#include "Ui.h"

Midi midi; // MIDI handling
Ui ui; // UI handling

// the setup function runs once when you press reset or power the board
void setup(){
    midi.Init(); // Initialize MIDI handling
}

void setup1(){
    ui.Init(); // Initialize UI handling
}

void loop(){
    midi.Update(); // Update MIDI handling
}

void loop1(){
    ui.SetLedStatus(midi.GetLedStatus());
    ui.Update(); // Update UI handling
}
