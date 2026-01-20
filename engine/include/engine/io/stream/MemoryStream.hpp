#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include <byte>

#include "engine/base/Error.hpp"
#include "engine/base/Result.hpp"
#include "engine/io/IoError.hpp"
#include "engine/io/stream/IStream.hpp" // StreamCaps, IStream
#include "engine/io/stream/Seek.hpp"    // SeekWhence

namespace Engine::IO::Stream {

    using IoError  = Engine::Base::Error<Engine::IO::IoErrorCode>;
    template<class T>
    using IoResult = Engine::Base::Result<T, IoError>;
    using IoResultVoid = Engine::Base::Result<void, IoError>;

    class MemoryStream final : public IStream {
    public:
        struct Options final {
            bool readable = true;
            bool writable = true;
            bool growable = true; // true: 書き込みで自動拡張
        };

        MemoryStream() : opt_{} {}
        explicit MemoryStream(Options opt) : opt_(opt) {}

        explicit MemoryStream(std::vector<std::byte> data, Options opt = {})
            : opt_(opt), buf_(std::move(data)) {}

        const std::vector<std::byte>& Buffer() const noexcept { return buf_; }
        std::vector<std::byte>& Buffer() noexcept { return buf_; }

        std::uint64_t Position() const noexcept { return pos_; }

        // ----- IStream -----
        StreamCaps Caps() const noexcept override {
            StreamCaps c;
            c.readable = opt_.readable;
            c.writable = opt_.writable;
            c.seekable = true;
            return c;
        }

        bool IsOpen() const noexcept override { return open_; }
        bool IsEof() const noexcept override { return eof_; }

        IoResult<std::size_t> Read(void* dst, std::size_t bytes) override {
            if (!open_) {
                return IoResult<std::size_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::ReadFailed,
                    "MemoryStream: read on closed stream"));
            }
            if (!opt_.readable) {
                return IoResult<std::size_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::NotSupported,
                    "MemoryStream: read not supported"));
            }
            if (bytes == 0) return IoResult<std::size_t>::Ok(0);

            const std::uint64_t size = static_cast<std::uint64_t>(buf_.size());
            if (pos_ >= size) {
                eof_ = true;
                return IoResult<std::size_t>::Ok(0);
            }

            const std::uint64_t remain = size - pos_;
            const std::size_t n = static_cast<std::size_t>(
                (std::min<std::uint64_t>)(remain, static_cast<std::uint64_t>(bytes))
            );

            std::memcpy(dst, buf_.data() + pos_, n);
            pos_ += static_cast<std::uint64_t>(n);
            eof_ = (pos_ >= size);
            return IoResult<std::size_t>::Ok(n);
        }

        IoResult<std::size_t> Write(const void* src, std::size_t bytes) override {
            if (!open_) {
                return IoResult<std::size_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::WriteFailed,
                    "MemoryStream: write on closed stream"));
            }
            if (!opt_.writable) {
                return IoResult<std::size_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::NotSupported,
                    "MemoryStream: write not supported"));
            }
            if (bytes == 0) return IoResult<std::size_t>::Ok(0);

            // 必要サイズ（pos_ は “末尾より先” も許可：growable+writable の場合）
            const std::uint64_t need = pos_ + static_cast<std::uint64_t>(bytes);

            if (need > static_cast<std::uint64_t>(buf_.size())) {
                if (!opt_.growable) {
                    return IoResult<std::size_t>::Err(IoError::Make(
                        Engine::IO::IoErrorCode::WriteFailed,
                        "MemoryStream: no space (not growable)",
                        "need=" + std::to_string(need) + " size=" + std::to_string(buf_.size())));
                }
                buf_.resize(static_cast<std::size_t>(need));
            }

            std::memcpy(buf_.data() + pos_, src, bytes);
            pos_ += static_cast<std::uint64_t>(bytes);
            eof_ = false; // 書き込んだら EOF 状態は解除
            return IoResult<std::size_t>::Ok(bytes);
        }

        IoResult<std::uint64_t> Tell() const override {
            if (!open_) {
                return IoResult<std::uint64_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::SeekFailed,
                    "MemoryStream: tell on closed stream"));
            }
            return IoResult<std::uint64_t>::Ok(pos_);
        }

        IoResult<std::uint64_t> Seek(std::int64_t offset, SeekWhence whence) override {
            if (!open_) {
                return IoResult<std::uint64_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::SeekFailed,
                    "MemoryStream: seek on closed stream"));
            }

            const std::int64_t cur  = static_cast<std::int64_t>(pos_);
            const std::int64_t end  = static_cast<std::int64_t>(buf_.size());
            std::int64_t target = 0;

            switch (whence) {
            case SeekWhence::Begin:   target = offset; break;
            case SeekWhence::Current: target = cur + offset; break;
            case SeekWhence::End:     target = end + offset; break;
            default:                  target = offset; break;
            }

            if (target < 0) {
                return IoResult<std::uint64_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::SeekFailed,
                    "MemoryStream: seek before begin",
                    "target=" + std::to_string(target)));
            }

            // growable+writable の場合は end を超える seek を許可（後で write が拡張する）
            const bool allowBeyondEnd = (opt_.writable && opt_.growable);
            if (!allowBeyondEnd && target > end) {
                return IoResult<std::uint64_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::SeekFailed,
                    "MemoryStream: seek beyond end",
                    "target=" + std::to_string(target) + " end=" + std::to_string(end)));
            }

            pos_ = static_cast<std::uint64_t>(target);
            eof_ = false;
            return IoResult<std::uint64_t>::Ok(pos_);
        }

        IoResult<std::uint64_t> Size() const override {
            if (!open_) {
                return IoResult<std::uint64_t>::Err(IoError::Make(
                    Engine::IO::IoErrorCode::NotSupported,
                    "MemoryStream: size on closed stream"));
            }
            return IoResult<std::uint64_t>::Ok(static_cast<std::uint64_t>(buf_.size()));
        }

        IoResultVoid Flush() override {
            if (!open_) {
                return IoResultVoid::Err(IoError::Make(
                    Engine::IO::IoErrorCode::NotSupported,
                    "MemoryStream: flush on closed stream"));
            }
            // no-op
            return IoResultVoid::Ok();
        }

        IoResultVoid Close() override {
            open_ = false;
            eof_ = false;
            return IoResultVoid::Ok();
        }

    private:
        Options opt_{};
        std::vector<std::byte> buf_;
        std::uint64_t pos_ = 0;
        bool open_ = true;
        bool eof_ = false;
    };

} // namespace Engine::IO::Stream
