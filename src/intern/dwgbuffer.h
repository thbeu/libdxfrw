/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DWGBUFFER_H
#define DWGBUFFER_H

#include <fstream>
#include <sstream>
#include <memory>
#include <cstring>
#include "../drw_base.h"

class DRW_Coord;
class DRW_TextCodec;

class dwgBasicStream{
protected:
    dwgBasicStream() = default;
public:
    virtual ~dwgBasicStream() = default;
    virtual bool read(duint8* s, duint64 n) = 0;
    virtual duint64 size() const = 0;
    virtual duint64 getPos() const = 0;
    virtual bool setPos(duint64 p) = 0;
    virtual bool good() const = 0;
    virtual dwgBasicStream* clone() const = 0;
    // Direct buffer access for fast-path reads (returns null if not memory-backed)
    virtual duint8* getBuffer() const { return nullptr; }
};

class dwgFileStream: public dwgBasicStream{
public:
    explicit dwgFileStream(std::ifstream *s)
        :stream{s}
    {
        stream->seekg (0, std::ios::end);
        sz = static_cast<duint64>(stream->tellg());
        stream->seekg(0, std::ios_base::beg);
        // Pre-read entire file into memory for faster random access
        buf.reset(new duint8[static_cast<size_t>(sz)]);
        stream->read(reinterpret_cast<char*>(buf.get()), static_cast<std::streamsize>(sz));
        isOk = stream->good();
    }
    bool read(duint8* s, duint64 n) override;
    duint64 size() const override{return sz;}
    duint64 getPos() const override{return pos;}
    bool setPos(duint64 p) override;
    bool good() const override{return isOk;}
    dwgBasicStream* clone() const override{return new dwgFileStream(stream);}
    duint8* getBuffer() const override{return buf.get();}
private:
    std::ifstream *stream{nullptr};
    std::unique_ptr<duint8[]> buf;
    duint64 sz{0};
    duint64 pos{0};
    bool isOk{true};
};

class dwgCharStream: public dwgBasicStream{
public:
    dwgCharStream(duint8 *buf, duint64 s)
        :stream{buf}
        ,sz{s}
    {}
    bool read(duint8* s, duint64 n) override {
        if ( n > (sz - pos) ) {
            isOk = false;
            return false;
        }
        std::memcpy(s, stream + pos, static_cast<size_t>(n));
        pos += n;
        return true;
    }
    duint64 size() const override {return sz;}
    duint64 getPos() const override {return pos;}
    bool setPos(duint64 p) override {
        if (p > sz) {
            isOk = false;
            return false;
        }
        pos = p;
        return true;
    }
    bool good() const override {return isOk;}
    dwgBasicStream* clone() const override {return new dwgCharStream(stream, sz);}
    duint8* getBuffer() const override{return nullptr;}
private:
    duint8 *stream{nullptr};
    duint64 sz{0};
    duint64 pos{0};
    bool isOk{true};
};

class dwgBuffer {
public:
    struct DirectTag {};
    dwgBuffer(std::ifstream *stream, DRW_TextCodec *decoder = nullptr);
    dwgBuffer(duint8 *buf, duint64 size, DRW_TextCodec *decoder= nullptr);
    dwgBuffer(DirectTag, duint8 *buf, duint64 size, DRW_TextCodec *decoder)
        :decoder{decoder}, maxSize{size}, directBuf{nullptr}, directGood{true} {}
    dwgBuffer( const dwgBuffer& org );
    dwgBuffer( dwgBuffer&& org ) = default;
    dwgBuffer& operator=( const dwgBuffer& org );
    dwgBuffer& operator=( dwgBuffer&& org ) = default;
    virtual ~dwgBuffer() = default;
    duint64 size() const {return directBuf ? maxSize : filestr->size();}
    bool setPosition(duint64 pos);
    duint64 getPosition() const;
    void resetPosition(){setPosition(0); setBitPos(0);}
    void setBitPos(duint8 pos);
    duint8 getBitPos() const {return bitPos;}
    bool moveBitPos(dint64 size);

    duint8 getBit();  //B
    bool getBoolBit();  //B as bool
    duint8 get2Bits(); //BB
    duint8 get3Bits(); //3B
    duint16 getBitShort(); //BS
    dint16 getSBitShort(); //BS
    dint32 getBitLong(); //BL
    duint64 getBitLongLong();  //BLL (R24)
    double getBitDouble(); //BD
    //2BD => call BD 2 times
    DRW_Coord get3BitDouble(); //3BD
    duint8 getRawChar8();  //RC
    duint16 getRawShort16();  //RS
    double getRawDouble(); //RD
    duint32 getRawLong32();   //RL
    duint64 getRawLong64();   //RLL
    DRW_Coord get2RawDouble(); //2RD
    //3RD => call RD 3 times
    duint32 getUModularChar(); //UMC, unsigned for offsets in 1015
    dint32 getModularChar(); //MC
    dint32 getModularShort(); //MS
    dwgHandle getHandle(); //H
    dwgHandle getOffsetHandle(duint32 href); //H converted to hard
    UTF8STRING getVariableText(DRW::Version v, bool nullTerm = true); //TV => call TU for 2007+ or T for previous versions
    UTF8STRING getCP8Text(); //T 8 bit text converted from codepage to utf8
    UTF8STRING getUCSText(bool nullTerm = true); //TU unicode 16 bit (UCS) text converted to utf8
    UTF8STRING getUCSStr(duint32 ts);

    duint16 getObjType(DRW::Version v);  //OT

    //X, U, SN,

    DRW_Coord getExtrusion(bool b_R2000_style); //BE
    double getDefaultDouble(double d); //DD
    double getThickness(bool b_R2000_style);//BT
    //3DD
    /// Read a CMC (Complex Material Color) field per ODA spec.
    /// Returns the indexed color value (or sentinel for ByLayer/ByBlock/RGB).
    /// If rgb24 is non-null, also populates the 24-bit RGB component for
    /// type-0xC2 (true color) entries; otherwise leaves rgb24 untouched.
    /// CMC color decoder. R15 and earlier: directly BS as ACIS. R2004+:
    /// BS index, BL rgb-with-method, RC method byte, optional name + book
    /// name from str_dat (R2007+) or dat (earlier).  See libreDWG
    /// bits.c:3704-3743 for the canonical layout.
    ///
    /// @param v        version
    /// @param rgb24    out: 24-bit RGB for type 0xC2 (true color)
    /// @param strBuf   string buffer for R2007+ name/book reads (separate
    ///                 string area).  If null, falls back to *this — wrong
    ///                 for R2007+ but matches the historical behavior.
    /// @param outName  out: color name (when method byte bit 1 set)
    /// @param outBookName out: book name (when method byte bit 2 set)
    duint32 getCmColor(DRW::Version v,
                       dint32* rgb24 = nullptr,
                       dwgBuffer* strBuf = nullptr,
                       UTF8STRING* outName = nullptr,
                       UTF8STRING* outBookName = nullptr); //CMC
    duint32 getEnColor(DRW::Version v); //ENC
    //TC

    duint16 getBERawShort16();  //RS big-endian order

    bool isGood() const {return directBuf ? (directGood && directPos <= maxSize) : filestr->good();}
    bool getBytes(duint8 *buf, duint64 size);
    dint64 numRemainingBytes() const {return directBuf ? (maxSize - directPos) : (maxSize- filestr->getPos());}
    void enableDirectBuf(duint8 * /*buf*/) { /* DISABLED for testing */ }
    duint8* getDirectPtr() const { return directBuf ? directBuf + directPos : nullptr; }
    void reinitDirect(duint8 * /*buf*/, duint64 size, DRW_TextCodec *dc) {
        /* directBuf DISABLED for testing */
        maxSize = size; decoder = dc; bitPos = 0; currByte = 0;
    }

    duint16 crc8(duint16 dx,dint64 start,dint64 end);
    duint32 crc32(duint32 seed,dint32 start,dint32 end);

//    duint8 getCurrByte(){return currByte;}
    DRW_TextCodec *decoder{nullptr};

    /// Side-channel: getEnColor() sets this true when the ENC field's flag
    /// byte has bit 0x40 (AcDbColor reference). Caller (DRW_Entity::parseDwg)
    /// captures it immediately, then DRW_Entity::parseDwgEntHandle consumes
    /// the corresponding offset handle at the START of the handle stream
    /// (libreDWG common_entity_data.spec:454-459 — read from hdl_dat).
    bool lastEnColorHadDbColorRef{false};

    /// Side-channel: ENC inline color/book names. Populated by getEnColor()
    /// when flags & 0x41 == 0x41 (color name) or flags & 0x42 == 0x42
    /// (book name). libreDWG common_entity_data.spec:468-475 reads these as
    /// 8-bit TV from the data stream (FIELD_TV — deliberately 8-bit even on
    /// R2007+; spec quirk verified against real files). Caller in
    /// DRW_Entity::parseDwg captures these and populates entity.colorName
    /// as "BOOK$ENTRY"; the inline ENC name takes precedence over the
    /// dbColorMap-resolved DBCOLOR name (entity-level override).
    UTF8STRING lastEnColorName;
    UTF8STRING lastEnColorBookName;

    /// Side-channel: ENC alpha-raw (DXF code 440). Populated by getEnColor()
    /// when flag 0x20 is set. libreDWG common_entity_data.spec:439 — the
    /// BL alpha_raw packs alpha_type in the high byte (0=ByLayer, 1=ByBlock,
    /// 3=explicit alpha) and alpha 0..255 in the low byte. Stored raw;
    /// DRW_Entity::parseDwg copies into transparency for the filter to
    /// interpret.  0 = "not set" (default for entities without flag 0x20).
    duint32 lastEnColorAlphaRaw{0};

private:
    std::unique_ptr<dwgBasicStream> filestr;
    duint64 maxSize{0};
    duint8 currByte{0};
    duint8 bitPos{0};
    // Fast-path: direct buffer access (avoids virtual dispatch)
    duint8* directBuf{nullptr};
    duint64 directPos{0};
    bool directGood{true};

    UTF8STRING get8bitStr();
    UTF8STRING get16bitStr(duint32 textSize, bool nullTerm = true);
};

#endif // DWGBUFFER_H
