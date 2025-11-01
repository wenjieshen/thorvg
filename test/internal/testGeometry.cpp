/*
 * Copyright (c) 2021 - 2025 the ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"
#include "../catch.hpp"
#include "tvgRender.h"

using namespace tvg;

TEST_CASE("RenderPath optimize - Empty Path", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    RenderPath out;

    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.empty());
    REQUIRE(out.pts.empty());
}

TEST_CASE("RenderPath optimize - Single MoveTo", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    in.moveTo({10.0f, 20.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count == 1);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
    REQUIRE(out.pts.count == 1);
    REQUIRE(out.pts[0].x == 10.0f);
    REQUIRE(out.pts[0].y == 20.0f);
}

TEST_CASE("RenderPath optimize - MoveTo and LineTo", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.lineTo({10.0f, 10.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count == 2);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
    REQUIRE(out.cmds[1] == PathCommand::LineTo);
    REQUIRE(out.pts.count == 2);
}

TEST_CASE("RenderPath optimize - Merge Close Points", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.lineTo({0.1f, 0.1f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count == 1);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
}

TEST_CASE("RenderPath optimize - Collinear Points", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.lineTo({5.0f, 5.0f});
    in.lineTo({10.0f, 10.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count >= 1);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
}

TEST_CASE("RenderPath optimize - CubicTo Command", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.cubicTo({5.0f, 0.0f}, {10.0f, 5.0f}, {10.0f, 10.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count >= 1);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
}

TEST_CASE("RenderPath optimize - Flat Cubic Curve", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    // (0,0), (3,3), (7,7), (10,10) on the line y = x
    in.moveTo({0.0f, 0.0f});
    in.cubicTo({3.0f, 3.0f}, {7.0f, 7.0f}, {10.0f, 10.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count == 2);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
    REQUIRE(out.cmds[1] == PathCommand::LineTo);
    REQUIRE(out.pts.count == 2);
    REQUIRE(out.pts[0].x == 0.0f);
    REQUIRE(out.pts[0].y == 0.0f);
    REQUIRE(out.pts[1].x == 10.0f);
    REQUIRE(out.pts[1].y == 10.0f);
}

TEST_CASE("RenderPath optimize - Flat Cubic Colinear with Line", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    // Line from (0,0) to (5,5), then flat cubic from (5,5) to (10,10) on same line y = x
    in.moveTo({0.0f, 0.0f});
    in.lineTo({5.0f, 5.0f});
    in.cubicTo({6.67f, 6.67f}, {8.33f, 8.33f}, {10.0f, 10.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count == 2);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
    REQUIRE(out.cmds[1] == PathCommand::LineTo);
    REQUIRE(out.pts.count == 2);
    REQUIRE(out.pts[0].x == 0.0f);
    REQUIRE(out.pts[0].y == 0.0f);
    REQUIRE(out.pts[1].x == 10.0f);
    REQUIRE(out.pts[1].y == 10.0f);
}

TEST_CASE("RenderPath optimize - Multiple Subpaths", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.lineTo({10.0f, 10.0f});
    in.moveTo({10.0f, 10.0f});
    in.lineTo({20.0f, 20.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count == 4);
    uint32_t moveToCount = 0;
    for (uint32_t i = 0; i < out.cmds.count; i++) {
        if (out.cmds[i] == PathCommand::MoveTo) moveToCount++;
    }
    REQUIRE(moveToCount == 2);
}

TEST_CASE("RenderPath optimize - With Scale Up Transform Matrix", "[tvgRenderPath]")
{
    auto matrix = Matrix{2.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.lineTo({10.0f, 10.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count == 2);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
    REQUIRE(out.cmds[1] == PathCommand::LineTo);
    REQUIRE(out.pts.count == 2);
}

TEST_CASE("RenderPath optimize - With Shrink Transform Matrix", "[tvgRenderPath]")
{
    auto matrix = Matrix{0.001f, 0.0f, 0.0f, 0.0f, 0.001f, 0.0f, 0.0f, 0.0f, 1.0f};

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.lineTo({10.0f, 10.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);


    REQUIRE(out.cmds.count == 1);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
    REQUIRE(out.pts.count == 1);
}

TEST_CASE("RenderPath optimize - Complex Path", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.lineTo({10.0f, 0.0f});
    in.lineTo({10.0f, 10.0f});
    in.lineTo({0.0f, 10.0f});
    in.close();
    in.moveTo({20.0f, 20.0f});
    in.cubicTo({25.0f, 20.0f}, {30.0f, 25.0f}, {30.0f, 30.0f});
    in.close();

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count >= 2);
    REQUIRE(out.pts.count >= 2);
}

TEST_CASE("RenderPath optimize - Merge Duplicate CubicTo", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.cubicTo({0.05f, 0.0f}, {0.05f, 0.05f}, {0.05f, 0.05f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count == 1);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
}

TEST_CASE("RenderPath optimize - Three Collinear Lines", "[tvgRenderPath]")
{
    auto matrix = tvg::identity();

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.lineTo({10.0f, 0.0f});
    in.lineTo({20.0f, 0.0f});
    in.lineTo({30.0f, 0.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count == 2);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
    REQUIRE(out.cmds[1] == PathCommand::LineTo);
    REQUIRE(out.pts.count == 2);
    REQUIRE(out.pts[0].x == 0.0f);
    REQUIRE(out.pts[0].y == 0.0f);
    REQUIRE(out.pts[1].x == 30.0f);
    REQUIRE(out.pts[1].y == 0.0f);
}

TEST_CASE("RenderPath optimize - Flat Cubic with Shrink Matrix", "[tvgRenderPath]")
{
    auto matrix = Matrix{0.01f, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f, 0.0f, 1.0f};

    RenderPath in;
    in.moveTo({0.0f, 0.0f});
    in.cubicTo({50.0f, 50.0f}, {50.0f, 50.0f}, {100.0f, 100.0f});

    RenderPath out;
    RenderPath::optimize(in, out, matrix);

    REQUIRE(out.cmds.count == 2);
    REQUIRE(out.cmds[0] == PathCommand::MoveTo);
    REQUIRE(out.pts.count == 2);
    REQUIRE(out.pts[0].x == 0.0f);
    REQUIRE(out.pts[0].y == 0.0f);
    REQUIRE(out.pts[1].x == 100.0f);
    REQUIRE(out.pts[1].y == 100.0f);
}

