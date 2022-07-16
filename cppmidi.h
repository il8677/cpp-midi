#ifndef CPP_MIDI_H
#define CPP_MIDI_H

#include <cstring>
#include <cstdint>
#include <fstream>
#include <vector>
#include <ostream>
#include <iomanip>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <functional>
#include <cmath>


namespace midi{
	typedef u_int32_t event_delta_t;
	typedef unsigned char channel_t;

	enum TrackFormat : int16_t{
		SINGLE = 0,
		MULTI,
		ASYNC_MULTI
	};

	enum TrackEventType : unsigned char {
		// Meta events
		SEQ_NUM=0x00,
		TEXT_EVENT=0x01,
		COPYRIGHT=0x02,
		TRACK_NAME=0x03,
		INSTRUMENT_NAME=0x04,
		LYRIC=0x05,
		TEXT_MARKER=0x06,
		CUE_POINT=0x07,
		CHANNEL_PREFIX=0x20,
		TRACK_END=0x2F,
		SET_TEMPO=0x51,
		TIME_SIGNATURE=0x58,

		// MIDI Events
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

	class MIDI;
	class Track;

	class Header{
		public:
		TrackFormat getType() const;
		uint16_t getNumTracks() const;
		uint16_t getTicksPerBeat() const;
		
		private:
		TrackFormat type;
		uint16_t numTracks;
		uint16_t ticksPerBeat;

		void swapEndian(); // MIDI is stored as big endian, on little endian systems this needs to be converted

		friend class MIDI;
	};

	class Event{
		private:
		union EventData{
			struct EventTempo {
				uint32_t msPerBeat;
			} tempo;

			struct EventNote {
				char note;
				char velocity;

				float getFreq() const;
			} note;

			struct EventPoly {
				char note;
				char value;
			} poly;

			struct EventController {
				char function;
				char value;
			} controller;

			struct EventProgram {
				char program;
			} program;
			
			struct EventChannelPressure {
				char value;
			} channelPressure;

			struct EventSongSelect {
				unsigned char songID;
			} songSelect;

			struct EventPitchBend {
				char LSB;
				char MSB;
			} pitchBend, songPosPointer;
		} eventData;
		public:

		const EventData& getData() const;
		
		event_delta_t getTickDelta() const;
		event_delta_t getTick() const;

		TrackEventType getType() const;
		channel_t getChannel() const;


		private:

		event_delta_t tickDelta;
		event_delta_t tick;
		TrackEventType type;

		channel_t channel;

		// Reads event from input file
		//	Returns bytes read
		uint32_t readEvent(std::ifstream& inputfile, event_delta_t prevTick);

		// Reads status byte from input file
		//	Returns bytes read
		uint32_t readStatusByte(std::ifstream& inputfile);

		// Reads event args from input file
		//	Returns bytes read
		uint32_t readArgs(std::ifstream& inputfile);

		friend class Track;
	};

	class Track{
		public:

		const std::vector<Event>& getEvents() const;
		const Event& getEvent(int index) const;


		private:
		bool readTrackChunk(std::ifstream& inputfile);

		std::vector<Event> events;

		friend class MIDI;
	};

	class MIDI{
		public:
		bool loadFile(const char* filename);

		const Header& getHeader() const;
		const std::vector<Track>& getTracks() const;
		const Track& getTrack(int track) const;
		const Event& getEvent(int track, int index) const;

		private:
		bool readHeaderChunk(std::ifstream& inputFile);
		bool readTrackChunk(std::ifstream& inputFile);

		Header header;

		std::vector<Track> tracks;

		event_delta_t currentTick;
	};

	class MIDIPlayer{
	public:
		MIDIPlayer(const MIDI& midiObject);	

		void registerEventCallback(TrackEventType, std::function<void(Event)>);	
		void play();

		void setTempo(uint32_t msPerBeat);

		bool done();
	private:
		const Event& getNextEvent();

		bool trackIsDone(int trackNum) const;
		const Event& getNextEventOfTrack(int trackNum) const;

		uint32_t msPerBeat = 500000;
		event_delta_t currentTick = 0;
	
		std::unordered_map<TrackEventType, std::vector<std::function<void(Event)>>> eventCallbacks;
		std::vector<size_t> nextEvents;

		const MIDI& midi;
	};
}

#ifdef CPP_MIDI_H_IMPL

#define ERRORLOG(x) std::cerr << x
#define INFOLOG(x) std::cout << x

#include <iostream>
#include <arpa/inet.h> // for endian detection

namespace midi{
	namespace{
		const char* midiHeaderMagic = "MThd";
		const char* midiTrackMagic = "MTrk";

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
			uint32_t i = 0;
			if(outBytesRead == nullptr){
				outBytesRead = &i;
			}

			char byte;
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

			return !std::memcmp(magicBuffer, midiTrackMagic, 4);
		}

		bool checkHeaderMagic(std::ifstream& inputFile){
			inputFile.seekg(0, inputFile.beg);

			char magicBuffer[4];
			inputFile.read(magicBuffer, 4);

			return !std::memcmp(magicBuffer, midiHeaderMagic, 4);
		}

	}

	// Header
	TrackFormat Header::getType() const{
		return type;
	}

	uint16_t Header::getNumTracks() const{
		return numTracks;
	}

	uint16_t Header::getTicksPerBeat() const{
		return ticksPerBeat;
	}

	void Header::swapEndian(){
		if(!isBigEndian){
			type = (TrackFormat)((type >> 8) | (type << 8));
			numTracks = (numTracks >> 8) | (numTracks << 8);
			ticksPerBeat = (ticksPerBeat >> 8) | (ticksPerBeat << 8);	
		}
	}

	// Event
	const Event::EventData& Event::getData() const{
		return eventData;
	}


	event_delta_t Event::getTickDelta() const{
		return tickDelta;
	}

	event_delta_t Event::getTick() const{
		return tick;
	}

	TrackEventType Event::getType() const{
		return type;
	}

	channel_t Event::getChannel() const{
		return channel;
	}


	uint32_t Event::readStatusByte(std::ifstream& inputFile){
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

	uint32_t Event::readArgs(std::ifstream& inputfile){
		switch (type)
		{
		// 2 byte commands
		case NOTE_ON:
		case NOTE_OFF:
		case POLY:
		case CONTROLLER:
		case PITCH_BEND_CHANGE:
		case SONG_POS_POINTER:
			inputfile.read((char*)&eventData.note, 2);

			return 2;
		// 1 byte commands
		case PROGRAM:
		case CHANNEL_PRESSURE:
		case SONG_SELECT:
			inputfile.read((char*)&eventData.program, 1);
			return 1;
		case META:{
			inputfile.read((char*)&type, 1);

			uint32_t variableLength;
			uint32_t metaLength = readVariableLength(inputfile, &variableLength);

			switch(type){
				case SET_TEMPO:
					unsigned char length[3];
					inputfile.read((char*)length, 3);
					
					eventData.tempo.msPerBeat = 0;

					for(int i = 0; i < 3; i++){
						eventData.tempo.msPerBeat |= length[2-i]<<(i*8);
					}
					break;
				default:
					inputfile.seekg(metaLength, std::ios::cur);
					break;
			}

			return metaLength + 1 + variableLength;
		}
		default:
		//TODO: Raise error
			return 0;
		}
	}

	uint32_t Event::readEvent(std::ifstream& inputFile, event_delta_t prevTick){
		uint32_t bytesRead = 0;

		tickDelta = readVariableLength(inputFile, &bytesRead);
		tick = tickDelta + prevTick;

		bytesRead += readStatusByte(inputFile);
		bytesRead += readArgs(inputFile);

		return bytesRead;
	}

	float Event::EventData::EventNote::getFreq() const {
		return pow(2, (note-69)/12.0f) * 440.0f;
	}

	// Track
	const std::vector<Event>& Track::getEvents() const{
		return events;
	}

	const Event& Track::getEvent(int index) const{
		return events.at(index);
	}

	bool Track::readTrackChunk(std::ifstream& inputFile){
		if(!checkTrackMagic(inputFile)){
			std::cerr << "Error: no magic string at beginning of track\n";
			return false;
		}

		// Read len of track
		uint32_t len;
		inputFile.read((char*)&len, 4);
		if(!isBigEndian) len = swapEndian(len);

		event_delta_t prevTick = 0;

		for(uint32_t bytesRead = 0; bytesRead < len;){
			Event event;
			bytesRead += event.readEvent(inputFile, prevTick);
			prevTick = event.getTick();
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

	const Track& MIDI::getTrack(int track) const{
		return tracks.at(track);
	}

	const Event& MIDI::getEvent(int track, int index) const{
		return getTrack(track).getEvent(index);
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

	// MIDIPlayer
	MIDIPlayer::MIDIPlayer(const MIDI& midiObject) : midi(midiObject){
		// TODO: Copy midi object to ensure iterator validness?
		for(const Track& track : midiObject.getTracks()){
			nextEvents.push_back(0);
		}
		
		registerEventCallback(SET_TEMPO, [&](Event e){
			this->setTempo(e.getData().tempo.msPerBeat);
		});
	}

	void MIDIPlayer::setTempo(uint32_t msPerBeat){
		this->msPerBeat = msPerBeat;
	}

	void MIDIPlayer::play(){
		while(!done()){
			const Event& nextEvent = getNextEvent();

			// Sleep until next event
			if(nextEvent.getTick() > currentTick){
				// Calculate time to sleep
				const auto TpB = midi.getHeader().getTicksPerBeat();
				const auto UspB = msPerBeat;
				const auto ticks = nextEvent.getTick() - currentTick;

				const auto B = ticks/TpB;
				const auto Us = UspB * B;

				std::this_thread::sleep_for(std::chrono::microseconds(Us));

			}

			// Call all callbacks for event
			if(eventCallbacks.find(nextEvent.getType()) != eventCallbacks.end()){
				for(auto& func : eventCallbacks.at(nextEvent.getType())){
					func(nextEvent);
				}
			}

			currentTick = nextEvent.getTick();
		}
	}

	bool MIDIPlayer::done(){
		for(int i = 0; i < nextEvents.size(); i++){
			if(!trackIsDone(i)){
				return false;
			}
		}

		return true;
	}

	bool MIDIPlayer::trackIsDone(int trackNum) const{
		return nextEvents[trackNum] >= midi.getTrack(trackNum).getEvents().size() - 1;
	}

	void MIDIPlayer::registerEventCallback(TrackEventType eventType, std::function<void(Event)> func){
		eventCallbacks[eventType].push_back(func);
	}

	// TODO: Review readibility
	// TODO: Fix bug where if track is done the first event is returned
	const Event& MIDIPlayer::getNextEvent() {
		size_t* minEventIndex = &nextEvents[0];
		const Event* minEvent = &getNextEventOfTrack(0);

		for(int i = 0; i < nextEvents.size(); i++){
			if(trackIsDone(i)) continue;
			if(getNextEventOfTrack(i).getTick() < minEvent->getTick()){
				minEventIndex = &nextEvents[i];
				minEvent = &getNextEventOfTrack(i);
			}
		}

		(*minEventIndex)++;

		return *minEvent;
	}

	
	const Event& MIDIPlayer::getNextEventOfTrack(int trackNum) const {
		return midi.getEvent(trackNum, nextEvents[trackNum]);
	}

}

// std overrides
namespace std{
	ostream& operator<<(std::ostream& strm, const midi::Event& event){
		return strm << event.getTickDelta() << " " << hex << (int)event.getType() << dec << "\t" << (int)event.getData().note.note << "\t" << (int)event.getData().note.velocity;
	}

	ostream& operator<<(std::ostream& strm, const midi::Track& track){
		for(const midi::Event& event : track.getEvents()){
			strm << event << "\n";
		}
		return strm;
	}

	ostream& operator<<(std::ostream& strm, const midi::Header& header){
		return strm << "Header" 
			<< "\n\tTrack type: " << header.getType() 
			<< "\n\tNum Tracks: " << header.getNumTracks()
			<< "\n\tTicks/Quater: " << header.getTicksPerBeat();
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
