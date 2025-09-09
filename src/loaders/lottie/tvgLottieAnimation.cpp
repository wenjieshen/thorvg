/*
 * Copyright (c) 2024 - 2025 the ThorVG project. All rights reserved.

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

#include "tvgCommon.h"
#include "thorvg_lottie.h"
#include "tvgLottieLoader.h"
#include "tvgAnimation.h"
#include <iostream>
using namespace std;


LottieAnimation::LottieAnimation() = default;
LottieAnimation::~LottieAnimation() = default;


uint32_t LottieAnimation::gen(const char* slot) noexcept
{
    auto loader = PICTURE(pImpl->picture)->loader;
    if (!loader) return 0;

    return static_cast<LottieLoader*>(loader)->gen(slot);
}


Result LottieAnimation::apply(uint32_t id) noexcept
{
    auto loader = PICTURE(pImpl->picture)->loader;
    if (!loader) return Result::InsufficientCondition;

    if (static_cast<LottieLoader*>(loader)->apply(id)) {
        PAINT(pImpl->picture)->mark(RenderUpdateFlag::All);
        return Result::Success;
    }

    return Result::InvalidArguments;
}


Result LottieAnimation::del(uint32_t id) noexcept
{
    auto loader = PICTURE(pImpl->picture)->loader;
    if (!loader) return Result::InsufficientCondition;

    if (static_cast<LottieLoader*>(loader)->del(id)) {
        PAINT(pImpl->picture)->mark(RenderUpdateFlag::All);
        return Result::Success;
    }

    return Result::InvalidArguments;
}


Result LottieAnimation::segment(const char* marker) noexcept
{
    auto loader = PICTURE(pImpl->picture)->loader;
    if (!loader) return Result::InsufficientCondition;

    if (!marker) {
        static_cast<LottieLoader*>(loader)->segment(0.0f, FLT_MAX);
        return Result::Success;
    }
    
    float begin, end;
    if (!static_cast<LottieLoader*>(loader)->segment(marker, begin, end)) return Result::InvalidArguments;
    return Animation::segment(begin, end);
}


Result LottieAnimation::tween(float from, float to, float progress) noexcept
{
    auto loader = PICTURE(pImpl->picture)->loader;
    if (!loader) return Result::InsufficientCondition;
    if (!static_cast<LottieLoader*>(loader)->tween(from, to, progress)) return Result::InsufficientCondition;
    PAINT(pImpl->picture)->mark(RenderUpdateFlag::All);
    return Result::Success;
}


uint32_t LottieAnimation::markersCnt() noexcept
{
    auto loader = PICTURE(pImpl->picture)->loader;
    if (!loader) return 0;
    return static_cast<LottieLoader*>(loader)->markersCnt();
}


const char* LottieAnimation::marker(uint32_t idx) noexcept
{
    auto loader = PICTURE(pImpl->picture)->loader;
    if (!loader) return nullptr;
    return static_cast<LottieLoader*>(loader)->markers(idx);
}


Result LottieAnimation::assign(const char* layer, uint32_t ix, const char* var, float val)
{
    if (!layer || !var) return Result::InvalidArguments;

    auto loader = PICTURE(pImpl->picture)->loader;
    if (!loader) return Result::InsufficientCondition;
    if (static_cast<LottieLoader*>(loader)->assign(layer, ix, var, val)) {
        PAINT(pImpl->picture)->mark(RenderUpdateFlag::All);
        return Result::Success;
    }

    return Result::NonSupport;
}


// load 이후에 제약 X
// LottieImage -> prepare() 에서 한번에 처리
// 메소드 명 변경 : resolve(assetCallback, user_data);

// Accessor 참고
/**
     * @brief Set the access function for traversing the Picture scene tree nodes.
     *
     * @param[in] paint The paint node to traverse the internal scene-tree.
     * @param[in] func The callback function calling for every paint nodes of the Picture.
     * @param[in] data Data passed to the @p func as its argument.
     *
     * @note The bitmap based picture might not have the scene-tree.
     *
     * @note Experimental API
     */
    // Result set(Paint* paint, std::function<bool(const Paint* paint, void* data)> func, void* data) noexcept;


Result LottieAnimation::resolve(std::function<bool(Paint* paint, const char* src, void* data)> callback, void* data) noexcept
{
    PICTURE(pImpl->picture)->assetResolver = callback;
    PICTURE(pImpl->picture)->assetResolverData = data;

    // Set the asset resolver callback in the LottieLoader
    auto loader = PICTURE(pImpl->picture)->loader;
    if (loader) PICTURE(pImpl->picture)->applyResolverToLoader();
    return Result::Success;
}


LottieAnimation* LottieAnimation::gen() noexcept
{
    return new LottieAnimation;
}
