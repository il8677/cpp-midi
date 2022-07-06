#ifndef CPP_MIDI_H
#define CPP_MIDI_H

#include <cstdint>
#include <fstream>
#include <vector>
#include <ostream>
#include <iomanip>


namespace midi{
	typedef u_int32_t event_delta_t;
	typedef unsigned char channel_t;

	typedef enum TrackFormat : int16_t{
		SINGLE = 0,
		MULTI,
		ASYNC_MULTI
	};

	typedef enum TrackEventType : unsigned char{
		NOTE_OFF=0x80,
		NOTE_ON=0x90,
		POLY=0xA0,
		CONTROLLER=0xB0,
		PROGRAM=0xC0,
		CHANNEL_PRESSURE=0xD0,
		PITCH_BEND_CHANGE=0xE0,
		SYS_EX=0xF0,
		MTC_QTR_FRAME=0xF1,
		SONG_POS_POINTER=0xF2,
		SONG_SELECT=0xF3,
		TUNE_REQUEST=0xF6,
		EO_SYS_EX=0xF7,
		TIMING_CLOCK=0xF8,
		START=0xFA,
		CONTINUE=0xFB,
		STOP=0xFC,
		ACTIVE_SENSING=0xFE,
		META=0xFF
	};

	struct Header{
		TrackFormat type;
		uint16_t numTracks;
		uint16_t ticksPerQuater;

		void swapEndian(); // MIDI is stored as big endian, on little endian systems this needs to be converted
	};

	struct TrackEvent{
		event_delta_t tickDelta;
		TrackEventType type;

		channel_t channel;

		union{
			struct{
				char note;
				char velocity;
			} note;

			struct{
				char note;
				char value;
			} poly;

			struct{
				char function;
				char value;
			} controller;

			struct{
				char program;
			} program;
			
			struct{
				char value;
			} channelPressure;

			struct{
				unsigned char songID;
			} songSelect;

			struct{
				char LSB;
				char MSB;
			} pitchBend, songPosPointer;
		};

		// Reads event from input file
		//	Returns bytes read
		uint32_t readEvent(std::ifstream& inputfile);

		private:

		// Reads status byte from input file
		//	Returns bytes read
		uint32_t readStatusByte(std::ifstream& inputfile);

		// Reads event args from input file
		//	Returns bytes read
		uint32_t readArgs(std::ifstream& inputfile);
	};

	class Track{
		public:
		bool readTrackChunk(std::ifstream& inputfile);

		const std::vector<TrackEvent>& getEvents() const;

		private:
		std::vector<TrackEvent> events;
	};

	class MIDI{
		private:

		bool readHeaderChunk(std::ifstream& inputFile);
		bool readTrackChunk(std::ifstream& inputFile);

		public:

		MIDI();
		bool loadFile(const char* filename);

		const Header& getHeader() const;
		const std::vector<Track>& getTracks() const;

		private:
		Header header;

		std::vector<Track> tracks;
	};

}
#define CPP_MIDI_H_IMPL

#ifdef CPP_MIDI_H_IMPL

#define MIDIHEADERMAGIC (char[]){'M', 'T', 'h', 'd'}
#define MIDITRACKMAGIC (char[]){'M', 'T', 'r', 'k'}

#define ERRORLOG(x) std::cerr << x
#define INFOLOG(x) std::cout << x

#include <iostream>
#include <arpa/inet.h> // for endian detection

namespace midi{
	namespace{
		constexpr bool isBigEndian = htonl(47) == 47;

		uint16_t swapEndian(uint16_t n){
			return (n >> 8) | (n << 8);
		}

		uint32_t swapEndian(uint32_t n){
			return ((n>>24)&0xff) | // move byte 3 to byte 0
						((n<<8)&0xff0000) | // move byte 1 to byte 2
						((n>>8)&0xff00) | // move byte 2 to byte 1
						((n<<24)&0xff000000); // byte 0 to byte 3
		}

		uint32_t readVariableLength(std::ifstream& inputFile, uint32_t* outBytesRead = nullptr){
			char byte;
			
			uint32_t i = 0;
			if(outBytesRead == nullptr){
				outBytesRead = &i;
			}

			uint32_t length = 0;
			*outBytesRead = 0;

			do{
				length = length << 7;
				inputFile.read(&byte, 1);
				length |= (byte & 0b01111111);

				(*outBytesRead)++;
			} while(byte & (1<<7));

			return length;
		}

		bool checkTrackMagic(std::ifstream& inputFile){
			char magicBuffer[4];
			
			inputFile.read(magicBuffer, 4);

			return !std::memcmp(magicBuffer, MIDITRACKMAGIC, 4);
		}

		bool checkHeaderMagic(std::ifstream& inputFile){
			inputFile.seekg(0, inputFile.beg);

			char magicBuffer[4];
			inputFile.read(magicBuffer, 4);

			return !std::memcmp(magicBuffer, MIDIHEADERMAGIC, 4);
		}

	}

	// Header
	void Header::swapEndian(){
		if(!isBigEndian){
			type = (TrackFormat)((type >> 8) | (type << 8));
			numTracks = (numTracks >> 8) | (numTracks << 8);
			ticksPerQuater = (ticksPerQuater >> 8) | (ticksPerQuater << 8);	
		}
	}

	// Event
	uint32_t TrackEvent::readStatusByte(std::ifstream& inputFile){
		unsigned char byte;
		inputFile.read((char*)&byte, 1);

		if(byte < 0xF0){ // Channel command
			type = (TrackEventType)(byte & 0xF0);
			channel = byte & 0x0F;
		}else{
			type = (TrackEventType)(byte);
		}

		return 1;
	}

	uint32_t TrackEvent::readArgs(std::ifstream& inputfile){
		switch (type)
		{
		// 2 byte commands
		case NOTE_ON:
		case NOTE_OFF:
		case POLY:
		case CONTROLLER:
		case PITCH_BEND_CHANGE:
		case SONG_POS_POINTER:
			inputfile.read((char*)&note, 2);

			return 2;
		// 1 byte commands
		case PROGRAM:
		case CHANNEL_PRESSURE:
		case SONG_SELECT:
			inputfile.read(&program.program, 1);
			return 1;
		case META:{
			//TODO: Implement meta events properly
			inputfile.seekg(1, std::ios::cur); // Skip type byte
			uint32_t variableLength;
			uint32_t metaLength = readVariableLength(inputfile, &variableLength);
			inputfile.seekg(metaLength, std::ios::cur); // Skip data

			return metaLength + 1 + variableLength;
		}
		default:
		//TODO: Raise error
			return 0;
		}
	}

	uint32_t TrackEvent::readEvent(std::ifstream& inputFile){
		uint32_t bytesRead = 0;

		tickDelta = readVariableLength(inputFile, &bytesRead);

		bytesRead += readStatusByte(inputFile);
		bytesRead += readArgs(inputFile);

		return bytesRead;
	}


	// Track
	const std::vector<TrackEvent>& Track::getEvents() const{
		return events;
	}

	bool Track::readTrackChunk(std::ifstream& inputFile){
		if(!checkTrackMagic(inputFile)){
			std::cerr << "Error: no magic string at beginning of track\n";
			return false;
		}

		// Read len
		uint32_t len;
		inputFile.read((char*)&len, 4);
		if(!isBigEndian) len = swapEndian(len);

		for(uint32_t i = 0; i < len;){
			TrackEvent event;
			i += event.readEvent(inputFile);
			events.push_back(event);
		}

		return true;
	}


	// MIDI
	bool MIDI::readHeaderChunk(std::ifstream& inputFile){
		if(!checkHeaderMagic(inputFile)){
			std::cerr << "Error: no magic string at beginning of file\n";
			return false;
		} 
		// Move 4 bytes past the size field
		inputFile.seekg(4, inputFile.cur);

		inputFile.read((char*)&header, sizeof(Header));
		header.swapEndian();

		return true;
	}

	bool MIDI::readTrackChunk(std::ifstream& inputFile){
		tracks.push_back(Track());
		return tracks.back().readTrackChunk(inputFile);
	}

	const Header& MIDI::getHeader() const{
		return header;
	}

	const std::vector<Track>& MIDI::getTracks() const{
		return tracks;
	}

	bool MIDI::loadFile(const char* filename){
		std::ifstream input(filename, std::ios::binary);
		if(!input.is_open()){
			std::cerr << "Error: could not open file " << filename << "\n";
			return false;
		}

		if(!readHeaderChunk(input)){
			input.close();
			return false;
		}

		for(int i = 0; i < header.numTracks; i++){
			if(!readTrackChunk(input)){
				input.close();
				return false;
			}
		}

		input.close();
		return true;
	}

	MIDI::MIDI(){

	}
}

// std overrides
namespace std{
	ostream& operator<<(std::ostream& strm, const midi::TrackEvent& event){
		return strm << event.tickDelta << " " << hex << (int)event.type << dec << "\t" << (int)event.note.note << "\t" << (int)event.note.velocity;
	}

	ostream& operator<<(std::ostream& strm, const midi::Track& track){
		for(const midi::TrackEvent& event : track.getEvents()){
			strm << event << "\n";
		}
		return strm;
	}

	ostream& operator<<(std::ostream& strm, const midi::Header& header){
		return strm << "Header" 
			<< "\n\tTrack type: " << header.type 
			<< "\n\tNum Tracks: " << header.numTracks
			<< "\n\tTicks/Quater: " << header.ticksPerQuater;
	}

	ostream& operator<<(std::ostream& strm, const midi::MIDI& midiobj){
		strm << midiobj.getHeader() << "\n";

		for(int i = 0; i < midiobj.getTracks().size(); i++){
			strm << "Track " << i << std::endl;
			strm << midiobj.getTracks()[i];
		}

		return strm;
	}
}
#endif
#endif
