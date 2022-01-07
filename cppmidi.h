#ifndef CPP_MIDI_H
#define CPP_MIDI_H

#include <cstdint>
#include <fstream>

namespace midi{
	typedef enum MidiType : int16_t{
		SINGLE = 0,
		MULTI,
		ASYNC_MULTI
	};

	struct Header{
		MidiType type;
		int16_t numTracks;
		int16_t ticksPerQuater;	
	};

	class MIDI{
		private:
		bool checkMagicString(std::ifstream& inputFile);
		bool readHeaderChunk(std::ifstream& inputFile);

		public:
		MIDI(const char* filename);

		const Header& getHeader();

		private:
		Header header;
	};


}
#define CPP_MIDI_H_IMPL

#ifdef CPP_MIDI_H_IMPL

#define MIDIMAGICSTRING "MThd"

namespace midi{

	bool checkMagicString(std::ifstream& inputFile){
		inputFile.seekg(0, inputFile.beg);

		char magicBuffer[4];
		inputFile.read(magicBuffer, 4);

		return magicBuffer == MIDIMAGICSTRING;
	}

	bool MIDI::readHeaderChunk(std::ifstream& inputFile){
		if(!checkMagicString(inputFile)) return false;

		// Move 4 bytes past the size field
		inputFile.seekg(4, inputFile.cur);
		
		inputFile.read((char*)&header, sizeof(Header));
	}

	const Header& MIDI::getHeader(){
		return header;
	}

	MIDI::MIDI(const char* filename){
		std::ifstream input(filename, std::ios::binary);

		readHeaderChunk(input);

		input.close();
	}
}
#endif

#endif
