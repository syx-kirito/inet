//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __INET_BYTELENGTHCHUNK_H_
#define __INET_BYTELENGTHCHUNK_H_

#include "inet/common/packet/Chunk.h"

namespace inet {

class ByteLengthChunk : public Chunk
{
  friend Chunk;

  protected:
    int64_t byteLength;

  protected:
    virtual const char *getSerializerClassName() const override { return "inet::ByteLengthChunkSerializer"; }

  protected:
    static std::shared_ptr<Chunk> createChunk(const std::type_info& typeInfo, const std::shared_ptr<Chunk>& chunk, int64_t byteOffset, int64_t byteLength);

  public:
    ByteLengthChunk();
    ByteLengthChunk(const ByteLengthChunk& other);
    ByteLengthChunk(int64_t byteLength);

    virtual ByteLengthChunk *dup() const override { return new ByteLengthChunk(*this); }
    virtual std::shared_ptr<Chunk> dupShared() const override { return std::make_shared<ByteLengthChunk>(*this); }

    virtual Type getChunkType() const override { return TYPE_BYTELENGTH; }

    virtual int64_t getByteLength() const override { return byteLength; }
    virtual void setByteLength(int64_t byteLength);

    virtual bool insertToBeginning(const std::shared_ptr<Chunk>& chunk) override;
    virtual bool insertToEnd(const std::shared_ptr<Chunk>& chunk) override;

    virtual bool removeFromBeginning(int64_t byteLength) override;
    virtual bool removeFromEnd(int64_t byteLength) override;

    // TODO: remove
    virtual std::shared_ptr<Chunk> peek(int64_t byteOffset = 0, int64_t byteLength = -1) const override {
        return peek(Iterator(true, 0, -1), byteLength);
    }

    virtual std::shared_ptr<Chunk> peek(const Iterator& iterator, int64_t byteLength = -1) const override;

    virtual std::string str() const override;
};

} // namespace

#endif // #ifndef __INET_BYTELENGTHCHUNK_H_

