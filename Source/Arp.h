/*
  ==============================================================================

    Arp.h
    Created: 12 Jan 2023 11:16:13pm
    Author:  Yves Parès

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ChordStore.h"

using namespace juce;

#define IS_NOTE_MESSAGE(msg) (msg.isNoteOn() || msg.isNoteOff())


class Arp : public ArplignerAudioProcessor {
private:
  ChordStore mLocalChordStore;
  
  // On each pattern chan, to which note is currently mapped each incoming NoteNumber
  HashMap<NoteNumber, NoteNumber> mCurMappings[16];

  void nonGlobalChordInstanceWork(MidiBuffer&, InstanceBehaviour::Enum);

  void toChordStore(ChordStore* chordStore, const MidiMessage& msg) {
    if (msg.isNoteOn())
      chordStore->addChordNote(msg.getNoteNumber());
    else if (msg.isNoteOff())
      chordStore->rmChordNote(msg.getNoteNumber());
  }

  void updateChordStore(ChordStore* chordStore) {
    chordStore->updateCurrentChord
      ((WhenNoChordNote::Enum)whenNoChordNote->getIndex(),
       (WhenSingleChordNote::Enum)whenSingleChordNote->getIndex());
  }

  // We special-case the main loop of the global chord instance since it has
  // much less operations to do, and should lock the chord store
  void globalChordInstanceWork(const MidiBuffer& midibuf) {
    GlobalChordStore* chordStore = GlobalChordStore::getInstance();
    const ScopedWriteLock lock(chordStore->globalStoreLock);

    for (auto msgMD : midibuf) {
      toChordStore(chordStore, msgMD.getMessage());
    }
    updateChordStore(chordStore);
  }
  
public:
  void runArp(MidiBuffer& midibuf) {
    auto behaviour = (InstanceBehaviour::Enum)instanceBehaviour->getIndex();
    switch (behaviour) {
    case InstanceBehaviour::BYPASS:
      break;
    case InstanceBehaviour::IS_CHORD:
      globalChordInstanceWork(midibuf);
      break;
    default:
      nonGlobalChordInstanceWork(midibuf, behaviour);
      break;
    }
  }
};
