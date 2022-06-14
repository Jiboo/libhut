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

#ifdef HUT_ENABLE_PROFILING
//#  define HUT_PROFILING_PROFILE

#  include <cassert>

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

#  include "hut/utils/chrono.hpp"
#  include "hut/utils/hash.hpp"
#  include "hut/utils/math.hpp"
#  include "hut/utils/string.hpp"
#endif  // HUT_ENABLE_PROFILING

#include "hut/utils/glm.hpp"

namespace hut {

enum profiling_category : u8 {
  PDISPLAY,
  PRENDERTARGET,
  PWINDOW,
  POFFSCREEN,
  PSTAGING,
  PBUFFER,
  PIMAGE,
  PSAMPLER,
  PFONT,
  PVULKAN,
  PGPU,
  PEVENT,
  PPIPELINE,
  PWAYLAND,
};

inline std::ostream &operator<<(std::ostream &_os, const profiling_category &_in) {
  switch (_in) {
    case PDISPLAY: return _os << "display";
    case PRENDERTARGET: return _os << "rendertarget";
    case PWINDOW: return _os << "window";
    case POFFSCREEN: return _os << "offscreen";
    case PSTAGING: return _os << "staging";
    case PBUFFER: return _os << "buffer";
    case PIMAGE: return _os << "image";
    case PSAMPLER: return _os << "sampler";
    case PFONT: return _os << "font";
    case PVULKAN: return _os << "vulkan";
    case PGPU: return _os << "gpu";
    case PEVENT: return _os << "event";
    case PPIPELINE: return _os << "pipeline";
    case PWAYLAND: return _os << "wayland";
    default: assert(false); return _os;
  }
}

}  // namespace hut

namespace hut::profiling {

using hut::operator<<;

#ifdef HUT_ENABLE_PROFILING

struct noop_component {
  template<typename... TNextCtorArgs>
  explicit noop_component(TNextCtorArgs &&..._rest) {
    static_assert(sizeof...(TNextCtorArgs) == 0, "Some construction parameters were discarded");
  }

  void dump(std::ostream &_os) {}
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

template<typename TClock, typename TNextParent>
struct timestamp_component : TNextParent {
  using time_point = typename TClock::time_point;

  time_point timestamp_;

  template<typename... TNextCtorArgs>
  explicit timestamp_component(time_point _tp, TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , timestamp_{_tp} {}

  void dump(std::ostream &_os) {
    _os << ",\"ts\":\""
        << std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(timestamp_.time_since_epoch()).count()
        << '\"';
    TNextParent::dump(_os);
  }
};

template<typename TClock, typename TNextParent>
struct duration_component : TNextParent {
  using duration = typename TClock::duration;

  duration duration_;

  template<typename... TNextCtorArgs>
  explicit duration_component(duration _dur, TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , duration_{_dur} {}

  void dump(std::ostream &_os) {
    _os << ",\"dur\":\"" << std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(duration_).count()
        << '\"';
    TNextParent::dump(_os);
  }
};

template<typename TKeyType>
static TKeyType this_thread() {
  return rehash<TKeyType>(std::this_thread::get_id());
}

template<typename TKeyType, typename TNextParent>
struct threadid_component : TNextParent {
  TKeyType thread_;

  template<typename... TNextCtorArgs>
  explicit threadid_component(TKeyType _tid, TNextCtorArgs &&..._rest)
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
  explicit static_threadid_component(TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...} {}

  void dump(std::ostream &_os) {
    _os << ",\"tid\":\"" << TName << '\"';
    TNextParent::dump(_os);
  }
};

constexpr auto STACKTRACE_DEPTH = 8;
using stacktrace                = boost::stacktrace::stacktrace;
using stacktraces_registry      = std::unordered_map<u32, stacktrace>;
using stacktrace_entry          = boost::stacktrace::frame::native_frame_ptr_t;
struct resolved_entry {
  std::string name_, source_file_;
  size_t      line_;
};
using stacktrace_entries_cache = std::unordered_map<stacktrace_entry, resolved_entry>;

template<typename TKeyType, typename TNextParent>
struct stacktrace_component : TNextParent {
  TKeyType stacktrace_hash_;

  template<typename... TNextCtorArgs>
  explicit stacktrace_component(TKeyType _stacktrace_hash, TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , stacktrace_hash_{_stacktrace_hash} {}

  void dump(std::ostream &_os) {
    _os << ",\"sf\":" << stacktrace_hash_;
    TNextParent::dump(_os);
  }
};

template<typename... TArgs>
using args_tuple = std::tuple<std::decay_t<TArgs>...>;

enum dispatch_op { DUMP, DESTROY };
using dispatcher = void (*)(void *, dispatch_op, void *);

template<fixed_string TFormat, fixed_string_array TArgNames, type TProfileType, profiling_category TProfileCat,
         typename TEventType, typename... TEventArgs>
void dispatcher_impl(void *_thiz, dispatch_op _op, void *_data) {
  auto &event = *static_cast<TEventType *>(_thiz);
  switch (_op) {
    case DUMP:
      event.template dump_impl<TFormat, TArgNames, TProfileType, TProfileCat, TEventArgs...>(
          *static_cast<std::ostream *>(_data));
      break;
    case DESTROY: event.template destroy_impl<TEventArgs...>(); break;
    default: assert(false);
  }
}

template<typename TKeyType>
struct dispatcher_repository {
  static inline std::array<dispatcher, NUMAX<TKeyType>> g_repository;

  static TKeyType slot(dispatcher _dispatcher) {
    static std::atomic_uint16_t s_counter = 0;
    assert(s_counter < NUMAX<TKeyType>);
    TKeyType result      = s_counter++;
    g_repository[result] = _dispatcher;
    return result;
  }

  static void dispatch(TKeyType _key, void *_thiz, dispatch_op _op, void *_data) {
    g_repository[_key](_thiz, _op, _data);
  }
};

template<typename TKeyType, typename TNextParent>
struct dispatcher_component : TNextParent {
  TKeyType dispatcher_slot_;

  template<typename... TNextCtorArgs>
  explicit dispatcher_component(TKeyType _slot, TNextCtorArgs &&..._rest)
      : TNextParent{std::forward<TNextCtorArgs>(_rest)...}
      , dispatcher_slot_{_slot} {}

  void dump(std::ostream &_os) { TNextParent::dump(_os); }
};

template<typename TDispatcherKeyType, size_t TTotalByteSize, typename TFirstParent>
class basic_event : public TFirstParent {
 public:
  constexpr static size_t TOTAL_SIZE  = TTotalByteSize;
  constexpr static size_t HEADER_SIZE = sizeof(TFirstParent);
  constexpr static size_t DATA_SIZE   = TOTAL_SIZE - HEADER_SIZE;

  static_assert(TOTAL_SIZE > HEADER_SIZE, "Total size is too small");

  using args_storage_container = std::array<u8, DATA_SIZE>;

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

  template<fixed_string TFormat, fixed_string_array TArgNames, type TProfileType, profiling_category TProfileCat,
           typename... TArgs>
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
    _os << ",\"ph\":\"" << TProfileType << '\"';
    _os << ",\"cat\":\"" << TProfileCat << '\"';
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

  template<typename... TEventArgs, typename... TParentsCtorArgs>
  explicit basic_event(std::tuple<TEventArgs...> &&_event_args, TParentsCtorArgs &&..._parent_args)
      : TFirstParent{std::forward<TParentsCtorArgs>(_parent_args)...} {
    static_assert(DATA_SIZE >= sizeof(_event_args), "Not enough args storage");
    new (&args_as<TEventArgs...>()) args_tuple<TEventArgs...>(std::move(_event_args));
  }

  template<typename... TParentsCtorArgs>
  explicit basic_event(TParentsCtorArgs &&..._parent_args)
      : TFirstParent{std::forward<TParentsCtorArgs>(_parent_args)...} {}

  void dispatch(dispatch_op _op, void *_data) {
    dispatcher_repository<TDispatcherKeyType>::dispatch(this->dispatcher_slot_, this, _op, _data);
  }

  void dump(std::ostream &_os) { dispatch(DUMP, &_os); }

  ~basic_event() { dispatch(DESTROY, nullptr); }
};

using clock_f32      = diff_clock_wrapper<std::chrono::steady_clock, float>;
using dispatcher_u16 = u16;
using stacktrace_u32 = u32;
using threadid_u16   = u16;

// clang-format off
using complete_components =
  timestamp_component<clock_f32,
    duration_component<clock_f32,
      stacktrace_component<stacktrace_u32,
        threadid_component<threadid_u16,
          dispatcher_component<dispatcher_u16,
            noop_component>>>>>;
// clang-format on

#  ifdef __cpp_lib_hardware_interference_size
constexpr size_t CACHE_LINE_BYTE_SIZE = std::hardware_constructive_interference_size;
#  else
constexpr size_t CACHE_LINE_BYTE_SIZE = 2 * sizeof(std::max_align_t);
#  endif

using complete_event = basic_event<dispatcher_u16, CACHE_LINE_BYTE_SIZE, complete_components>;

namespace size_tests {
static_assert(sizeof(timestamp_component<clock_f32, noop_component>) == 4);
static_assert(sizeof(duration_component<clock_f32, noop_component>) == 4);
static_assert(sizeof(stacktrace_component<stacktrace_u32, noop_component>) == 4);
static_assert(sizeof(threadid_component<threadid_u16, noop_component>) == 2);
static_assert(sizeof(dispatcher_component<dispatcher_u16, noop_component>) == 2);

static_assert(complete_event::HEADER_SIZE == 16);
static_assert(complete_event::DATA_SIZE == 48);
}  // namespace size_tests

using cpu_frames = std::vector<std::vector<complete_event>>;

constexpr size_t FRAME_HISTORY = 20;
struct thread_data {
  std::mutex frames_mutex_;
  cpu_frames frames_{FRAME_HISTORY};

  std::mutex           registry_mutex_;
  stacktraces_registry registry_;
};

struct threads_data {
  using thread_data_mapping = std::unordered_map<std::thread::id, std::unique_ptr<thread_data>>;
  static inline std::mutex          g_mapping_mutex;
  static inline thread_data_mapping g_threads_mapping;
  static inline std::atomic_uint    g_frame_index;

  static inline bool             g_dump_requested   = false;
  static inline std::atomic_bool g_dump_in_progress = false;

  static thread_data *make_tls(std::thread::id _id = std::this_thread::get_id()) {
    std::scoped_lock lock{g_mapping_mutex};
    auto             inserted = g_threads_mapping.emplace(_id, std::make_unique<thread_data>());
    assert(inserted.second);
    return inserted.first->second.get();
  }

  static thread_data &get() {
    thread_local thread_data *s_tls_data = make_tls(std::this_thread::get_id());
    return *s_tls_data;
  }

  static std::vector<complete_event> &my_queue() {
    auto            &tdata = get();
    std::scoped_lock lock_completed(tdata.frames_mutex_);
    return tdata.frames_[g_frame_index];
  }

  static void next_frame() {
    if (g_dump_requested && !g_dump_in_progress.load()) {
      g_dump_in_progress.store(true);
      std::thread{background_dump, collect_frames()}.detach();
    }
    g_dump_requested = false;

    auto prev = g_frame_index.load();
    auto next = (prev + 1) % FRAME_HISTORY;

    std::scoped_lock lock_mapping{g_mapping_mutex};
    for (auto &thread : g_threads_mapping) {
      std::scoped_lock lock_completed(thread.second->frames_mutex_);
      thread.second->frames_[next].clear();
    }
    g_frame_index.store(next);
  }

  static void request_dump() { g_dump_requested = true; }

  static void dump_stackframes(std::ostream &_os) {
    static stacktraces_registry       s_registry;
    static stacktrace_entries_cache   s_entries;
    static std::unordered_set<size_t> s_done;
    static std::stringstream          s_json_cache;

    auto before = clock_f32::now();
    {
      std::scoped_lock mapping_lock{g_mapping_mutex};
      for (auto &mapping : g_threads_mapping) {
        std::scoped_lock cache_lock(mapping.second->registry_mutex_);
        s_registry.insert(mapping.second->registry_.begin(), mapping.second->registry_.end());
      }
    }
    auto copied = clock_f32::now();

    for (const auto &strace : s_registry) {
      if (!s_done.emplace(strace.first).second)
        continue;
      const auto &vec  = strace.second.as_vector();
      const auto  size = (int)vec.size();
      for (int i = size - 1; i >= 0; --i) {
        s_json_cache << ",\n\"" << strace.first;
        if (i != 0)
          s_json_cache << '_' << i;

        auto entry   = vec[i];
        auto itcache = s_entries.find(entry.address());
        if (itcache == s_entries.end()) {
          auto result = s_entries.emplace(entry.address(),
                                          resolved_entry{entry.name(), entry.source_file(), entry.source_line()});
          itcache     = result.first;
        }

        s_json_cache << "\":{\"name\":\"";
        escape_json(s_json_cache, itcache->second.name_);
        s_json_cache << " at ";
        escape_json(s_json_cache, itcache->second.source_file_);
        s_json_cache << ':' << itcache->second.line_;
        if (i != (size - 1))
          s_json_cache << "\",\"parent\":\"" << strace.first << '_' << (i + 1);
        s_json_cache << "\"}";
      }
    }
    _os << s_json_cache.view();
#  ifdef HUT_PROFILING_PROFILE
    std::cout << "[hut] dumped stackframes in " << (clock_f32::now() - before) << " (locked " << (copied - before)
              << ")" << std::endl;
#  endif  // HUT_PROFILING_PROFILE
  }

  static cpu_frames collect_frames() {
    auto             before = clock_f32::now();
    cpu_frames       result;
    std::scoped_lock lock{g_mapping_mutex};
    for (auto &thread : g_threads_mapping) {
      std::scoped_lock lock_completed(thread.second->frames_mutex_);
      for (auto &frame : thread.second->frames_) {
        result.emplace_back(std::move(frame));
      }
    }
#  ifdef HUT_PROFILING_PROFILE
    std::cout << "[hut] collected profiling frames in " << (clock_f32::now() - before) << std::endl;
#  endif  // HUT_PROFILING_PROFILE
    return result;
  }

  static std::filesystem::path unique_profiline_path() {
    constexpr std::string_view PREFIX = "profiling-";

    static bool s_cleaned = false;
    if (!s_cleaned) {
      for (const auto &entry : std::filesystem::directory_iterator(".")) {
        auto             stem_str = entry.path().stem().string();
        std::string_view stem     = stem_str;
        if (stem.starts_with(PREFIX)) {
          std::filesystem::remove(entry);
        }
      }
      s_cleaned = true;
    }

    static unsigned   s_last_index = 0;
    std::stringstream filename;
    filename << PREFIX << (s_last_index++ % 10) << ".json";
    return filename.str();
  }

  static void background_dump(cpu_frames &&_frames) {
    auto before   = clock_f32::now();
    auto filename = unique_profiline_path();
    std::cout << "[hut] starting background profiling dump to: " << filename << "..." << std::endl;
    std::ofstream os{filename};

    os << std::fixed << "{\"traceEvents\":[{}";
    for (auto &frame : _frames) {
      for (auto &event : frame) {
        os << "," << std::endl;
        event.dump(os);
      }
    }

#  ifdef HUT_PROFILING_PROFILE
    std::cout << "[hut] dumped frames in " << (clock_f32::now() - before) << std::endl;
#  endif  // HUT_PROFILING_PROFILE

    os << "],\"stackFrames\":{\"dummy\":{\"name\":\"dummy\"}";
    dump_stackframes(os);
    os << "}}" << std::defaultfloat << std::endl;

    std::cout << "[hut] wrote profiling to: " << filename << " in " << (clock_f32::now() - before) << std::endl;
    g_dump_in_progress.store(false);
  }
};

inline u32 register_stacktrace(stacktrace &&_stacktrace) {
  u32 result = rehash<u32>(boost::stacktrace::hash_value(_stacktrace));
  {
    auto            &tdata = threads_data::get();
    std::scoped_lock lock_cache(tdata.registry_mutex_);
    tdata.registry_.try_emplace(result, std::move(_stacktrace));
  }
  return result;
}

template<typename TEventType, typename... TEventArgs>
struct complete_event_scope {
  std::vector<TEventType>  &buffer_;
  std::tuple<TEventArgs...> args_;
  clock_f32::time_point     start_timestamp_;
  stacktrace_u32            stacktrace_;
  threadid_u16              threadid_;
  dispatcher_u16            dispatcher_;

  complete_event_scope(std::vector<TEventType> &_buffer, std::tuple<TEventArgs...> &&_event_args,
                       clock_f32::time_point _start, stacktrace_u32 _stacktrace, threadid_u16 _threadid,
                       dispatcher_u16 _dispatcher)
      : buffer_{_buffer}
      , args_{std::move(_event_args)}
      , start_timestamp_{_start}
      , stacktrace_{_stacktrace}
      , threadid_{_threadid}
      , dispatcher_{_dispatcher} {}

  ~complete_event_scope() {
    auto duration = clock_f32::now() - start_timestamp_;
    buffer_.emplace_back(std::move(args_), start_timestamp_, duration, stacktrace_, threadid_, dispatcher_);
  }
};

template<fixed_string TFormat, fixed_string_array TArgNames, profiling_category TProfileCat, typename TEventType,
         typename... TEventArgs>
auto make_complete_event_scope(std::vector<TEventType> &_buffer, std::tuple<TEventArgs...> &&_event_args) {
  static auto s_slot = dispatcher_repository<dispatcher_u16>::slot(
      dispatcher_impl<TFormat, TArgNames, type::COMPLETE, TProfileCat, TEventType, TEventArgs...>);

  auto before = clock_f32::now();
  auto trace  = stacktrace{1, STACKTRACE_DEPTH};
  auto traced = clock_f32::now();
  auto hash   = register_stacktrace(std::move(trace));
  auto hashed = clock_f32::now();

#  ifdef HUT_PROFILING_PROFILE
  if (hashed - before > 100ms) {
    std::cout << "[hut] collecting a single stacktrace for an event in " << (traced - before) << " and hashed in "
              << (hashed - traced) << std::endl;
  }
#  endif  // HUT_PROFILING_PROFILE

  return complete_event_scope<TEventType, TEventArgs...>{_buffer, std::move(_event_args),      hashed,
                                                         hash,    this_thread<threadid_u16>(), s_slot};
}

#  define HUT_PROFILE_TRANSFORM_STRINGIFY(MR, MData, MElement) BOOST_PP_STRINGIZE(MElement)

#  define HUT_PROFILE_MAP_DETAIL(MTransform, MSeq) BOOST_PP_SEQ_TO_TUPLE(BOOST_PP_SEQ_TRANSFORM(MTransform, _, MSeq))
#  define HUT_PROFILE_MAP(MTransform, ...)         HUT_PROFILE_MAP_DETAIL(MTransform, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#  define HUT_PROFILE_UNIQUE_SYMBOL(MPrefix) BOOST_PP_CAT(MPrefix##_scope, __LINE__)

#  define HUT_PROFILE_SCOPE_IMPL_DATANAMED(MCat, MFormat, MArgNames, MArgNamesVar, MDataVar, ...)                      \
    auto HUT_PROFILE_UNIQUE_SYMBOL(_scope) = ::hut::profiling::make_complete_event_scope<                              \
        MFormat, ::hut::fixed_string_array{BOOST_PP_TUPLE_ENUM(MArgNames)}, MCat>(                                     \
        ::hut::profiling::threads_data::my_queue(), std::make_tuple(__VA_ARGS__));

#  define HUT_PROFILE_SCOPE_IMPL(MCat, MFormat, MArgNames, ...)                                                        \
    HUT_PROFILE_SCOPE_IMPL_DATANAMED(MCat, MFormat, MArgNames, HUT_PROFILE_UNIQUE_SYMBOL(_names),                      \
                                     HUT_PROFILE_UNIQUE_SYMBOL(_data), __VA_ARGS__)

#  define HUT_PROFILE_SCOPE(MCat, MFormat, ...)                                                                        \
    HUT_PROFILE_SCOPE_IMPL(MCat, MFormat, HUT_PROFILE_MAP(HUT_PROFILE_TRANSFORM_STRINGIFY, __VA_ARGS__), __VA_ARGS__)
#  define HUT_PROFILE_SCOPE_NAMED(MCat, MFormat, MArgNames, ...)                                                       \
    HUT_PROFILE_SCOPE_IMPL(MCat, MFormat, MArgNames, __VA_ARGS__)

#  define HUT_PROFILE_FUN(MCat, ...) HUT_PROFILE_SCOPE(MCat, __FUNCTION__, __VA_ARGS__)
#  define HUT_PROFILE_FUN_NAMED(MCat, MArgNames, ...)                                                                  \
    HUT_PROFILE_SCOPE_NAMED(MCat, __FUNCTION__, MArgNames, __VA_ARGS__)

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

#  define HUT_PROFILE_FUN(MCat, ...)
#  define HUT_PROFILE_FUN_NAMED(MCat, MArgNames, ...)

#  define HUT_PROFILE_EVENT(MTarget, MEvent, ...)                                      MTarget->MEvent.fire(__VA_ARGS__)
#  define HUT_PROFILE_EVENT_NAMED(MTarget, MEvent, MArgNames, ...)                     MTarget->MEvent.fire(__VA_ARGS__)
#  define HUT_PROFILE_EVENT_NAMED_ALIASED(MTarget, MEvent, MArgNames, MArgValues, ...) MTarget->MEvent.fire(__VA_ARGS__)

#  define HUT_PVK(MName, ...) MName(__VA_ARGS__)

#  define HUT_PVK_NAMED_ALIASED(MName, MArgNames, MArgValues, ...) MName(__VA_ARGS__)

#endif  // HUT_ENABLE_PROFILING

}  // namespace hut::profiling
