#include <thorvg.h>
#include "config.h"
#include "../catch.hpp"
#include "tvgMath.h"

using namespace tvg;

TEST_CASE("shouldMergePoints - basic functionality", "[internal][math]")
{
    Point p1 = {0.0f, 0.0f};
    Point p2 = {0.1f, 0.1f};
    Point p3 = {1.0f, 1.0f};
    
    REQUIRE(shouldMergePoints(p1, p1) == true);
    REQUIRE(shouldMergePoints(p1, p2) == true);
    REQUIRE(shouldMergePoints(p1, p3) == false);
}

TEST_CASE("shouldMergePoints - tolerance", "[internal][math]")
{
    Point p1 = {0.0f, 0.0f};
    Point p2 = {0.3f, 0.0f};
    
    REQUIRE(shouldMergePoints(p1, p2, 0.25f) == false);
    REQUIRE(shouldMergePoints(p1, p2, 0.5f) == true);
}

TEST_CASE("shouldMergePoints - diagonal distance", "[internal][math]")
{
    Point p1 = {0.0f, 0.0f};
    Point p2 = {0.15f, 0.15f};

    REQUIRE(shouldMergePoints(p1, p2, 0.25f) == true);
}

TEST_CASE("orientation - internal function", "[internal][math]")
{
    Point p1 = {0.0f, 0.0f};
    Point p2 = {1.0f, 0.0f};
    Point p3 = {1.0f, 1.0f};

    REQUIRE(orientation(p1, p2, p3) == Orientation::Clockwise);

    Point p4 = {1.0f, -1.0f};
    REQUIRE(orientation(p1, p2, p4) == Orientation::CounterClockwise);

    Point p5 = {2.0f, 0.0f};
    REQUIRE(orientation(p1, p2, p5) == Orientation::Linear);
}

