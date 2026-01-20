#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <byte>

#include "engine/base/Error.hpp"
#include "engine/base/Result.hpp"
#include "engine/base/Span.hpp"
#include "engine/io/IoError.hpp"
#include "engine/io/stream/IStream.hpp"
#include "engine/io/stream/Seek.hpp"

namespace Engine::IO::Stream {

    using IoError  = Base::Error<Engine::IO::IoErrorCode>;

    template<class T>
    using IoResult = Base::Result<T, IoError>;

    using IoResultVoid = Base::Result<void, IoError>;

    class SpanStream final : public IStream {
    public:
        // ReadOnly
        explicit SpanStream(Base::Span<const std::byte> ro)
            : ro_(ro.data()), rw_(nullptr), size_(static_cast<std::uint64_t>(ro.size())),
              writable_(false) {}

        // ReadWrite
        explicit SpanStream(Base::Span<std::byte> rw)
            : ro_(rw.data()), rw_(rw.data()), size_(static_cast<std::uint64_t>(rw.size())),
              writable_(true) {}

        Stream::StreamCaps Caps() const noexcept override {
            StreamCaps c;
            c.readable = true;
            c.writable = writable_;
            c.seekable = true;
            return c;
        }

        bool IsOpen() const noexcept override { return open_; }
        bool IsEof() const noexcept override { return eof_; }

        IoResult<std::size_t> Read(void* dst, std::size_t bytes) override {
            if (!open_) {
                return IoResult<std::size_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::ReadFailed,
                    "SpanStream: read on closed stream"));
            }
            if (bytes == 0) return IoResult<std::size_t>::Ok(0);

            if (pos_ >= size_) {
                eof_ = true;
                return IoResult<std::size_t>::Ok(0);
            }

            const std::uint64_t remain = size_ - pos_;
            const std::size_t n = static_cast<std::size_t>(
                (std::min<std::uint64_t>)(remain, static_cast<std::uint64_t>(bytes))
            );

            std::memcpy(dst, ro_ + pos_, n);
            pos_ += static_cast<std::uint64_t>(n);
            eof_ = (pos_ >= size_);
            return IoResult<std::size_t>::Ok(n);
        }

        IoResult<std::size_t> Write(const void* src, std::size_t bytes) override {
            if (!open_) {
                return IoResult<std::size_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::WriteFailed,
                    "SpanStream: write on closed stream"));
            }
            if (!writable_) {
                return IoResult<std::size_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::NotSupported,
                    "SpanStream: write not supported (read-only)"));
            }
            if (bytes == 0) return IoResult<std::size_t>::Ok(0);

            const std::uint64_t need = pos_ + static_cast<std::uint64_t>(bytes);
            if (need > size_) {
                return IoResult<std::size_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::WriteFailed,
                    "SpanStream: write beyond end",
                    "need=" + std::to_string(need) + " size=" + std::to_string(size_)));
            }

            std::memcpy(rw_ + pos_, src, bytes);
            pos_ += static_cast<std::uint64_t>(bytes);
            eof_ = false;
            return IoResult<std::size_t>::Ok(bytes);
        }

        IoResult<std::uint64_t> Tell() const override {
            if (!open_) {
                return IoResult<std::uint64_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::SeekFailed,
                    "SpanStream: tell on closed stream"));
            }
            return IoResult<std::uint64_t>::Ok(pos_);
        }

        IoResult<std::uint64_t> Seek(std::int64_t offset, SeekWhence whence) override {
            if (!open_) {
                return IoResult<std::uint64_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::SeekFailed,
                    "SpanStream: seek on closed stream"));
            }

            const std::int64_t cur = static_cast<std::int64_t>(pos_);
            const std::int64_t end = static_cast<std::int64_t>(size_);
            std::int64_t target = 0;

            switch (whence) {
            case SeekWhence::Begin:   target = offset; break;
            case SeekWhence::Current: target = cur + offset; break;
            case SeekWhence::End:     target = end + offset; break;
            default:                  target = offset; break;
            }

            if (target < 0 || target > end) {
                return IoResult<std::uint64_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::SeekFailed,
                    "SpanStream: seek out of range",
                    "target=" + std::to_string(target) + " size=" + std::to_string(end)));
            }

            pos_ = static_cast<std::uint64_t>(target);
            eof_ = false;
            return IoResult<std::uint64_t>::Ok(pos_);
        }

        IoResult<std::uint64_t> Size() const override {
            if (!open_) {
                return IoResult<std::uint64_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::NotSupported,
                    "SpanStream: size on closed stream"));
            }
            return IoResult<std::uint64_t>::Ok(size_);
        }

        IoResultVoid Flush() override {
            if (!open_) {
                return IoResultVoid::Err(IoError::Make(
                    Engine::IO::IoErrorCode::NotSupported,
                    "SpanStream: flush on closed stream"));
            }
            return IoResultVoid::Ok(); // no-op
        }

        IoResultVoid Close() override {
            open_ = false;
            eof_ = false;
            return IoResultVoid::Ok();
        }

    private:
        const std::byte* ro_ = nullptr;
        std::byte* rw_ = nullptr;
        std::uint64_t size_ = 0;
        std::uint64_t pos_  = 0;
        bool writable_ = false;
        bool open_ = true;
        bool eof_ = false;
    };

} // namespace Engine::IO::Stream
