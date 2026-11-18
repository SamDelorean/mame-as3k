// license:BSD-3-Clause
#include "catch.hpp"

#include "../../src/mame/skeleton/neo_lcd.h"

namespace {
void address(neo::lcd &lcd, unsigned controller, unsigned page, unsigned column)
{
    lcd.command(controller, 0xb0 | page);
    lcd.command(controller, 0x10 | (column >> 4));
    lcd.command(controller, column & 0x0f);
}
}

TEST_CASE("Neo LCD buffered sequential reads", "[mame][neo][lcd]")
{
    neo::lcd lcd;
    address(lcd, 0, 2, 7);
    lcd.write(0, 0x12);
    lcd.write(0, 0x34);
    lcd.write(0, 0x56);
    address(lcd, 0, 2, 7);
    lcd.read(0); // Dummy read primes column 7 without advancing.
    REQUIRE(lcd.state(0).column == 7);
    REQUIRE(lcd.read(0) == 0x12);
    REQUIRE(lcd.state(0).column == 8);
    REQUIRE(lcd.read(0) == 0x34);
    REQUIRE(lcd.read(0) == 0x56);

    SECTION("page setup requires another dummy read")
    {
        address(lcd, 0, 3, 10);
        lcd.write(0, 0xa5);
        address(lcd, 0, 2, 10);
        lcd.read(0);
        lcd.command(0, 0xb3);
        lcd.read(0);
        REQUIRE(lcd.state(0).column == 10);
        REQUIRE(lcd.read(0) == 0xa5);
    }
    SECTION("both halves of column setup invalidate the latch")
    {
        lcd.command(0, 7);
        lcd.read(0);
        REQUIRE(lcd.state(0).column == 7);
        REQUIRE(lcd.read(0) == 0x12);
        lcd.command(0, 0x10);
        lcd.read(0);
        REQUIRE(lcd.state(0).column == 8);
        REQUIRE(lcd.read(0) == 0x34);
    }
}

TEST_CASE("Neo LCD writes and read-modify-write reads", "[mame][neo][lcd]")
{
    neo::lcd lcd;
    address(lcd, 0, 0, 4);
    lcd.write(0, 0x11);
    lcd.write(0, 0x22);
    address(lcd, 0, 0, 4);
    lcd.command(0, 0xe0);
    lcd.read(0);
    REQUIRE(lcd.read(0) == 0x11);
    REQUIRE(lcd.read(0) == 0x11);
    REQUIRE(lcd.state(0).column == 4);
    lcd.write(0, 0x33);
    REQUIRE(lcd.state(0).column == 5);
    lcd.read(0); // A write invalidates the read pipeline, including in RMW mode.
    REQUIRE(lcd.read(0) == 0x22);
    REQUIRE(lcd.state(0).column == 5);
    lcd.write(0, 0x44);
    lcd.command(0, 0xee);
    REQUIRE_FALSE(lcd.state(0).rmw);
    REQUIRE(lcd.state(0).column == 4);
    lcd.read(0);
    REQUIRE(lcd.read(0) == 0x33);
    REQUIRE(lcd.read(0) == 0x44);
    lcd.write(0, 0x55);
    lcd.read(0);
    REQUIRE(lcd.state(0).column == 7);
    REQUIRE(lcd.read(0) == 0);
}

TEST_CASE("Neo LCD debugger reads preserve both pipelines", "[mame][neo][lcd]")
{
    neo::lcd lcd;
    for(unsigned n=0; n<2; ++n) {
        address(lcd, n, 0, 0);
        lcd.write(n, n ? 0xa5 : 0x5a);
        lcd.write(n, n ? 0xb6 : 0x6b);
        address(lcd, n, 0, 0);
        REQUIRE(lcd.read(n, false) == lcd.read(n, false));
        REQUIRE(lcd.state(n).read_pending);
        REQUIRE(lcd.state(n).column == 0);
        lcd.read(n);
        REQUIRE(lcd.read(n, false) == (n ? 0xa5 : 0x5a));
        REQUIRE(lcd.read(n, false) == (n ? 0xa5 : 0x5a));
        REQUIRE(lcd.state(n).column == 0);
    }
    REQUIRE(lcd.read(0) == 0x5a);
    REQUIRE(lcd.read(1) == 0xa5);
    REQUIRE(lcd.read(0) == 0x6b);
    REQUIRE(lcd.read(1) == 0xb6);

    SECTION("last column saturates")
    {
        address(lcd, 0, 0, neo::lcd::columns-1);
        lcd.write(0, 0xef);
        lcd.read(0);
        REQUIRE(lcd.read(0) == 0xef);
        REQUIRE(lcd.read(0) == 0xef);
        REQUIRE(lcd.state(0).column == neo::lcd::columns-1);
    }
    SECTION("reset requires the latch to be primed again")
    {
        lcd.reset();
        REQUIRE(lcd.state(0).read_pending);
        REQUIRE(lcd.state(1).read_pending);
        REQUIRE(lcd.read(0, false) == 0);
        REQUIRE(lcd.state(0).column == 0);
    }
}
