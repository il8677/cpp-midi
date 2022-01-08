#ifndef CPP_MIDI_H
#define CPP_MIDI_H

#include <cstdint>
#include <fstream>

namespace midi{
	typedef enum TrackFormat : int16_t{
		SINGLE = 0,
		MULTI,
		ASYNC_MULTI
	};

	struct Header{
		TrackFormat type;
		int16_t numTracks;
		int16_t ticksPerQuater;

		void swapEndian(); // MIDI is stored as big endian, on little endian systems this needs to be converted
	};

	class MIDI{
		private:
		bool checkMagicString(std::ifstream& inputFile);
		bool readHeaderChunk(std::ifstream& inputFile);

		public:
		MIDI();

		bool loadFile(const char* filename);

		const Header& getHeader();

		private:
		Header header;
	};

}
#define CPP_MIDI_H_IMPL

#ifdef CPP_MIDI_H_IMPL

#define MIDIMAGICSTRING (char[]){'M', 'T', 'h', 'd'}

#define ERRORLOG(x) std::cerr << x
#define INFOLOG(x)

#include <iostream>
#include <arpa/inet.h> // for endian detection

namespace midi{
	constexpr bool isBigEndian = htonl(47) == 47;

	// Header
	void Header::swapEndian(){
		if(!isBigEndian){
			type = (TrackFormat)((type >> 8) | (type << 8));
			numTracks = (numTracks >> 8) | (numTracks << 8);
			ticksPerQuater = (ticksPerQuater >> 8) | (ticksPerQuater << 8);	
		}
	}

	// MIDI
	bool MIDI::checkMagicString(std::ifstream& inputFile){
		inputFile.seekg(0, inputFile.beg);

		char magicBuffer[4];
		inputFile.read(magicBuffer, 4);

		return !std::memcmp(magicBuffer, MIDIMAGICSTRING, 4);
	}

	bool MIDI::readHeaderChunk(std::ifstream& inputFile){
		if(!checkMagicString(inputFile)){
			std::cerr << "Error, no magic string at begining of file\n";
			return false;
		} 
		// Move 4 bytes past the size field
		inputFile.seekg(4, inputFile.cur);
		
		inputFile.read((char*)&header, sizeof(Header));
		header.swapEndian();


		return true;
	}

	const Header& MIDI::getHeader(){
		return header;
	}

	bool MIDI::loadFile(const char* filename){
		std::ifstream input(filename, std::ios::binary);
		if(!input.is_open()){
			std::cerr << "Error could not open file " << filename << "\n";
			return false;
		}

		if(!readHeaderChunk(input)){
			input.close();
			return false;
		}

		input.close();
		return true;
	}

	MIDI::MIDI(){

	}
}
#endif

#endif
