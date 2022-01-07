#define CATCH_CONFIG_MAIN
#define CPP_MIDI_H_IMPL

#include "catch.hpp" 
#include "../cppmidi.h"


TEST_CASE("Header read correctly", "[header]"){
	midi::MIDI rondo("rondo.mid");

	REQUIRE(rondo.getHeader().type == midi::MULTI);
	REQUIRE(rondo.getHeader().numTracks == 2);
		
}
