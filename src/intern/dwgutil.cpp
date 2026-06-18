/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <sstream>
#include <cstring>
#include "drw_dbg.h"
#include "dwgutil.h"
#include "rscodec.h"
#include "../libdwgr.h"

// Section sentinel byte sequences. Values verified against libreDWG
// common.c:100-177 and the ODA Open Design Specification v5.4.1.
// Each pair of BEGIN/END is the byte-wise XOR-0xFF of the other.
namespace dwgSentinels {
    const std::uint8_t FILE_HEADER_END[16] = {
        0x95, 0xA0, 0x4E, 0x28, 0x99, 0x82, 0x1A, 0xE5,
        0x5E, 0x41, 0xE0, 0x5F, 0x9D, 0x3A, 0x4D, 0x00
    };
    const std::uint8_t HEADER_BEGIN[16] = {
        0xCF, 0x7B, 0x1F, 0x23, 0xFD, 0xDE, 0x38, 0xA9,
        0x5F, 0x7C, 0x68, 0xB8, 0x4E, 0x6D, 0x33, 0x5F
    };
    const std::uint8_t HEADER_END[16] = {
        0x30, 0x84, 0xE0, 0xDC, 0x02, 0x21, 0xC7, 0x56,
        0xA0, 0x83, 0x97, 0x47, 0xB1, 0x92, 0xCC, 0xA0
    };
    const std::uint8_t CLASSES_BEGIN[16] = {
        0x8D, 0xA1, 0xC4, 0xB8, 0xC4, 0xA9, 0xF8, 0xC5,
        0xC0, 0xDC, 0xF4, 0x5F, 0xE7, 0xCF, 0xB6, 0x8A
    };
    const std::uint8_t CLASSES_END[16] = {
        0x72, 0x5E, 0x3B, 0x47, 0x3B, 0x56, 0x07, 0x3A,
        0x3F, 0x23, 0x0B, 0xA0, 0x18, 0x30, 0x49, 0x75
    };
    const std::uint8_t PREVIEW_BEGIN[16] = {
        0x1F, 0x25, 0x6D, 0x07, 0xD4, 0x36, 0x28, 0x28,
        0x9D, 0x57, 0xCA, 0x3F, 0x9D, 0x44, 0x10, 0x2B
    };
    const std::uint8_t PREVIEW_END[16] = {
        0xE0, 0xDA, 0x92, 0xF8, 0x2B, 0xC9, 0xD7, 0xD7,
        0x62, 0xA8, 0x35, 0xC0, 0x62, 0xBB, 0xEF, 0xD4
    };
    const std::uint8_t SECOND_HEADER_BEGIN[16] = {
        0xD4, 0x7B, 0x21, 0xCE, 0x28, 0x93, 0x9F, 0xBF,
        0x53, 0x24, 0x40, 0x09, 0x12, 0x3C, 0xAA, 0x01
    };
    const std::uint8_t SECOND_HEADER_END[16] = {
        0x2B, 0x84, 0xDE, 0x31, 0xD7, 0x6C, 0x60, 0x40,
        0xAC, 0xDB, 0xBF, 0xF6, 0xED, 0xC3, 0x55, 0xFE
    };
}

namespace dwgVersionString {
    const char R12[6]   = {'A','C','1','0','0','9'};
    const char R13[6]   = {'A','C','1','0','1','2'};
    const char R14[6]   = {'A','C','1','0','1','4'};
    const char R2000[6] = {'A','C','1','0','1','5'};
    const char R2004[6] = {'A','C','1','0','1','8'};
    const char R2007[6] = {'A','C','1','0','2','1'};
    const char R2010[6] = {'A','C','1','0','2','4'};
    const char R2013[6] = {'A','C','1','0','2','7'};
    const char R2018[6] = {'A','C','1','0','3','2'};
}

/** utility function
 * convert a int to string in hex
 **/
namespace DRW {
std::string toHexStr(int n){
#if defined(__APPLE__)
    char buffer[9]= {'\0'};
    snprintf(buffer,9, "%X", n);
    return std::string(buffer);
#else
    std::ostringstream Convert;
    Convert << std::uppercase << std::hex << n;
    return Convert.str();
#endif
}
}

/**
 * @brief dwgRSCodec::decode239I
 * @param in : input data (at least 255*blk bytes)
 * @param out : output data (at least 239*blk bytes)
 * @param blk number of codewords ( 1 cw == 255 bytes)
 */
bool dwgRSCodec::decode239I(unsigned char *in, unsigned char *out, std::uint32_t blk){
    int k=0;
    bool allOk = true;
    unsigned char data[255];
    RScodec rsc(0x96, 8, 8); //(255, 239)
    for (std::uint32_t i=0; i<blk; i++){
        k = i;
        for (int j=0; j<255; j++) {
            data[j] = in[k];
            k +=blk;
        }
        int r = rsc.decode(data);
        if (r<0) {
            DRW_DBG("\nWARNING: dwgRSCodec::decode239I, can't correct all errors");
            allOk = false;
        }
        k = i*239;
        for (int j=0; j<239; j++) {
            out[k++] = data[j];
        }
    }
    return allOk;
}

/**
 * @brief dwgRSCodec::decode251I
 * @param in : input data (at least 255*blk bytes)
 * @param out : output data (at least 251*blk bytes)
 * @param blk number of codewords ( 1 cw == 255 bytes)
 */
bool dwgRSCodec::decode251I(unsigned char *in, unsigned char *out, std::uint32_t blk){
    int k=0;
    bool allOk = true;
    unsigned char data[255];
    RScodec rsc(0xB8, 8, 2); //(255, 251)
    for (std::uint32_t i=0; i<blk; i++){
        k = i;
        for (int j=0; j<255; j++) {
            data[j] = in[k];
            k +=blk;
        }
        int r = rsc.decode(data);
        if (r<0) {
            DRW_DBG("\nWARNING: dwgRSCodec::decode251I, can't correct all errors");
            allOk = false;
        }
        k = i*251;
        for (int j=0; j<251; j++) {
            out[k++] = data[j];
        }
    }
    return allOk;
}

std::uint32_t dwgCompressor::twoByteOffset(std::uint64_t *ll){
    std::uint32_t cont = 0;
    std::uint8_t fb = compressedByte();
    cont = (fb >> 2) | (compressedByte() << 6);
    *ll = (fb & 0x03);
    return cont;
}

std::uint32_t dwgCompressor::longCompressionOffset(){
    std::uint32_t cont = 0;
    std::uint8_t ll = compressedByte();
    while (ll == 0x00 && compressedGood) {
        cont += 0xFF;
        ll = compressedByte();
    }
    cont += ll;
    return cont;
}

std::uint32_t dwgCompressor::long20CompressionOffset(){
//    std::uint32_t cont = 0;
    std::uint32_t cont = 0x0F;
    std::uint8_t ll = compressedByte();
    while (ll == 0x00 && compressedGood){
//        cont += 0xFF;
        ll = compressedByte();
    }
    cont += ll;
    return cont;
}

std::uint32_t dwgCompressor::litLength18(){
    std::uint32_t cont = 0;
    std::uint8_t ll = compressedByte();
    //no literal length, this byte is next opCode
    if (ll > 0x0F) {
        --compressedPos;
        return 0;
    }

    if (ll == 0x00) {
        cont = 0x0F;
        ll = compressedByte();
        while (ll == 0x00 && compressedGood) {//repeat until ll != 0x00
            cont += 0xFF;
            ll = compressedByte();
        }
    }

    return cont + ll + 3;
}

bool dwgCompressor::decompress18(std::uint8_t *cbuf, std::uint8_t *dbuf, std::uint64_t csize, std::uint64_t dsize){
    compressedBuffer = cbuf;
    decompBuffer = dbuf;
    compressedSize = csize;
    decompSize = dsize;
    compressedPos = 0;
    decompPos = 0;
    compressedGood = true;
    decompGood = true;

    DRW_DBG("dwgCompressor::decompress, last 2 bytes: ");
    DRW_DBGH(compressedBuffer[compressedSize - 2]);DRW_DBG(" ");DRW_DBGH(compressedBuffer[compressedSize - 1]);DRW_DBG("\n");

    std::uint64_t compBytes {0};
    std::uint32_t compOffset {0};
    std::uint64_t litCount {litLength18()};

    //copy first literal length
    for (std::uint32_t i = 0; i < litCount && buffersGood(); ++i) {
        decompSet( compressedByte());
    }

    while (buffersGood()) {
        std::uint8_t oc = compressedByte(); //next opcode
        if (oc == 0x10){
            compBytes = longCompressionOffset()+ 9;
            compOffset = twoByteOffset(&litCount) + 0x3FFF;
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc > 0x11 && oc< 0x20){
            compBytes = (oc & 0x0F) + 2;
            compOffset = twoByteOffset(&litCount) + 0x3FFF;
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc == 0x20){
            compBytes = longCompressionOffset() + 0x21;
            compOffset = twoByteOffset(&litCount);
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc > 0x20 && oc< 0x40){
            compBytes = oc - 0x1E;
            compOffset = twoByteOffset(&litCount);
            if (litCount == 0)
                litCount= litLength18();
        } else if ( oc > 0x3F){
            compBytes = ((oc & 0xF0) >> 4) - 1;
            std::uint8_t ll2 = compressedByte();
            compOffset =  (ll2 << 2) | ((oc & 0x0C) >> 2);
            litCount = oc & 0x03;
            if (litCount < 1){
                litCount= litLength18();}
        } else if (oc == 0x11){
            DRW_DBG("dwgCompressor::decompress, end of input stream, Cpos: ");
            DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG("\n");
            return true; //end of input stream
        } else { //ll < 0x10
            DRW_DBG("WARNING dwgCompressor::decompress, failed, illegal char: "); DRW_DBGH(oc);
            DRW_DBG(", Cpos: "); DRW_DBG(compressedPos);
            DRW_DBG(", Dpos: "); DRW_DBG(decompPos); DRW_DBG("\n");
            return false; //fails, not valid
        }

        //copy "compressed data", if size allows
        if (decompSize < decompPos + compBytes) {
            DRW_DBG("WARNING dwgCompressor::decompress18, bad compBytes size, Cpos: ");
            DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG(", need ");DRW_DBG(compBytes);DRW_DBG(", available ");DRW_DBG(decompSize - decompPos);DRW_DBG("\n");
            // only copy what we can fit
            compBytes = decompSize - decompPos;
        }
        {
            std::uint64_t j = decompPos - compOffset - 1;
            // Use memcpy when source and destination don't overlap
            if (j + compBytes <= decompPos) {
                std::memcpy(decompBuffer + decompPos, decompBuffer + j, static_cast<size_t>(compBytes));
                decompPos += compBytes;
            } else {
                for (std::uint64_t i = 0; i < compBytes; i++) {
                    decompBuffer[decompPos++] = decompBuffer[j++];
                }
            }
        }

        //copy "uncompressed data", if size allows
        if (decompSize < decompPos + litCount) {
            DRW_DBG("WARNING dwgCompressor::decompress18, bad litCount size, Cpos: ");
            DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG(", need ");DRW_DBG(litCount);DRW_DBG(", available ");DRW_DBG(decompSize - decompPos);DRW_DBG("\n");
            // only copy what we can fit
            litCount = decompSize - decompPos;
        }
        if (compressedPos + litCount <= compressedSize) {
            std::memcpy(decompBuffer + decompPos, compressedBuffer + compressedPos, static_cast<size_t>(litCount));
            decompPos += litCount;
            compressedPos += litCount;
        } else {
            for (std::uint64_t i=0; i < litCount && buffersGood(); i++) {
                decompSet( compressedByte());
            }
        }
    }

    DRW_DBG("WARNING dwgCompressor::decompress, bad out, Cpos: ");DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG("\n");
    return false;
}

std::uint8_t dwgCompressor::compressedByte(void)
{
    std::uint8_t result {0};

    compressedGood = (compressedPos < compressedSize);
    if (compressedGood) {
        result = compressedBuffer[compressedPos];
        ++compressedPos;
    }

    return result;
}

std::uint8_t dwgCompressor::compressedByte(const std::uint64_t index)
{
    if (index < compressedSize) {
        return compressedBuffer[index];
    }

    return 0;
}

std::uint32_t dwgCompressor::compressedHiByte(void)
{
    return static_cast<std::uint32_t>(compressedByte()) << 8;
}

bool dwgCompressor::compressedInc(const std::int32_t inc /*= 1*/)
{
    compressedPos += inc;
    compressedGood = (compressedPos <= compressedSize);

    return compressedGood;
}

std::uint8_t dwgCompressor::decompByte(const std::uint64_t index)
{
    if (index < decompSize) {
        return decompBuffer[index];
    }

    return 0;
}

void dwgCompressor::decompSet(const std::uint8_t value)
{
    decompGood = (decompPos < decompSize);
    if (decompGood) {
        decompBuffer[decompPos] = value;
        ++decompPos;
    }
}

bool dwgCompressor::buffersGood(void)
{
    return compressedGood && decompGood;
}

void dwgCompressor::decrypt18Hdr(std::uint8_t *buf, std::uint64_t size, std::uint64_t offset){
    std::uint64_t max = size / 4;
    std::uint32_t secMask = 0x4164536b ^ static_cast<std::uint32_t>(offset);
    std::uint32_t* pHdr = reinterpret_cast<std::uint32_t*>(buf);
    for (std::uint64_t j = 0; j < max; j++)
        *pHdr++ ^= secMask;
}

/*void dwgCompressor::decrypt18Data(std::uint8_t *buf, std::uint32_t size, std::uint32_t offset){
    std::uint8_t max = size / 4;
    std::uint32_t secMask = 0x4164536b ^ offset;
    std::uint32_t* pHdr = (std::uint32_t*)buf;
    for (std::uint8_t j = 0; j < max; j++)
        *pHdr++ ^= secMask;
}*/

std::uint32_t dwgCompressor::litLength21(std::uint8_t opCode)
{
    std::uint32_t length = 8u + opCode;
    if (0x17 == length) {
        std::uint32_t n = compressedByte();
        length += n;
        if (0xffu == n) {
            do {
                n = compressedByte();
                n |= compressedHiByte();
                length += n;
            }
            while (0xffffu == n);
        }
    }

    return length;
}

bool dwgCompressor::decompress21(std::uint8_t *cbuf, std::uint8_t *dbuf, std::uint64_t csize, std::uint64_t dsize){
    compressedBuffer = cbuf;
    decompBuffer = dbuf;
    compressedSize = csize;
    decompSize = dsize;
    compressedPos = 0;
    decompPos = 0;
    compressedGood = true;
    decompGood = true;

    std::uint32_t length {0};
    std::uint32_t sourceOffset {0};
    std::uint8_t opCode {compressedByte()};
    if ((opCode >> 4) == 2){
        compressedInc( 2);
        length = compressedByte() & 0x07;
    }

    while (buffersGood()) {
        if (length == 0) {
            length = litLength21(opCode);
        }
        copyCompBytes21( length);

        if (decompPos >= decompSize) {
            break; //check if last chunk are compressed & terminate
        }

        length = 0;
        opCode = compressedByte();
        readInstructions21( opCode,  sourceOffset,  length);
        while (true) {
            //prevent crash with corrupted data
            if (sourceOffset > decompPos) {
                DRW_DBG("\nWARNING dwgCompressor::decompress21 => sourceOffset> dstIndex.\n");
                DRW_DBG("csize = "); DRW_DBG(compressedSize); DRW_DBG("  srcIndex = "); DRW_DBG(compressedPos);
                DRW_DBG("\ndsize = "); DRW_DBG(decompSize); DRW_DBG("  dstIndex = "); DRW_DBG(decompPos);
                sourceOffset = static_cast<std::uint32_t>(decompPos);
            }
            //prevent crash with corrupted data
            if (length > decompSize - decompPos){
                DRW_DBG("\nWARNING dwgCompressor::decompress21 => length > dsize - dstIndex.\n");
                DRW_DBG("csize = "); DRW_DBG(compressedSize); DRW_DBG("  srcIndex = "); DRW_DBG(compressedPos);
                DRW_DBG("\ndsize = "); DRW_DBG(decompSize); DRW_DBG("  dstIndex = "); DRW_DBG(decompPos);
                length = static_cast<std::uint32_t>(decompSize - decompPos);
                compressedPos = compressedSize; //force exit
                compressedGood = false;
            }
            sourceOffset = static_cast<std::uint32_t>(decompPos) - sourceOffset;
            // Back-reference copy - source may overlap destination
            if (sourceOffset + length <= decompPos || sourceOffset >= decompPos) {
                // No overlap with future writes - safe to use memcpy/memmove
                if (decompPos + length <= decompSize) {
                    std::memmove(decompBuffer + decompPos, decompBuffer + sourceOffset, length);
                    decompPos += length;
                } else {
                    for (std::uint32_t i=0; i< length; i++)
                        decompSet( decompByte( sourceOffset + i));
                }
            } else {
                for (std::uint32_t i=0; i< length; i++)
                    decompSet( decompByte( sourceOffset + i));
            }

            length = opCode & 7;
            if ((length != 0) || (compressedPos >= compressedSize)) {
                break;
            }
            opCode = compressedByte();
            if ((opCode >> 4) == 0) {
                break;
            }
            if ((opCode >> 4) == 15) {
                opCode &= 15;
            }
            readInstructions21( opCode, sourceOffset, length);
        }

        if (compressedPos >= compressedSize) {
            break;
        }
    }
    DRW_DBG("\ncsize = "); DRW_DBG(compressedSize); DRW_DBG("  srcIndex = "); DRW_DBG(compressedPos);
    DRW_DBG("\ndsize = "); DRW_DBG(decompSize); DRW_DBG("  dstIndex = "); DRW_DBG(decompPos);DRW_DBG("\n");

    return buffersGood();
}

void dwgCompressor::readInstructions21(std::uint8_t &opCode, std::uint32_t &sourceOffset, std::uint32_t &length){

    switch (opCode >> 4) {
    case 0:
        length = (opCode & 0x0f) + 0x13;
        sourceOffset = compressedByte();
        opCode = compressedByte();
        length = ((opCode >> 3) & 0x10) + length;
        sourceOffset = ((opCode & 0x78) << 5) + 1 + sourceOffset;
        break;
    case 1:
        length = (opCode & 0xf) + 3;
        sourceOffset = compressedByte();
        opCode = compressedByte();
        sourceOffset = ((opCode & 0xf8) << 5) + 1 + sourceOffset;
        break;
    case 2:
        sourceOffset = compressedByte();
        sourceOffset = (compressedHiByte() & 0xff00) | sourceOffset;
        length = opCode & 7;
        if ((opCode & 8) == 0) {
            opCode = compressedByte();
            length = (opCode & 0xf8) + length;
        } else {
            ++sourceOffset;
            length = (static_cast<std::uint32_t>(compressedByte()) << 3) + length;
            opCode = compressedByte();
            length = (((opCode & 0xf8) << 8) + length) + 0x100;
        }
        break;
    default:
        length = opCode >> 4;
        sourceOffset = opCode & 15;
        opCode = compressedByte();
        sourceOffset = (((opCode & 0xf8) << 1) + sourceOffset) + 1;
        break;
    }
}

const std::uint8_t dwgCompressor::CopyOrder21_01[] = {0};
const std::uint8_t dwgCompressor::CopyOrder21_02[] = {1,0};
const std::uint8_t dwgCompressor::CopyOrder21_03[] = {2,1,0};
const std::uint8_t dwgCompressor::CopyOrder21_04[] = {0,1,2,3};
const std::uint8_t dwgCompressor::CopyOrder21_05[] = {4,0,1,2,3};
const std::uint8_t dwgCompressor::CopyOrder21_06[] = {5,1,2,3,4,0};
const std::uint8_t dwgCompressor::CopyOrder21_07[] = {6,5,1,2,3,4,0};
const std::uint8_t dwgCompressor::CopyOrder21_08[] = {0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_09[] = {8,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_10[] = {9,1,2,3,4,5,6,7,8,0};
const std::uint8_t dwgCompressor::CopyOrder21_11[] = {10,9,1,2,3,4,5,6,7,8,0};
const std::uint8_t dwgCompressor::CopyOrder21_12[] = {8,9,10,11,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_13[] = {12,8,9,10,11,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_14[] = {13,9,10,11,12,1,2,3,4,5,6,7,8,0};
const std::uint8_t dwgCompressor::CopyOrder21_15[] = {14,13,9,10,11,12,1,2,3,4,5,6,7,8,0};
const std::uint8_t dwgCompressor::CopyOrder21_16[] = {8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_17[] = {9,10,11,12,13,14,15,16,8,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_18[] = {17,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,0};
const std::uint8_t dwgCompressor::CopyOrder21_19[] = {18,17,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_20[] = {16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_21[] = {20,16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_22[] = {21,20,16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_23[] = {22,21,20,16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_24[] = {16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_25[] = {17,18,19,20,21,22,23,24,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_26[] = {25,17,18,19,20,21,22,23,24,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_27[] = {26,25,17,18,19,20,21,22,23,24,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_28[] = {24,25,26,27,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_29[] = {28,24,25,26,27,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_30[] = {29,28,24,25,26,27,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_31[] = {30,26,27,28,29,18,19,20,21,22,23,24,25,10,11,12,13,14,15,16,17,2,3,4,5,6,7,8,9,1,0};
const std::uint8_t dwgCompressor::CopyOrder21_32[] = {24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t *dwgCompressor::CopyOrder21[dwgCompressor::Block21OrderArray] = {
        nullptr,
        CopyOrder21_01, CopyOrder21_02, CopyOrder21_03, CopyOrder21_04,
        CopyOrder21_05, CopyOrder21_06, CopyOrder21_07, CopyOrder21_08,
        CopyOrder21_09, CopyOrder21_10, CopyOrder21_11, CopyOrder21_12,
        CopyOrder21_13, CopyOrder21_14, CopyOrder21_15, CopyOrder21_16,
        CopyOrder21_17, CopyOrder21_18, CopyOrder21_19, CopyOrder21_20,
        CopyOrder21_21, CopyOrder21_22, CopyOrder21_23, CopyOrder21_24,
        CopyOrder21_25, CopyOrder21_26, CopyOrder21_27, CopyOrder21_28,
        CopyOrder21_29, CopyOrder21_30, CopyOrder21_31, CopyOrder21_32
};

void dwgCompressor::copyBlock21(const std::uint32_t length)
{
    if (MaxBlock21Length < length) {
        return;
    }

    const std::uint8_t *order {CopyOrder21[length]};
    if (nullptr == order) {
        return;
    }

    for (std::uint32_t index = 0; (length > index) && buffersGood(); ++index) {
        decompSet( compressedByte( compressedPos + order[index]));
    }
    compressedInc( length);
}

bool dwgCompressor::copyCompBytes21(std::uint32_t length)
{
    DRW_DBG("\ncopyCompBytes21() "); DRW_DBG(length); DRW_DBG("\n");

    while (length >= MaxBlock21Length) {
        copyBlock21( MaxBlock21Length);
        length -= MaxBlock21Length;
    }

    copyBlock21( length);

    return buffersGood();
}


secEnum::DWGSection secEnum::getEnum(const std::string &nameSec){
    //TODO: complete it
    if (nameSec=="AcDb:Header"){
        return HEADER;
    } else if (nameSec=="AcDb:Classes"){
        return CLASSES;
    } else if (nameSec=="AcDb:SummaryInfo"){
        return SUMARYINFO;
    } else if (nameSec=="AcDb:Preview"){
        return PREVIEW;
    } else if (nameSec=="AcDb:VBAProject"){
        return VBAPROY;
    } else if (nameSec=="AcDb:AppInfo"){
        return APPINFO;
    } else if (nameSec=="AcDb:FileDepList"){
        return FILEDEP;
    } else if (nameSec=="AcDb:RevHistory"){
        return REVHISTORY;
    } else if (nameSec=="AcDb:Security"){
        return SECURITY;
    } else if (nameSec=="AcDb:AcDbObjects"){
        return OBJECTS;
    } else if (nameSec=="AcDb:ObjFreeSpace"){
        return OBJFREESPACE;
    } else if (nameSec=="AcDb:Template"){
        return TEMPLATE;
    } else if (nameSec=="AcDb:Handles"){
        return HANDLES;
    } else if (nameSec=="AcDb:AcDsPrototype_1b"){
        return PROTOTYPE;
    } else if (nameSec=="AcDb:AuxHeader"){
        return AUXHEADER;
    } else if (nameSec=="AcDb:Signature"){
        return SIGNATURE;
    } else if (nameSec=="AcDb:AppInfoHistory"){ //in ac1021
        return APPINFOHISTORY;
//    } else if (nameSec=="AcDb:Extended Entity Data"){
//        return EXTEDATA;
//    } else if (nameSec=="AcDb:PROXY ENTITY GRAPHICS"){
//        return PROXYGRAPHICS;
    }
    return UNKNOWNS;
}

// ---------------------------------------------------------------------------
// dwgUtil checksum / CRC
// ---------------------------------------------------------------------------

/*!
 * \brief Adler-32-variant checksum used by R2004 (AC1018) section pages
 * and their headers.
 */
std::uint32_t dwgUtil::checksum18(std::uint32_t seed, const std::uint8_t* data, std::uint64_t sz){
    std::uint32_t sum1 = seed & 0xffff;
    std::uint32_t sum2 = seed >> 0x10;
    while (sz != 0) {
        std::uint64_t chunkSize = 0x15b0 < sz ? 0x15b0 : sz;
        sz -= chunkSize;
        for (std::uint64_t i = 0; i < chunkSize; i++) {
            sum1 += *data++;
            sum2 += sum1;
        }
        sum1 %= 0xFFF1;
        sum2 %= 0xFFF1;
    }
    return (sum2 << 0x10) | (sum1 & 0xffff);
}

// CRC-32 table (ISO-HDLC / PKZIP, same polynomial 0xEDB88320 as dwgBuffer).
namespace {
const std::uint32_t crc32Table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};
} // namespace

/*!
 * \brief Standard CRC-32/ISO-HDLC over a byte buffer.
 * Same algorithm as dwgBuffer::crc32 but operates on a raw buffer.
 */
std::uint32_t dwgUtil::crc32(std::uint32_t seed, const std::uint8_t* data, std::uint32_t sz) {
    std::uint32_t invertedCrc = ~seed;
    for (std::uint32_t i = 0; i < sz; ++i) {
        invertedCrc = crc32Table[(invertedCrc ^ data[i]) & 0xFF] ^ (invertedCrc >> 8);
    }
    return ~invertedCrc;
}
