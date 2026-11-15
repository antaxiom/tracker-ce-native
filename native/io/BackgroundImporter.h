#pragma once

// Loads sample / instrument files off the UI thread so a large audio file never
// blocks the editor. The worker only runs the pure I/O + decode (trakr::io
// loaders) into a freshly built Instrument; the caller stays on the main thread
// and applies the completed result to the model + engine via poll().

#include "Song.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace trakr::io {

class BackgroundImporter {
public:
    BackgroundImporter();
    ~BackgroundImporter();  // joins the worker thread
    BackgroundImporter(const BackgroundImporter&) = delete;
    BackgroundImporter& operator=(const BackgroundImporter&) = delete;

    struct Job {
        std::string path;  // absolute path to load (.wav/.flac/.aif/.pti/...)
        int slot{0};       // destination instrument slot
    };
    struct Result {
        bool ok{false};
        int slot{0};
        std::string kind;  // "pti" or the audio extension (empty on failure)
        std::string name;  // file stem (sample) / parsed name (pti)
        Instrument inst;   // valid when ok
    };

    // Queue a file load. Returns false when the loader is stopped or the path
    // has no known audio extension (so the UI can reject it immediately).
    bool enqueue(Job job);

    // Non-blocking: pop one completed result if ready. Returns true on success.
    bool poll(Result& out);

    // Number of queued + in-flight jobs (not yet produce a result).
    size_t pending() const;

private:
    void workerThread();

    std::thread thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<Job> jobs_;
    std::deque<Result> results_;
    bool stop_{false};
};

}  // namespace trakr::io