#include "BackgroundImporter.h"

#include "PolyendImport.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <utility>

namespace trakr::io {

BackgroundImporter::BackgroundImporter()
    : thread_([this] { workerThread(); }) {}

BackgroundImporter::~BackgroundImporter() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

bool BackgroundImporter::enqueue(Job job) {
    if (job.path.empty()) return false;

    // Reject files the loader cannot decode up front so we never spin the
    // worker on a useless read.
    const std::string ext = [&]() {
        const std::size_t dot = job.path.rfind('.');
        if (dot == std::string::npos) return std::string();
        std::string e = job.path.substr(dot);
        std::transform(e.begin(), e.end(), e.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return e;
    }();
    const bool known = ext == ".pti" || ext == ".wav" || ext == ".flac" ||
                       ext == ".aif" || ext == ".aiff" || ext == ".ogg" || ext == ".mp3";
    if (!known) return false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_) return false;
        jobs_.push_back(std::move(job));
    }
    cv_.notify_one();
    return true;
}

bool BackgroundImporter::poll(Result& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (results_.empty()) return false;
    out = std::move(results_.front());
    results_.pop_front();
    return true;
}

size_t BackgroundImporter::pending() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return jobs_.size() + results_.size();
}

void BackgroundImporter::workerThread() {
    for (;;) {
        Job job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !jobs_.empty(); });
            if (stop_ && jobs_.empty()) return;
            job = std::move(jobs_.front());
            jobs_.pop_front();
        }

        Result result;
        result.slot = job.slot;
        const std::size_t dot = job.path.rfind('.');
        const std::string ext = [&]() {
            if (dot == std::string::npos) return std::string();
            std::string e = job.path.substr(dot);
            std::transform(e.begin(), e.end(), e.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            return e;
        }();

        bool loaded = false;
        if (ext == ".pti") {
            Instrument inst;
            if (loadPtiInstrument(job.path, inst)) {
                result.inst = std::move(inst);
                result.kind = "pti";
                loaded = true;
            }
        } else {
            auto samp = std::make_shared<SampleData>();
            if (loadAudioFile(job.path, *samp)) {
                result.inst.sample = std::move(samp);
                result.inst.name = job.path.substr(0, dot);
                const std::size_t slash = result.inst.name.find_last_of("/\\");
                if (slash != std::string::npos) result.inst.name = result.inst.name.substr(slash + 1);
                result.kind = ext.empty() ? "sample" : ext.substr(1);
                loaded = true;
            }
        }
        result.ok = loaded;
        if (loaded) result.name = result.inst.name;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            results_.push_back(std::move(result));
        }
    }
}

}  // namespace trakr::io