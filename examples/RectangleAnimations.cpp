/*
 * Copyright (c) 2025 the ThorVG project. All rights reserved.

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

#include <vector>
#include <cmath>
#include <fstream>
#include <chrono>
#include "Example.h"

/************************************************************************/
/* ThorVG Drawing Contents                                              */
/************************************************************************/

struct UserExample : tvgexam::Example
{
    struct AnimatedRect {
        tvg::Shape* shape;
        float x, y;
        float targetX, targetY;
        float width, height;
        float rotation;
        float rotationSpeed;
        float speed;
        uint8_t r, g, b;
        float phase;
        float scalePhase;
        float baseScale;
    };

    int rectCount = 30000;
    std::vector<AnimatedRect> rectangles;
    uint32_t w, h;
    float time = 0.0f;

    // FPS tracking - store in memory
    struct FpsRecord {
        uint32_t frame;
        double fps;
        int64_t timestamp;
    };
    std::vector<FpsRecord> fpsRecords;
    uint32_t frameCount = 0;
    std::chrono::steady_clock::time_point lastFpsTime;
    std::chrono::steady_clock::time_point startTime;

    float randomFloat(float min, float max)
    {
        return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
    }

    void setNewTarget(AnimatedRect& rect)
    {
        rect.targetX = randomFloat(0, w);
        rect.targetY = randomFloat(0, h);
        rect.speed = randomFloat(0.5f, 2.0f);
    }

    bool content(tvg::Canvas* canvas, uint32_t w, uint32_t h) override
    {
        this->w = w;
        this->h = h;

        srand(42);

        // Initialize FPS tracking
        fpsRecords.reserve(1000); // Pre-allocate for ~100 seconds at 10 fps recording
        startTime = std::chrono::steady_clock::now();
        lastFpsTime = startTime;

        // Background
        auto bg = tvg::Shape::gen();
        bg->appendRect(0, 0, w, h);
        bg->fill(15, 15, 25);
        canvas->push(bg);

        // Create thousands of animated rectangles
        rectangles.reserve(rectCount);

        for (int i = 0; i < rectCount; ++i) {
            auto shape = tvg::Shape::gen();
            
            AnimatedRect rect;
            rect.shape = shape;
            rect.x = randomFloat(0, w);
            rect.y = randomFloat(0, h);
            rect.targetX = randomFloat(0, w);
            rect.targetY = randomFloat(0, h);
            rect.width = randomFloat(5, 30);
            rect.height = randomFloat(5, 30);
            rect.rotation = randomFloat(0, 360);
            rect.rotationSpeed = randomFloat(-2.0f, 2.0f);
            rect.speed = randomFloat(0.5f, 2.0f);
            rect.phase = randomFloat(0, 6.28f);
            rect.scalePhase = randomFloat(0, 6.28f);
            rect.baseScale = randomFloat(0.5f, 1.5f);
            
            // Colorful palette
            int colorScheme = rand() % 5;
            switch (colorScheme) {
                case 0: // Blues
                    rect.r = rand() % 100;
                    rect.g = rand() % 150 + 100;
                    rect.b = rand() % 100 + 155;
                    break;
                case 1: // Purples
                    rect.r = rand() % 100 + 155;
                    rect.g = rand() % 100;
                    rect.b = rand() % 100 + 155;
                    break;
                case 2: // Greens
                    rect.r = rand() % 100;
                    rect.g = rand() % 100 + 155;
                    rect.b = rand() % 100;
                    break;
                case 3: // Oranges
                    rect.r = rand() % 100 + 155;
                    rect.g = rand() % 100 + 100;
                    rect.b = rand() % 100;
                    break;
                case 4: // Pinks
                    rect.r = rand() % 100 + 155;
                    rect.g = rand() % 100 + 50;
                    rect.b = rand() % 100 + 155;
                    break;
            }

            shape->appendRect(-rect.width / 2, -rect.height / 2, rect.width, rect.height);
            shape->fill(rect.r, rect.g, rect.b);
            shape->translate(rect.x, rect.y);
            shape->rotate(rect.rotation);

            canvas->push(shape);
            rectangles.push_back(rect);
        }

        return true;
    }

    bool update(tvg::Canvas* canvas, uint32_t elapsed) override
    {
        time += 0.016f; // Approximate frame time
        frameCount++;

        // Record FPS every 10 frames (store in memory)
        if (frameCount % 100 == 0) {
            auto currentTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFpsTime).count();

            if (duration > 0) {
                double fps = (100.0 * 1000.0) / duration;
                auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

                // Store in memory instead of writing to file
                fpsRecords.push_back({frameCount, fps, totalTime});

                lastFpsTime = currentTime;
            }
        }

        for (auto& rect : rectangles) {
            // Move towards target
            float dx = rect.targetX - rect.x;
            float dy = rect.targetY - rect.y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < 5.0f) {
                // Reached target, set new target
                setNewTarget(rect);
            } else {
                // Move towards target
                float moveX = (dx / distance) * rect.speed;
                float moveY = (dy / distance) * rect.speed;
                rect.x += moveX;
                rect.y += moveY;
            }

            // Update rotation
            rect.rotation += rect.rotationSpeed;
            if (rect.rotation > 360) rect.rotation -= 360;
            if (rect.rotation < 0) rect.rotation += 360;

            // Pulsating scale effect
            float scale = rect.baseScale + sin(time * 2.0f + rect.scalePhase) * 0.3f;

            // Apply transformations
            rect.shape->translate(rect.x, rect.y);
            rect.shape->rotate(rect.rotation);
            rect.shape->scale(scale);
        }

        canvas->update();
        return true;
    }

    ~UserExample()
    {
        // Write all FPS records to file at once
        if (!fpsRecords.empty()) {
            std::ofstream fpsFile("fps_log.txt");
            if (fpsFile.is_open()) {
                fpsFile << "Frame,FPS,Timestamp(ms)\n";
                for (const auto& record : fpsRecords) {
                    fpsFile << record.frame << "," << record.fps << "," << record.timestamp << "\n";
                }
                fpsFile.close();
                std::cout << "FPS log written with " << fpsRecords.size() << " records." << std::endl;
            } else {
                std::cerr << "Failed to write fps_log.txt" << std::endl;
            }
        }
    }
};


/************************************************************************/
/* Entry Point                                                          */
/************************************************************************/

int main(int argc, char **argv)
{
    auto example = new UserExample;

    // Parse rectangle count from command-line arguments
    // Usage: ./RectangleAnimations [engine] [rect_count]
    // Example: ./RectangleAnimations gl 50000
    if (argc > 2) {
        int count = atoi(argv[2]);
        if (count > 0) {
            example->rectCount = count;
            printf("Using %d rectangles\n", count);
        }
    } else {
        printf("Using default %d rectangles\n", example->rectCount);
        printf("Usage: %s [engine] [rect_count]\n", argv[0]);
        printf("Example: %s gl 50000\n", argv[0]);
    }

    return tvgexam::main(example, argc, argv, false, 1920, 1080, 4, true);
}

