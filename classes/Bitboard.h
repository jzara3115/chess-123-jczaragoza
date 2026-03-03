#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#endif
#include <iostream>
#include <stdint.h>

enum ChessPiece;

class BitboardElement {
  public:
    // Constructors
    BitboardElement()
        : _data(0) { }
    BitboardElement(uint64_t data)
        : _data(data) { }

    // Getters and Setters
    uint64_t getData() const { return _data; }
    void setData(uint64_t data) { _data = data; }

    // loop through each bit in the element and perform an operation on it.
    template <typename Func>
    void forEachBit(Func func) const {
        if (_data != 0) {
            uint64_t tempData = _data;
            while (tempData) {
                int index = bitScanForward(tempData);
                func(index);
                tempData &= tempData - 1;
            }
        }
    }

    BitboardElement& operator|=(const uint64_t other) {
        _data |= other;
        return *this;
    }

    void printBitboard() {
        std::cout << "\n  a b c d e f g h\n";
        for (int rank = 7; rank >= 0; rank--) {
            std::cout << (rank + 1) << " ";
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;
                if (_data & (1ULL << square)) {
                    std::cout << "X ";
                } else {
                    std::cout << ". ";
                }
            }
            std::cout << (rank + 1) << "\n";
            std::cout << std::flush;
        }
        std::cout << "  a b c d e f g h\n";
        std::cout << std::flush;
    }

private:
    uint64_t    _data;

    inline int bitScanForward(uint64_t bb) const {
#if defined(_MSC_VER) && !defined(__clang__)
        unsigned long index;
        _BitScanForward64(&index, bb);
        return index;
#else
        return __builtin_ffsll(bb) - 1;
#endif
    };
};

// BitMove structure for storing moves
struct BitMove {
    uint8_t from;
    uint8_t to;
    uint8_t piece;
    
    BitMove(int from, int to, ChessPiece piece)
        : from(from), to(to), piece(piece) { }
        
    BitMove() : from(0), to(0), piece(0) { }
    
    bool operator==(const BitMove& other) const {
        return from == other.from && 
               to == other.to && 
               piece == other.piece;
    }
};

#define SET_BIT(bb, sq) ((bb) |= (1ULL << (sq)))
#define CLEAR_BIT(bb, sq) ((bb) &= ~(1ULL << (sq)))
#define GET_BIT(bb, sq) ((bb) & (1ULL << (sq)))
#define SQUARE(rank, file) ((rank) * 8 + (file))

// Directional shift macros
#define NORTH(bb) ((bb) << 8)
#define SOUTH(bb) ((bb) >> 8)
#define EAST(bb) (((bb) & ~0x8080808080808080ULL) << 1)
#define WEST(bb) (((bb) & ~0x0101010101010101ULL) >> 1)
#define NORTH_EAST(bb) (((bb) & ~0x8080808080808080ULL) << 9)
#define NORTH_WEST(bb) (((bb) & ~0x0101010101010101ULL) << 7)
#define SOUTH_EAST(bb) (((bb) & ~0x8080808080808080ULL) >> 7)
#define SOUTH_WEST(bb) (((bb) & ~0x0101010101010101ULL) >> 9)

#define WHITE_PAWN_ATTACKS(pawns) (NORTH_EAST(pawns) | NORTH_WEST(pawns))
#define BLACK_PAWN_ATTACKS(pawns) (SOUTH_EAST(pawns) | SOUTH_WEST(pawns))

// PRE CALCULATED KNIGHT ATTACKS
const uint64_t KnightAttacks[64] = {
  0x20400ULL, 0x50800ULL, 0xa1100ULL, 0x142200ULL,
  0x284400ULL, 0x508800ULL, 0xa01000ULL, 0x402000ULL,
  0x2040004ULL, 0x5080008ULL, 0xa110011ULL, 0x14220022ULL,
  0x28440044ULL, 0x50880088ULL, 0xa0100010ULL, 0x40200020ULL,
  0x204000402ULL, 0x508000805ULL, 0xa1100110aULL, 0x1422002214ULL,
  0x2844004428ULL, 0x5088008850ULL, 0xa0100010a0ULL, 0x4020002040ULL,
  0x20400040200ULL, 0x50800080500ULL, 0xa1100110a00ULL, 0x142200221400ULL,
  0x284400442800ULL, 0x508800885000ULL, 0xa0100010a000ULL, 0x402000204000ULL,
  0x2040004020000ULL, 0x5080008050000ULL, 0xa1100110a0000ULL, 0x14220022140000ULL,
  0x28440044280000ULL, 0x50880088500000ULL, 0xa0100010a00000ULL, 0x40200020400000ULL,
  0x204000402000000ULL, 0x508000805000000ULL, 0xa1100110a000000ULL, 0x1422002214000000ULL,
  0x2844004428000000ULL, 0x5088008850000000ULL, 0xa0100010a0000000ULL, 0x4020002040000000ULL,
  0x400040200000000ULL, 0x800080500000000ULL, 0x1100110a00000000ULL, 0x2200221400000000ULL,
  0x4400442800000000ULL, 0x8800885000000000ULL, 0x100010a000000000ULL, 0x2000204000000000ULL,
  0x4020000000000ULL, 0x8050000000000ULL, 0x110a0000000000ULL, 0x22140000000000ULL,
  0x44280000000000ULL, 0x88500000000000ULL, 0x10a00000000000ULL, 0x20400000000000ULL,
};

// PRE CALCULATED KING ATTACKS
const uint64_t KingAttacks[64] = {
  0x302ULL, 0x705ULL, 0xe0aULL, 0x1c14ULL,
  0x3828ULL, 0x7050ULL, 0xe0a0ULL, 0xc040ULL,
  0x30203ULL, 0x70507ULL, 0xe0a0eULL, 0x1c141cULL,
  0x382838ULL, 0x705070ULL, 0xe0a0e0ULL, 0xc040c0ULL,
  0x3020300ULL, 0x7050700ULL, 0xe0a0e00ULL, 0x1c141c00ULL,
  0x38283800ULL, 0x70507000ULL, 0xe0a0e000ULL, 0xc040c000ULL,
  0x302030000ULL, 0x705070000ULL, 0xe0a0e0000ULL, 0x1c141c0000ULL,
  0x3828380000ULL, 0x7050700000ULL, 0xe0a0e00000ULL, 0xc040c00000ULL,
  0x30203000000ULL, 0x70507000000ULL, 0xe0a0e000000ULL, 0x1c141c000000ULL,
  0x382838000000ULL, 0x705070000000ULL, 0xe0a0e0000000ULL, 0xc040c0000000ULL,
  0x3020300000000ULL, 0x7050700000000ULL, 0xe0a0e00000000ULL, 0x1c141c00000000ULL,
  0x38283800000000ULL, 0x70507000000000ULL, 0xe0a0e000000000ULL, 0xc040c000000000ULL,
  0x302030000000000ULL, 0x705070000000000ULL, 0xe0a0e0000000000ULL, 0x1c141c0000000000ULL,
  0x3828380000000000ULL, 0x7050700000000000ULL, 0xe0a0e00000000000ULL, 0xc040c00000000000ULL,
  0x203000000000000ULL, 0x507000000000000ULL, 0xa0e000000000000ULL, 0x141c000000000000ULL,
  0x2838000000000000ULL, 0x5070000000000000ULL, 0xa0e0000000000000ULL, 0x40c0000000000000ULL,
};
