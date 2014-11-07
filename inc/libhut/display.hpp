/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste "Jiboo" Lepesme
 * github.com/jiboo/libhut @lepesmejb +JeanBaptisteLepesme
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <functional>
#include <chrono>
#include <list>
#include <map>
#include <tuple>
#include <algorithm>

namespace hut {

    class base_display {
    public:
        virtual ~base_display() {
        }

        inline auto post(std::function<void()> callback);
        inline void unpost(auto callback_ref);

        inline auto post_delayed(std::function<void()> callback, std::chrono::milliseconds delay);
        inline void unpost_delayed(auto callback_ref);

        inline auto schedule(std::function<void()> callback, std::chrono::milliseconds delay);
        inline void unschedule(auto callback_ref);

        virtual void flush() = 0;

        virtual uint16_t max_texture_size() = 0;

    protected:
        using time_point = std::chrono::steady_clock::time_point;
        using duration = std::chrono::steady_clock::duration;

        std::list<std::function<void()>> posted_jobs;
        std::map<time_point, std::function<void()>> delayed_jobs;
        std::map<time_point, std::tuple<std::function<void()>, duration>> scheduled_jobs;

        virtual void dispatch_draw() {

        }

        void tick_jobs() {
            for(auto job : posted_jobs)
                job();
            posted_jobs.clear();

            time_point now = std::chrono::steady_clock::now();

            for(auto it = delayed_jobs.begin(); it != delayed_jobs.end(); it++) {
                if(it->first < now) {
                    it->second();
                    delayed_jobs.erase(it);
                }
            }

            for(auto it = scheduled_jobs.begin(); it != scheduled_jobs.end(); it++) {
                if(it->first < now) {
                    std::get<0>(it->second)();

                    scheduled_jobs.emplace(now + std::get<1>(it->second), it->second);
                    scheduled_jobs.erase(it);
                }
            }
        }
    };

    auto base_display::post(std::function<void()> callback) {
        return posted_jobs.emplace(posted_jobs.end(), callback);
    }
    void base_display::unpost(auto callback_ref) {
        posted_jobs.erase(callback_ref);
    }

    auto base_display::post_delayed(std::function<void()> callback, std::chrono::milliseconds delay) {
        return delayed_jobs.emplace(std::chrono::steady_clock::now() + delay, callback);
    }
    void base_display::unpost_delayed(auto callback_ref) {
        delayed_jobs.erase(callback_ref);
    }

    auto base_display::schedule(std::function<void()> callback, std::chrono::milliseconds delay) {
        return scheduled_jobs.emplace(std::chrono::steady_clock::now() + delay, std::make_tuple(callback, delay));
    }
    void base_display::unschedule(auto callback_ref) {
        scheduled_jobs.erase(callback_ref);
    }

} //namespace hut

#ifdef HUT_WAYLAND

#include "libhut/wayland/display.hpp"

#endif

