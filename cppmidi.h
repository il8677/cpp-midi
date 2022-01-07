#ifndef CPP_MIDI_H
#define CPP_MIDI_H

#include <cstdint>

namespace midi{
	typedef enum MidiType : int16_t{
		SINGLE = 0,
		MULTI,
		ASYNC_MULTI
	}

	struct Header{
		MidiType type;
		int16_t numTracks;
		int16_t ticksPerQuater;	
	}

	class MIDI{
		private:
		bool checkMagicString(std::ifstream& inputFile);
		void readHeaderChunk(std::ifstream& inputFile);

		public:
		MIDI(const char* filename);		

		private:
		Header header;
	}


}

#ifdef CPP_MIDI_H_IMPL
#include <fstream>

#define MIDIMAGICSTRING "MThd"

namespace midi{

	bool checkMagicString(std::ifstream& inputFile){
		inputFile.seekg(0, inputFile.beg);

		char magicBuffer[4];
		inputFile.read(magicBuffer, 4);

		return magicBuffer == MIDIMAGICSTRING;
	}

	bool MIDI::readHeaderChunk(std::ifstream& inputFile){
		if(!checkMagicString()) return false;

		// Move 4 bytes past the size field
		inputFile.seekg(4, inputFile.cur);
		
		inputFile.read((char*)&header, sizeof(Header));
	}

	MIDI::MIDI(const char* filename){
		std::ifstream input(filename, std::ios::binary);

		readHeaderChunk(input);
	}
}
#endif

#endif
