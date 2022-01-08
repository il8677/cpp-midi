#define CATCH_CONFIG_MAIN
#define CPP_MIDI_H_IMPL

#include "catch.hpp" 
#include "../cppmidi.h"
#include <cstddef>


TEST_CASE("Header struct makes sense", "[header]"){
	CHECK(offsetof(midi::Header, type)==0);
	CHECK(offsetof(midi::Header, numTracks)==2);
	CHECK(offsetof(midi::Header, ticksPerQuater)==4);
}

TEST_CASE("Bad magic fails to load", "[header][loading]"){
	midi::MIDI m;
	REQUIRE_FALSE(m.loadFile("f.0.1.1284"));
}

TEST_CASE("Non existant file fails to load", "[loading]"){
	midi::MIDI m;
	REQUIRE_FALSE(m.loadFile(""));
	REQUIRE_FALSE(m.loadFile("TTTTTTTTTTTTTT"));
}

TEST_CASE("Correct header loads", "[header][loading]"){
	midi::MIDI m;
	REQUIRE(m.loadFile("c.1.1.1284"));
	REQUIRE(m.loadFile("c.0.1.9"));
	REQUIRE(m.loadFile("c.1.4.1284"));
}

TEST_CASE("Type loads", "[header][loading]"){
	midi::MIDI m;
	m.loadFile("c.1.1.1284");
	REQUIRE(m.getHeader().type == 1);
	m.loadFile("c.0.1.9");
	REQUIRE(m.getHeader().type == 0);
}

TEST_CASE("Num Tracks Load", "[header][loading]"){
	midi::MIDI m;
	m.loadFile("c.1.1.1284");
	REQUIRE(m.getHeader().numTracks == 1);

	m.loadFile("c.1.4.1284");
	REQUIRE(m.getHeader().numTracks == 4);
}

TEST_CASE("Num Ticks Load", "[header][loading]"){
	midi::MIDI m;
	m.loadFile("c.1.1.1284");
	REQUIRE(m.getHeader().ticksPerQuater == 257);
	m.loadFile("c.0.1.9");
	REQUIRE(m.getHeader().ticksPerQuater == 9);
}
