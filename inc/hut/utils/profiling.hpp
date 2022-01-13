/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste Lepesme github.com/jiboo/libhut
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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

#include <cassert>
#include <cstring>

#include <iomanip>
#include <sstream>

#ifdef HUT_ENABLE_PROFILING
#  include <array>
#  include <atomic>
#  include <charconv>
#  include <chrono>
#  include <filesystem>
#  include <fstream>
#  include <iomanip>
#  include <iostream>
#  include <mutex>
#  include <sstream>
#  include <thread>
#  include <tuple>
#  include <unordered_map>
#  include <unordered_set>
#  include <vector>

#  include <boost/functional/hash.hpp>
#  include <boost/preprocessor/cat.hpp>
#  include <boost/preprocessor/seq/to_tuple.hpp>
#  include <boost/preprocessor/seq/transform.hpp>
#  include <boost/preprocessor/stringize.hpp>
#  include <boost/preprocessor/tuple/enum.hpp>
#  include <boost/preprocessor/variadic/to_seq.hpp>
#  include <boost/stacktrace.hpp>
#  include <fmt/chrono.h>
#  include <fmt/core.h>
#  include <fmt/ostream.h>
#endif  // HUT_ENABLE_PROFILING

#include "hut/utils/math.hpp"

namespace hut {
enum profiling_category : u8 {
  PDEFAULT,
  PDISPLAY,
  PWINDOW,
  PSTAGING,
  PBUFFER,
  PIMAGE,
  PFONT,
  PVULKAN,
  PGPU,
  PEVENT,
  PPIPELINE
};

inline std::ostream &operator<<(std::ostream &_os, const profiling_category &_in) {
  switch (_in) {
    case PDEFAULT: return _os << "?";
    case PDISPLAY: return _os << "display";
    case PWINDOW: return _os << "window";
    case PSTAGING: return _os << "staging";
    case PIMAGE: return _os << "image";
    case PFONT: return _os << "font";
    case PGPU: return _os << "gpu";
    case PVULKAN: return _os << "vulkan";
    case PEVENT: return _os << "event";
    case PPIPELINE: return _os << "pipeline";
    case PBUFFER: return _os << "buffer";
  }
  return _os;
}

inline void escape_json(std::ostream &_os, std::string_view _in) {
  std::stringstream buffer;

  for (char c : _in) {
    if (c == '"' || c == '\\' || ('\x00' <= c && c <= '\x1f')) {
      buffer << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
    } else {
      buffer << c;
    }
  }

  _os << buffer.str();
}

template<typename T, T... TValues>
constexpr T max() {
  T values[] = {TValues...};
  T result   = values[0];
  for (size_t i = 0; i < sizeof...(TValues); i++)
    result = std::max(result, values[i]);
  return result;
}

template<typename T, T... TValues>
constexpr T sum() {
  T values[] = {TValues...};
  T result   = 0;
  for (size_t i = 0; i < sizeof...(TValues); i++)
    result += values[i];
  return result;
}

template<size_t TSize>
struct fixed_string {
  static constexpr size_t size     = TSize;
  static constexpr size_t str_size = TSize - 1;
  using data_type                  = char[size];

  data_type data_ = {};

  fixed_string()                         = default;
  fixed_string(const fixed_string &)     = default;
  fixed_string(fixed_string &&) noexcept = default;

  constexpr fixed_string(const char *_in, size_t _byte_size) {
    auto min = std::min(str_size, _byte_size);
    for (size_t i = 0; i != min; ++i)
      data_[i] = _in[i];
    if (_byte_size > min && TSize > 3) {
      data_[min - 3] = '.';  // NOTE JBL: UTF8 sequence for h ellipsis would be 0xE2 0x80 0xA6, so no gains
      data_[min - 2] = '.';
      data_[min - 1] = '.';
    }
    data_[min] = 0;
  }

  constexpr fixed_string(const char (&_in)[TSize])
      : fixed_string(_in, TSize - 1) {}
};

template<size_t TMaxSize>
fixed_string<TMaxSize> make_fixed(std::string_view _view) {
  return fixed_string<TMaxSize>{_view.data(), _view.size()};
}

template<size_t TMaxSize>
fixed_string<TMaxSize> make_fixed(std::u8string_view _view) {
  return fixed_string<TMaxSize>{(char *)_view.data(), _view.size()};
}

template<size_t TMaxSize>
fixed_string<TMaxSize> make_fixed(const char *_in) {
  return _in != nullptr ? fixed_string<TMaxSize>{_in, strlen(_in)} : fixed_string<TMaxSize>{nullptr, 0};
}

template<size_t TSize>
inline std::ostream &operator<<(std::ostream &_os, const fixed_string<TSize> &_in) {
  return _os << _in.data_;
}

template<size_t... TSizes>
struct fixed_string_array {
  static constexpr size_t count     = sizeof...(TSizes);
  static constexpr size_t data_size = sum<size_t, TSizes...>();
  using data_type                   = char[data_size];
  using offsets_type                = size_t[count];

  data_type    data_    = {};
  offsets_type offsets_ = {};

  fixed_string_array(const fixed_string_array &)     = default;
  fixed_string_array(fixed_string_array &&) noexcept = default;

  constexpr fixed_string_array(const char (&..._in)[TSizes]) { init(0, 0, _in...); }

  template<size_t TFirstSize, size_t... TRestSizes>
  constexpr void init(size_t _index, size_t _byte_offset, const char (&_first)[TFirstSize],
                      const char (&..._rest)[TRestSizes]) {
    offsets_[_index] = _byte_offset;
    for (size_t i = 0; i != TFirstSize; ++i)
      data_[i + _byte_offset] = _first[i];
    if constexpr (sizeof...(TRestSizes) > 0)
      init(_index + 1, _byte_offset + TFirstSize, _rest...);
  }

  constexpr void init(size_t, size_t) {}

  [[nodiscard]] constexpr const char *at(size_t _index) const {
    assert(_index < count);
    return data_ + offsets_[_index];
  }
};

}  // namespace hut

namespace hut::profiling {

using hut::operator<<;

#ifdef HUT_ENABLE_PROFILING

struct noop_component {
  template<typename... TNextCtorArgs>
  noop_component(TNextCtorArgs &&..._rest) {
    static_assert(sizeof...(TNextCtorArgs) == 0, "Some construction parameters were discarded");
  }

  void dump(std::ostream &_os) {}
};

template<typename TNextParent>
struct category_component : TNextParent {
  profiling_category category_;

  template<typename... TNextCtorArgs>
  category_component(profiling_category _cat, TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , category_{_cat} {}

  void dump(std::ostream &_os) {
    _os << ",\"cat\":\"" << category_ << '\"';
    TNextParent::dump(_os);
  }
};

enum type : u8 {
  BEGIN,
  END,
  INSTANT,
  COMPLETE,
};
inline std::ostream &operator<<(std::ostream &_os, const type &_in) {
  switch (_in) {
    case type::BEGIN: return _os << "B";
    case type::END: return _os << "E";
    case type::INSTANT: return _os << "I";
    case type::COMPLETE: return _os << "X";
  }
  return _os;
}

template<typename TNextParent>
struct type_component : TNextParent {
  type type_;

  template<typename... TNextCtorArgs>
  type_component(type _type, TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , type_{_type} {}

  void dump(std::ostream &_os) {
    _os << ",\"ph\":\"" << type_ << '\"';
    TNextParent::dump(_os);
  }
};

template<typename TInternalClock, typename TRep>
class diff_clock_wrapper {
 public:
  using internal                  = TInternalClock;
  using rep                       = TRep;
  using period                    = std::micro;
  using duration                  = std::chrono::duration<rep, period>;
  using time_point                = std::chrono::time_point<diff_clock_wrapper, duration>;
  static constexpr bool is_steady = TInternalClock::is_steady;

  static inline const typename TInternalClock::time_point epoch = TInternalClock::now();

  static time_point now() { return time_point{std::chrono::duration_cast<duration>(TInternalClock::now() - epoch)}; }
};
using clock_f32 = diff_clock_wrapper<std::chrono::steady_clock, float>;

template<typename TNextParent>
struct timestamp_component : TNextParent {
  using time_point = typename clock_f32::time_point;
  time_point timestamp_;

  template<typename... TNextCtorArgs>
  timestamp_component(TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , timestamp_{clock_f32::now()} {}

  template<typename... TNextCtorArgs>
  timestamp_component(time_point _tp, TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , timestamp_{_tp} {}

  void dump(std::ostream &_os) {
    _os << ",\"ts\":\""
        << std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(timestamp_.time_since_epoch()).count()
        << '\"';
    TNextParent::dump(_os);
  }
};

template<typename TNextParent>
struct duration_component : TNextParent {
  using duration = clock_f32::duration;

  clock_f32::duration duration_;

  template<typename... TNextCtorArgs>
  duration_component(duration _dur, TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , duration_{_dur} {}

  void dump(std::ostream &_os) {
    _os << ",\"dur\":\"" << std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(duration_).count()
        << '\"';
    TNextParent::dump(_os);
  }
};

template<typename TOut, typename TIn>
static TOut rehash(const TIn &_in) {
  const TOut *ptr    = reinterpret_cast<const TOut *>(&_in);
  TOut        result = *ptr;
  for (size_t i = 1; i < sizeof(TIn) / sizeof(TOut); i++)
    result ^= ptr[i];
  return result;
}

template<typename TRep>
static TRep this_thread() {
  return rehash<TRep>(std::this_thread::get_id());
}

template<typename TNextParent>
struct threadid_component : TNextParent {
  u16 thread_;

  template<typename... TNextCtorArgs>
  threadid_component(TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , thread_{this_thread<u16>()} {}

  template<typename... TNextCtorArgs>
  threadid_component(u16 _tid, TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , thread_{_tid} {}

  void dump(std::ostream &_os) {
    _os << ",\"tid\":\"" << thread_ << '\"';
    TNextParent::dump(_os);
  }
};

template<fixed_string TName, typename TNextParent>
struct static_threadid_component : TNextParent {
  template<typename... TNextCtorArgs>
  static_threadid_component(TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...} {}

  void dump(std::ostream &_os) {
    _os << ",\"tid\":\"" << TName << '\"';
    TNextParent::dump(_os);
  }
};

using stacktrace = boost::stacktrace::stacktrace;

template<typename TNextParent>
struct stacktrace_component : TNextParent {
  u32 stacktrace_hash_;

  template<typename... TNextCtorArgs>
  stacktrace_component(stacktrace &&_stacktrace, TNextCtorArgs &&..._rest);

  void dump(std::ostream &_os) {
    _os << ",\"sf\":" << stacktrace_hash_;
    TNextParent::dump(_os);
  }
};

template<size_t TArgCount>
using arg_names = std::array<std::string_view, TArgCount>;

template<typename... TArgs>
using args_tuple = std::tuple<std::decay_t<TArgs>...>;

enum dispatch_op { DUMP, DESTROY };

template<fixed_string TFormat, fixed_string_array TArgNames, typename TEventType, typename... TEventArgs>
void dispatcher_impl(TEventType &_thiz, dispatch_op _op, void *_data) {
  switch (_op) {
    case DUMP: _thiz.template dump_impl<TFormat, TArgNames, TEventArgs...>(*static_cast<std::ostream *>(_data)); break;
    case DESTROY: _thiz.template destroy_impl<TEventArgs...>(); break;
  }
}

template<size_t TTotalByteSize, typename TFirstParent>
class basic_event : public TFirstParent {
 public:
  using dispatcher = void (*)(basic_event &, dispatch_op, void *);

 public:
  static constexpr size_t total_size  = TTotalByteSize;
  static constexpr size_t header_size = sizeof(TFirstParent) + sizeof(dispatcher);
  static constexpr size_t data_size   = total_size - header_size;

 private:

 public:
  static_assert(total_size > header_size, "Total size is too small");

  using args_storage_container = std::array<u8, data_size>;

  dispatcher             dispatcher_;
  args_storage_container args_storage_;

  template<typename... TArgs>
  args_tuple<TArgs...> &args_as() {
    return *reinterpret_cast<args_tuple<TArgs...> *>(args_storage_.data());
  }

  template<typename... TArgs>
  const args_tuple<TArgs...> &args_as() const {
    return *reinterpret_cast<args_tuple<TArgs...> *>(args_storage_.data());
  }

  template<typename... TArgs>
  void destroy_impl() {
    args_as<TArgs...>().~tuple();
  }

  static void dump_impl_rec(std::ostream &_os, size_t _index) {}

  template<fixed_string_array TArgNames, typename TLast>
  static void dump_impl_rec(std::ostream &_os, size_t _index, TLast &&_last) {
    _os << '"';
    escape_json(_os, TArgNames.at(_index));
    _os << "\":\"" << _last << '"';
  }

  template<fixed_string_array TArgNames, typename TFirst, typename... TArgs>
  static void dump_impl_rec(std::ostream &_os, size_t _index, TFirst &&_first, TArgs &&..._rest) {
    _os << '"';
    escape_json(_os, TArgNames.at(_index));
    _os << "\":\"" << _first << "\",";
    basic_event::dump_impl_rec<TArgNames>(_os, _index + 1, std::forward<TArgs>(_rest)...);
  }

  template<fixed_string TFormat, fixed_string_array TArgNames, typename... TArgs>
  void dump_impl(std::ostream &_os) {
    _os << "{\"name\":\"";

    escape_json(_os, std::apply(
                         [](auto &&..._args) {
                           try {
                             return fmt::format(TFormat.data_, std::forward<decltype(_args)>(_args)...);
                           } catch (...) { return std::string(TFormat.data_); }
                         },
                         args_as<TArgs...>()));

    _os << "\",\"pid\":1";
    TFirstParent::dump(_os);
    if constexpr (sizeof...(TArgs) > 0) {
      _os << ",\"args\":{";

      std::apply(
          [&_os](auto &&..._args) {
            basic_event::dump_impl_rec<TArgNames>(_os, 0, std::forward<decltype(_args)>(_args)...);
          },
          args_as<TArgs...>());

      _os << "}";
    }
    _os << "}";
  }

 public:
  template<typename... TEventArgs, typename... TParentsCtorArgs>
  basic_event(const dispatcher &_dispatcher, std::tuple<TEventArgs...> &&_event_args,
              TParentsCtorArgs &&..._parent_args)
      : TFirstParent{std::forward<TParentsCtorArgs>(_parent_args)...}
      , dispatcher_{_dispatcher} {
    static_assert(data_size >= sizeof(_event_args), "Not enough args storage");
    new (&args_as<TEventArgs...>()) args_tuple<TEventArgs...>(std::move(_event_args));
  }

  template<typename... TParentsCtorArgs>
  basic_event(const dispatcher &_dispatcher, TParentsCtorArgs &&..._parent_args)
      : TFirstParent{std::forward<TParentsCtorArgs>(_parent_args)...}
      , dispatcher_{_dispatcher} {}

  void dump(std::ostream &_os) { dispatcher_(*this, DUMP, &_os); }

  ~basic_event() { dispatcher_(*this, DESTROY, nullptr); }
};

using complete_components = timestamp_component<  //4
    duration_component<                           // 4
        stacktrace_component<                     //4
            threadid_component<                   //2
                type_component<                   //1
                    category_component<           //1
                        noop_component>>>>>>;

#  ifdef __cpp_lib_hardware_interference_size
constexpr size_t cache_line_byte_size = std::hardware_constructive_interference_size;
#  else
constexpr size_t cache_line_byte_size = 2 * sizeof(std::max_align_t);
#  endif

using complete_event = basic_event<cache_line_byte_size, complete_components>;

namespace size_tests {
static_assert(sizeof(timestamp_component<noop_component>) == 4);
static_assert(sizeof(threadid_component<noop_component>) == 2);
static_assert(sizeof(type_component<noop_component>) == 1);
static_assert(sizeof(category_component<noop_component>) == 1);
static_assert(sizeof(stacktrace_component<noop_component>) == sizeof(u32));

static_assert(complete_event::header_size == 24);
static_assert(complete_event::data_size == 40);
}  // namespace size_tests

using cpu_frames = std::vector<std::vector<complete_event>>;

constexpr size_t frame_history = 20;
struct thread_data {
  cpu_frames completed_{frame_history};

  std::unordered_map<u32, stacktrace> stacktrace_cache_;
};

thread_local inline thread_data *tls_data_ = nullptr;

struct threads_data {
  static inline std::mutex                                                        mapping_mutex_;
  static inline std::unordered_map<std::thread::id, std::unique_ptr<thread_data>> threads_mapping_;
  static inline std::atomic_uint                                                  frame_index_;

  static inline bool             dump_requested_   = false;
  static inline std::atomic_bool dump_in_progress_ = false;

  static thread_data &get(std::thread::id _id = std::this_thread::get_id()) {
    if (tls_data_ != nullptr)
      return *tls_data_;

    std::scoped_lock lock{mapping_mutex_};
    auto             inserted = threads_mapping_.emplace(_id, std::make_unique<thread_data>());
    assert(inserted.second);
    tls_data_ = inserted.first->second.get();
    return *tls_data_;
  }

  static std::vector<complete_event> &my_queue() { return get().completed_[frame_index_]; }

  static void next_frame() {
    std::scoped_lock lock{mapping_mutex_};
    if (dump_requested_ && !dump_in_progress_.load()) {
      dump_in_progress_.store(true);
      std::thread{background_dump, collect_frames()}.detach();
    }
    dump_requested_ = false;

    auto prev = frame_index_.load();
    auto next = (prev + 1) % frame_history;
    for (auto &thread : threads_mapping_) {
      thread.second->completed_[next].clear();
    }
    frame_index_.store(next);
  }

  static void request_dump() { dump_requested_ = true; }

  struct frame_cache {
    std::string name_, source_file_;
    size_t      line_;
  };

  static void dump_stackframes(std::ostream &_os) {
    std::unordered_set<size_t>                    done;
    std::unordered_map<const void *, frame_cache> cache;

    for (auto &thread_data : threads_mapping_) {
      for (const auto &entry : thread_data.second->stacktrace_cache_) {
        if (!done.emplace(entry.first).second)
          continue;
        const auto &vec  = entry.second.as_vector();
        const auto  size = (int)vec.size();
        for (int i = size - 1; i >= 0; --i) {
          _os << ",\n\"" << entry.first;
          if (i != 0)
            _os << '_' << i;

          auto frame   = vec[i];
          auto itcache = cache.find(frame.address());
          if (itcache == cache.end()) {
            auto result
                = cache.emplace(frame.address(), frame_cache{frame.name(), frame.source_file(), frame.source_line()});
            itcache = result.first;
          }

          _os << "\":{\"name\":\"";
          escape_json(_os, itcache->second.name_);
          _os << " at ";
          escape_json(_os, itcache->second.source_file_);
          _os << ':' << itcache->second.line_;
          if (i != (size - 1))
            _os << "\",\"parent\":\"" << entry.first << '_' << (i + 1);
          _os << "\"}";
        }
      }
    }
  }

  struct frames {
    cpu_frames cpu_frames_;
  };

  static frames collect_frames() {
    assert(mapping_mutex_.try_lock() == false);

    frames result;
    for (auto &thread : threads_mapping_) {
      for (auto &frame : thread.second->completed_) {
        result.cpu_frames_.emplace_back(std::move(frame));
      }
    }
    return result;
  }

  static std::filesystem::path unique_profiline_path() {
    constexpr std::string_view prefix = "profiling-";

    static bool cleaned = false;
    if (!cleaned) {
      for (auto &entry : std::filesystem::directory_iterator(".")) {
        auto             stem_str = entry.path().stem().string();
        std::string_view stem     = stem_str;
        if (stem.starts_with(prefix)) {
          std::filesystem::remove(entry);
        }
      }
      cleaned = true;
    }

    static unsigned   last_index = 0;
    std::stringstream filename;
    filename << prefix << (++last_index % 20) << ".json";
    return filename.str();
  }

  static void background_dump(frames &&_frames) {
    auto filename = unique_profiline_path();
    std::cout << "[hut] Starting background profiling dump to: " << filename << "..." << std::endl;
    std::ofstream os{filename};

    os << std::fixed << "{\"traceEvents\":[{}";
    for (auto &frame : _frames.cpu_frames_) {
      for (auto &event : frame) {
        os << "," << std::endl;
        event.dump(os);
      }
    }
    os << "],\"stackFrames\":{\"dummy\":{\"name\":\"dummy\"}";
    dump_stackframes(os);
    os << "}}" << std::defaultfloat << std::endl;

    std::cout << "[hut] Wrote profiling to: " << filename << std::endl;
    dump_in_progress_.store(false);
  }
};

template<typename TNextParent>
template<typename... TNextCtorArgs>
stacktrace_component<TNextParent>::stacktrace_component(stacktrace &&_stacktrace, TNextCtorArgs &&..._rest)
    : TNextParent{std::forward<TNextCtorArgs>(_rest)...} {
  stacktrace_hash_ = rehash<u32>(boost::stacktrace::hash_value(_stacktrace));
  {
    auto &thread_data = threads_data::get();
    thread_data.stacktrace_cache_.try_emplace(stacktrace_hash_, std::move(_stacktrace));
  }
}

template<fixed_string TFormat, fixed_string_array TArgNames, typename TEventType, typename... TEventArgs>
struct complete_event_scope {
  using dispatcher = typename TEventType::dispatcher;

  std::vector<TEventType>  &buffer_;
  profiling_category        cat_;
  clock_f32::time_point     start_;
  stacktrace                stacktrace_;
  std::tuple<TEventArgs...> args_;
  dispatcher                dispatcher_;

  complete_event_scope(std::vector<TEventType> &_buffer, profiling_category _cat, clock_f32::time_point _start,
                       stacktrace &&_stacktrace, std::tuple<TEventArgs...> &&_event_args)
      : buffer_{_buffer}
      , cat_{_cat}
      , start_{_start}
      , stacktrace_{std::move(_stacktrace)}
      , args_{std::move(_event_args)}
      , dispatcher_{dispatcher_impl<TFormat, TArgNames, TEventType, TEventArgs...>} {}

  ~complete_event_scope() {
    buffer_.emplace_back(dispatcher_, std::move(args_), start_, (clock_f32::now() - start_), std::move(stacktrace_),
                         type::COMPLETE, cat_);
  }
};

template<fixed_string TFormat, fixed_string_array TArgNames, typename TEventType, typename... TEventArgs>
auto make_complete_event_scope(std::vector<TEventType> &_buffer, profiling_category _cat, clock_f32::time_point _start,
                               stacktrace &&_stacktrace, std::tuple<TEventArgs...> &&_event_args) {
  return complete_event_scope<TFormat, TArgNames, TEventType, TEventArgs...>{
      _buffer, _cat, _start, std::move(_stacktrace), std::move(_event_args)};
}

#  define HUT_PROFILE_TRANSFORM_STRINGIFY(MR, MData, MElement) BOOST_PP_STRINGIZE(MElement)

#  define HUT_PROFILE_MAP_DETAIL(MTransform, MSeq) BOOST_PP_SEQ_TO_TUPLE(BOOST_PP_SEQ_TRANSFORM(MTransform, _, MSeq))
#  define HUT_PROFILE_MAP(MTransform, ...)         HUT_PROFILE_MAP_DETAIL(MTransform, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#  define HUT_PROFILE_UNIQUE_SYMBOL(MPrefix) BOOST_PP_CAT(MPrefix##_scope, __COUNTER__)

#  define HUT_PROFILE_SCOPE_IMPL_DATANAMED(MCat, MFormat, MArgNames, MArgNamesVar, MDataVar, ...)                      \
    auto HUT_PROFILE_UNIQUE_SYMBOL(_scope)                                                                             \
        = ::hut::profiling::make_complete_event_scope<MFormat,                                                         \
                                                      ::hut::fixed_string_array{BOOST_PP_TUPLE_ENUM(MArgNames)}>(      \
            ::hut::profiling::threads_data::my_queue(), MCat, ::hut::profiling::clock_f32::now(),                      \
            ::hut::profiling::stacktrace{1, 8}, std::make_tuple(__VA_ARGS__))

#  define HUT_PROFILE_SCOPE_IMPL(MCat, MFormat, MArgNames, ...)                                                        \
    HUT_PROFILE_SCOPE_IMPL_DATANAMED(MCat, MFormat, MArgNames, HUT_PROFILE_UNIQUE_SYMBOL(_names),                      \
                                     HUT_PROFILE_UNIQUE_SYMBOL(_data), __VA_ARGS__)

#  define HUT_PROFILE_SCOPE(MCat, MFormat, ...)                                                                        \
    HUT_PROFILE_SCOPE_IMPL(MCat, MFormat, HUT_PROFILE_MAP(HUT_PROFILE_TRANSFORM_STRINGIFY, __VA_ARGS__), __VA_ARGS__)
#  define HUT_PROFILE_SCOPE_NAMED(MCat, MFormat, MArgNames, ...)                                                       \
    HUT_PROFILE_SCOPE_IMPL(MCat, MFormat, MArgNames, __VA_ARGS__)

#  define HUT_PROFILE_EVENT(MTarget, MEvent, ...)                                                                      \
    [&]() -> bool {                                                                                                    \
      HUT_PROFILE_SCOPE(PEVENT, "Event " BOOST_PP_STRINGIZE(MEvent), __VA_ARGS__);                                     \
      return MTarget->MEvent.fire(__VA_ARGS__);                                                                        \
    }()
#  define HUT_PROFILE_EVENT_NAMED(MTarget, MEvent, MArgNames, ...)                                                     \
    [&]() -> bool {                                                                                                    \
      HUT_PROFILE_SCOPE_NAMED(PEVENT, "Event " BOOST_PP_STRINGIZE(MEvent), MArgNames, __VA_ARGS__);                    \
      return MTarget->MEvent.fire(__VA_ARGS__);                                                                        \
    }()
#  define HUT_PROFILE_EVENT_NAMED_ALIASED(MTarget, MEvent, MArgNames, MArgValues, ...)                                 \
    [&]() -> bool {                                                                                                    \
      HUT_PROFILE_SCOPE_NAMED(PEVENT, "Event " BOOST_PP_STRINGIZE(MEvent), MArgNames,                                  \
                                                                  BOOST_PP_TUPLE_ENUM(MArgValues));                    \
      return MTarget->MEvent.fire(__VA_ARGS__);                                                                        \
    }()

#  define HUT_PVK(MName, ...)                                                                                          \
    [&]() {                                                                                                            \
      HUT_PROFILE_SCOPE(::hut::PVULKAN, BOOST_PP_STRINGIZE(MName));                                                    \
      return MName(__VA_ARGS__);                                                                                       \
    }()

#  define HUT_PVK_NAMED_ALIASED(MName, MArgNames, MArgValues, ...)                                                     \
    [&]() {                                                                                                            \
      HUT_PROFILE_SCOPE_NAMED(PVULKAN, BOOST_PP_STRINGIZE(MName), MArgNames, BOOST_PP_TUPLE_ENUM(MArgValues));         \
      return MName(__VA_ARGS__);                                                                                       \
    }()

#else  // HUT_ENABLE_PROFILING

#  define HUT_PROFILE_SCOPE(MCat, MFormat, ...)
#  define HUT_PROFILE_SCOPE_NAMED(MCat, MFormat, MArgNames, ...)

#  define HUT_PROFILE_EVENT(MTarget, MEvent, ...)                                      MTarget->MEvent.fire(__VA_ARGS__)
#  define HUT_PROFILE_EVENT_NAMED(MTarget, MEvent, MArgNames, ...)                     MTarget->MEvent.fire(__VA_ARGS__)
#  define HUT_PROFILE_EVENT_NAMED_ALIASED(MTarget, MEvent, MArgNames, MArgValues, ...) MTarget->MEvent.fire(__VA_ARGS__)

#  define HUT_PVK(MName, ...) MName(__VA_ARGS__)

#  define HUT_PVK_NAMED_ALIASED(MName, MArgNames, MArgValues, ...) MName(__VA_ARGS__);

struct threads_data {
  static void next_frame() {}
  static void request_dump() {}
};

#endif  // HUT_ENABLE_PROFILING

}  // namespace hut::profiling
