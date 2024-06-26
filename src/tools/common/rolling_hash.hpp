#pragma once
//
// Created by anton on 7/20/20.
//

#include "common/hash_utils.hpp"
#include "sequences/sequence.hpp"
#include <deque>
#include "common/logging.hpp"

namespace hashing {
    template<typename T, typename U>
    T pow(T base, U p) {
        if (p == 0)
            return 1;
        T tmp = pow(base, p / 2);
        if (p % 2 == 1)
            return base * tmp * tmp;
        else
            return tmp * tmp;
    }

    class RollingHash {
    private:
        size_t k;
        htype hbase;
        htype kpow;
        htype inv;
    public:

        RollingHash(size_t _k, htype _hbase) : k(_k), hbase(_hbase),
                                               kpow(pow(hbase, k - 1)),
                                               inv(pow(hbase, (htype(1u) << (sizeof(htype) * 8u - 1u)) - 1u)) {
            VERIFY(inv * hbase == htype(1));
        }

        size_t getK() const {
            return k;
        }

        RollingHash extensionHash() const {
            return RollingHash(k + 1, hbase);
        }

        htype hash(const Sequence &seq, size_t pos) const {
            htype hash = 0;
            for (size_t i = pos; i < pos + k; i++) {
                hash = hash * hbase + seq[i];
            }
            return hash;
        }

        htype extendRight(const Sequence &seq, size_t pos, htype hash, unsigned char c) const {
            return hash * hbase + c;
        }

        htype extendLeft(const Sequence &seq, size_t pos, htype hash, unsigned char c) const {
            return hash + c * kpow * hbase;
        }

        htype shiftRight(const Sequence &seq, size_t pos, htype hash, unsigned char c) const {
            return (hash - kpow * seq[pos]) * hbase + c;
        }

        htype shiftLeft(const Sequence &seq, size_t pos, htype hash, unsigned char c) const {
            return (hash - seq[pos + k - 1]) * inv + c * kpow;
        }

        htype next(const Sequence &seq, size_t pos, htype hash) const {
            return shiftRight(seq, pos, hash, seq[pos + k]);
        }

        htype prev(const Sequence &seq, size_t pos, htype hash) const {
            return shiftLeft(seq, pos, hash, seq[pos - 1]);
        }

        bool hasNext(const Sequence &seq, size_t pos) const {
            return pos + k < seq.size();
        }

        bool hasPrev(const Sequence &seq, size_t pos) const {
            return pos > 0;
        }
    };

    class KWH {
    private:
        KWH(const RollingHash &_hasher, const Sequence &_seq, size_t _pos, htype _fhash, htype _rhash) :
                hasher(_hasher), seq(_seq), pos(_pos), fhash(_fhash), rhash(_rhash) {
        }

        htype fhash;
        htype rhash;
        Sequence seq; 
    public:
        const RollingHash &hasher;
        size_t pos;
        inline static size_t accumulatedTime_extendRight = 0;
        inline static int numCalls_extendRight = 0;
        inline static size_t accumulatedTime_extendLeft = 0;
        inline static int numCalls_extendLeft = 0;
        inline static size_t accumulatedTime_next = 0;
        inline static int numCalls_next = 0;
        inline static size_t accumulatedTime_prev = 0;
        inline static int numCalls_prev = 0;
        inline static size_t accumulatedTime_hasNext = 0;
        inline static int numCalls_hasNext = 0;
        inline static size_t accumulatedTime_hasPrev = 0;
        inline static int numCalls_hasPrev = 0;

        KWH(const RollingHash &_hasher, const Sequence &_seq, size_t _pos) :
                hasher(_hasher), seq(_seq), pos(_pos), fhash(_hasher.hash(_seq, _pos)),
                rhash(_hasher.hash(!_seq, _seq.size() - _pos - _hasher.getK())) {
        }

        KWH(const KWH &other) = default;

        Sequence getSeq() const {
            return seq.Subseq(pos, pos + hasher.getK());
        }

        KWH operator!() const {
            return KWH(hasher, !seq, seq.size() - pos - hasher.getK(), rhash, fhash);
        }

        htype hash() const {
            return std::min(fhash, rhash);
        }

        htype fHash() const {
            return fhash;
        }

        htype rHash() const {
            return rhash;
        }

        static void print_times() {
            std::stringstream ss;
            ss << "accumulatedTime_extendRight: " << hashing::KWH::accumulatedTime_extendRight
            << "\nnumCalls_extendRight: " << numCalls_extendRight
            << "\naccumulatedTime_extendLeft: " << hashing::KWH::accumulatedTime_extendLeft
            << "\nnumCalls_extendLeft: " << hashing::KWH::numCalls_extendLeft
            << "\naccumulatedTime_next: " << hashing::KWH::accumulatedTime_next
            << "\nnumCalls_next: " << hashing::KWH::numCalls_next
            << "\naccumulatedTime_prev: " << hashing::KWH::accumulatedTime_prev
            << "\nnumCalls_prev: " << hashing::KWH::numCalls_prev
            << "\naccumulatedTime_hasNext: " << hashing::KWH::accumulatedTime_hasNext
            << "\nnumCalls_hasNext: " << hashing::KWH::numCalls_hasNext
            << "\naccumulatedTime_hasPrev: " << hashing::KWH::accumulatedTime_hasPrev
            << "\nnumCalls_hasPrev: " << hashing::KWH::numCalls_hasPrev;
            std::cout << ss.str() << endl;
        }

        htype extendRight(unsigned char c) const {  // profile this
            logging::TimeSpace t;
            htype result = std::min(hasher.extendRight(seq, pos, fhash, c),
                            hasher.extendLeft(!seq, seq.size() - pos - hasher.getK(), rhash, c ^ 3u));
            accumulatedTime_extendRight += t.get_nanoseconds();
            numCalls_extendRight++;
            std::cout << "numCalls_extendRight: " << numCalls_extendRight << std::endl;
            std::cout << "accumulatedTime_extendRight: " << accumulatedTime_extendRight << std::endl;
            return result;
        }

        htype extendLeft(unsigned char c) const { // profile this
            logging::TimeSpace t;
            htype result = std::min(hasher.extendLeft(seq, pos, fhash, c),
                            hasher.extendRight(!seq, seq.size() - pos - hasher.getK(), rhash, c ^ 3u));
            accumulatedTime_extendLeft += t.get_nanoseconds();
            numCalls_extendLeft++;
            return result;
        }

        KWH next() const { // profile this
            logging::TimeSpace t;
            KWH result = {hasher, seq, pos + 1, hasher.next(seq, pos, fhash),
                    hasher.prev(!seq, seq.size() - pos - hasher.getK(), rhash)};
            accumulatedTime_next += t.get_nanoseconds();
            numCalls_next++;
            return result;
        }

        KWH prev() const { // profile this
            logging::TimeSpace t;
            KWH result = {hasher, seq, pos - 1, hasher.prev(seq, pos, fhash),
                    hasher.next(!seq, seq.size() - pos - hasher.getK(), rhash)};
            accumulatedTime_prev += t.get_nanoseconds();
            numCalls_prev++;
            return result;
        }

        bool hasNext() const { // profile this
            logging::TimeSpace t;
            bool result = hasher.hasNext(seq, pos);
            accumulatedTime_hasNext += t.get_nanoseconds();
            numCalls_hasNext++;
            return result;
        }

        bool hasPrev() const { // profile this
            logging::TimeSpace t;
            bool result = hasher.hasPrev(seq, pos);
            accumulatedTime_hasPrev += t.get_nanoseconds();
            numCalls_hasPrev++;
            return result;
        }

        KWH &operator=(const KWH &other) {
            if (this == &other)
                return *this;
            seq = other.seq;
            pos = other.pos;
            fhash = other.fhash;
            rhash = other.rhash;
            return *this;
        }

        bool isCanonical() const {
            return fhash < rhash;
        }
    };


    class MinQueue {
        std::deque<KWH> q;
    public:
        MinQueue() = default;

        void push(const KWH &kwh) {
            while (!q.empty() && q.back().hash() > kwh.hash()) {
                q.pop_back();
            }
            q.push_back(kwh);
        }

        void pop(size_t pos) {
            if (!q.empty() && q.front().pos < pos) {
                q.pop_front();
            }
        }

        bool empty() const {
            return q.empty();
        }

        KWH get() const {
            return q.front();
        }

        size_t size() const {
            return q.size();
        }
    };

    class MinimizerCalculator {
    private:
        const Sequence seq;
        const size_t w;
        KWH kwh;
        size_t pos;
        MinQueue queue;
    public:
        MinimizerCalculator(const Sequence &_seq, const RollingHash &_hasher, size_t _w) :
                seq(_seq), w(_w), kwh(_hasher, seq, 0), pos(-1) {
            VERIFY(w >= 2); //This code does not work for w = 1
            VERIFY(seq.size() >= _hasher.getK() + w - 1)
            queue.push(kwh);
            for (size_t i = 1; i < w; i++) {
                kwh = kwh.next();
                queue.push(kwh);
            }
        }

        KWH next() {
            pos += 1;
            queue.pop(pos);
            kwh = kwh.next();
            queue.push(kwh);
            return queue.get();
        }

        bool hasNext() const {
            return kwh.hasNext();
        }

        std::vector<htype> minimizerHashs() {
            std::vector<htype> res;
            res.push_back(queue.get().hash());
            while (hasNext()) {
                htype val = next().hash();
                if (val != res.back()) {
                    res.push_back(val);
                }
            }
            return std::move(res);
        }

        std::vector<KWH> minimizers() {
            std::vector<KWH> res;
            res.push_back(next());
            while (hasNext()) {
                KWH val = next();
                if (val.pos != res.back().pos) {
                    res.push_back(val);
                }
            }
            return std::move(res);
        }
    };
}