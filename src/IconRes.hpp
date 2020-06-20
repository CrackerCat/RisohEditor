// IconRes.hpp --- Icon Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, 
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#include <cassert>
#include "MByteStreamEx.hpp"

//////////////////////////////////////////////////////////////////////////////

// the common header of RT_CURSOR, RT_ICON, icon/cursor files
typedef struct ICONDIR
{
    WORD           idReserved;  // Reserved (must be 0)
    WORD           idType;      // RES_ICON or RES_CURSOR
    WORD           idCount;     // How many images?
} ICONDIR, *LPICONDIR;

// the header of RT_CURSOR
typedef struct LOCALHEADER {
    WORD xHotSpot;
    WORD yHotSpot;
} LOCALHEADER;

// the entry of icon/cursor files (after ICONDIR)
typedef struct ICONDIRENTRY
{
    BYTE        bWidth;         // Width, in pixels, of the image
    BYTE        bHeight;        // Height, in pixels, of the image
    BYTE        bColorCount;    // Number of colors in image (0 if >= 8bpp)
    BYTE        bReserved;      // Reserved (must be 0)
    union
    {
        WORD    xHotSpot;       // Hot Spot X
        WORD    wPlanes;        // Color Planes
    };
    union
    {
        WORD    yHotSpot;       // Hot Spot Y
        WORD    wBitCount;      // Bits per pixel
    };
    DWORD       dwBytesInRes;   // How many bytes in this resource?
    DWORD       dwImageOffset;  // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

// the entry of group icon resource (RT_GROUP_ICON)
#include <pshpack2.h>
typedef struct GRPICONDIRENTRY
{
    BYTE   bWidth;              // Width, in pixels, of the image
    BYTE   bHeight;             // Height, in pixels, of the image
    BYTE   bColorCount;         // Number of colors in image (0 if >=8bpp)
    BYTE   bReserved;           // Reserved
    WORD   wPlanes;             // Color Planes
    WORD   wBitCount;           // Bits per pixel
    DWORD  dwBytesInRes;        // how many bytes in this resource?
    WORD   nID;                 // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;
#include <poppack.h>

// the entry of group cursor resource (RT_GROUP_CURSOR)
#include <pshpack2.h>
typedef struct GRPCURSORDIRENTRY
{
    WORD    wWidth;             // Width, in pixels, of the image
    WORD    wHeight;            // Height, in pixels, of the image
    WORD    wPlanes;            // Must be 1
    WORD    wBitCount;          // Bits per pixel
    DWORD   dwBytesInRes;       // how many bytes in this resource?
    WORD    nID;                // the ID
} GRPCURSORDIRENTRY, *LPGRPCURSORDIRENTRY;
#include <poppack.h>

//////////////////////////////////////////////////////////////////////////////

class IconFile
{
public:
    typedef ICONDIRENTRY            EntryType;
    typedef std::vector<EntryType>  EntryListType;
    typedef std::vector<BYTE>       DataType;
    typedef std::vector<DataType>   DataListType;
    typedef GRPICONDIRENTRY         ResourceEntryType;

    IconFile()
    {
        memset(&m_dir, 0, sizeof(m_dir));
    }

    void clear()
    {
        memset(&m_dir, 0, sizeof(m_dir));
        m_entries.clear();
        m_images.clear();
    }

    bool IsIconDirOK() const
    {
        return m_dir.idReserved == 0 && m_dir.idType == RES_ICON &&
               m_dir.idCount > 0;
    }

    WORD type() const
    {
        return m_dir.idType;
    }

    int SizeOfIconGroup() const
    {
        return sizeof(ICONDIR) + sizeof(ResourceEntryType) * GetImageCount();
    }

    WORD GetImageCount() const
    {
        return m_dir.idCount;
    }

    DataType& GetImage(int index)
    {
        assert(0 <= index && index < GetImageCount());
        return m_images[index];
    }
    const DataType& GetImage(int index) const
    {
        assert(0 <= index && index < GetImageCount());
        return m_images[index];
    }

    BYTE *GetImagePtr(int index)
    {
        return &(GetImage(index)[0]);
    }
    const BYTE *GetImagePtr(int index) const
    {
        return &(GetImage(index)[0]);
    }

    DWORD GetImageSize(int index) const
    {
        return DWORD(m_images[index].size());
    }

    bool LoadFromStream(const MByteStreamEx& stream)
    {
        DWORD size = sizeof(ICONDIR);
        if (stream.size() < size)
            return false;

        memcpy(&m_dir, stream.ptr(), size);
        if (!IsIconDirOK())
            return false;

        stream.pos(size);
        size = m_dir.idCount * sizeof(EntryType);
        if (stream.remainder() < size)
            return false;

        m_entries.resize(m_dir.idCount);
        memcpy(&m_entries[0], stream.ptr(stream.pos()), size);
        m_images.resize(m_dir.idCount);

        for (int i = 0; i < m_dir.idCount; i++)
        {
            stream.pos(m_entries[i].dwImageOffset);
            if (stream.remainder() < m_entries[i].dwBytesInRes)
            {
                return false;
            }

            LPBYTE pb = (LPBYTE)stream.ptr(stream.pos());
            m_images[i].assign(&pb[0], &pb[m_entries[i].dwBytesInRes]);
        }
        return true;
    }

    bool LoadFromFile(LPCTSTR pszFileName)
    {
        MByteStreamEx stream;
        if (!stream.LoadFromFile(pszFileName))
            return false;

        return LoadFromStream(stream);
    }

    bool SaveToStream(MByteStreamEx& stream)
    {
        stream.clear();
        if (!IsIconDirOK())
            return false;

        DWORD offset = sizeof(ICONDIR);
        const DWORD SizeOfEntries = GetImageCount() * sizeof(EntryType);
        offset += SizeOfEntries;

        const DWORD nCount = GetImageCount();
        for (DWORD i = 0; i < nCount; ++i)
        {
            m_entries[i].dwImageOffset = offset;
            offset += m_entries[i].dwBytesInRes;
        }

        if (!stream.WriteRaw(m_dir) ||
            !stream.WriteData(&m_entries[0], SizeOfEntries))
        {
            return false;
        }

        for (DWORD i = 0; i < nCount; ++i)
        {
            if (!stream.WriteData(GetImagePtr(i), GetImageSize(i)))
                return false;
        }

        return true;
    }

    bool SaveToFile(LPCTSTR pszFileName)
    {
        MByteStreamEx stream;
        if (!SaveToStream(stream))
            return false;
        return stream.SaveToFile(pszFileName);
    }

    DataType GetIconGroup(int nBaseID) const
    {
        DataType group(SizeOfIconGroup());

        memcpy(&group[0], &m_dir, sizeof(ICONDIR));

        int offset = sizeof(ICONDIR);
        for (int i = 0; i < GetImageCount(); i++)
        {
            BITMAPCOREHEADER    bmch;
            BITMAPINFOHEADER    bmih;

            memcpy(&bmch, GetImagePtr(i), sizeof(bmch));
            bool bCoreOnly = (bmch.bcSize == sizeof(bmch));
            if (!bCoreOnly)
            {
                memcpy(&bmih, GetImagePtr(i), sizeof(bmih));
            }

            ResourceEntryType  grpEntry;
            grpEntry.bWidth             = m_entries[i].bWidth;
            grpEntry.bHeight            = m_entries[i].bHeight;
            grpEntry.bColorCount        = m_entries[i].bColorCount;
            grpEntry.bReserved          = m_entries[i].bReserved;

            if (bCoreOnly)
            {
                if (grpEntry.bWidth == 0)
                    grpEntry.bWidth     = (BYTE)bmch.bcWidth;
                if (grpEntry.bHeight == 0)
                    grpEntry.bHeight    = (BYTE)bmch.bcHeight;

                grpEntry.wPlanes        = bmch.bcPlanes;
                grpEntry.wBitCount      = bmch.bcBitCount;
            }
            else
            {
                if (grpEntry.bWidth == 0)
                    grpEntry.bWidth     = (BYTE)bmih.biWidth;
                if (grpEntry.bHeight == 0)
                    grpEntry.bHeight    = (BYTE)bmih.biHeight;

                grpEntry.wPlanes        = bmih.biPlanes;
                grpEntry.wBitCount      = bmih.biBitCount;
            }

            grpEntry.dwBytesInRes       = m_entries[i].dwBytesInRes;
            grpEntry.nID                = WORD(nBaseID + i);

            memcpy(&group[offset], &grpEntry, sizeof(grpEntry));
            offset += sizeof(grpEntry);
        }

        return group;
    }

protected:
    ICONDIR                     m_dir;
    EntryListType               m_entries;
    DataListType                m_images;
}; // class IconFile

//////////////////////////////////////////////////////////////////////////////

class CursorFile
{
public:
    typedef ICONDIRENTRY            EntryType;
    typedef std::vector<EntryType>  EntryListType;
    typedef std::vector<BYTE>       DataType;
    typedef std::vector<DataType>   DataListType;
    typedef GRPCURSORDIRENTRY       ResourceEntryType;

    CursorFile()
    {
        memset(&m_dir, 0, sizeof(m_dir));
    }

    void clear()
    {
        memset(&m_dir, 0, sizeof(m_dir));
        m_entries.clear();
        m_images.clear();
    }

    bool IsIconDirOK() const
    {
        return m_dir.idReserved == 0 && m_dir.idType == RES_CURSOR &&
               m_dir.idCount > 0;
    }

    WORD type() const
    {
        return m_dir.idType;
    }

    int SizeOfCursorGroup() const
    {
        return sizeof(ICONDIR) + sizeof(ResourceEntryType) * GetImageCount();
    }

    WORD GetImageCount() const
    {
        return m_dir.idCount;
    }

    DataType& GetImage(int index)
    {
        assert(0 <= index && index < GetImageCount());
        return m_images[index];
    }
    const DataType& GetImage(int index) const
    {
        assert(0 <= index && index < GetImageCount());
        return m_images[index];
    }

    BYTE *GetImagePtr(int index)
    {
        return &(GetImage(index)[0]);
    }
    const BYTE *GetImagePtr(int index) const
    {
        return &(GetImage(index)[0]);
    }

    DWORD GetImageSize(int index) const
    {
        return DWORD(m_images[index].size());
    }

    bool LoadFromStream(const MByteStreamEx& stream)
    {
        DWORD size = sizeof(ICONDIR);
        if (stream.size() < size)
            return false;

        memcpy(&m_dir, stream.ptr(), size);
        if (!IsIconDirOK())
            return false;

        stream.pos(size);
        size = m_dir.idCount * sizeof(EntryType);
        if (stream.remainder() < size)
            return false;

        m_entries.resize(m_dir.idCount);
        memcpy(&m_entries[0], stream.ptr(stream.pos()), size);
        m_images.resize(m_dir.idCount);

        for (int i = 0; i < m_dir.idCount; i++)
        {
            stream.pos(m_entries[i].dwImageOffset);
            if (stream.remainder() < m_entries[i].dwBytesInRes)
            {
                return false;
            }

            LOCALHEADER local;
            local.xHotSpot = m_entries[i].xHotSpot;
            local.yHotSpot = m_entries[i].yHotSpot;
            LPBYTE pb = (LPBYTE)stream.ptr(stream.pos());

            MByteStreamEx bs;
            if (!bs.WriteRaw(local))
                return false;
            if (!bs.WriteData(pb, m_entries[i].dwBytesInRes))
                return false;

            m_entries[i].dwBytesInRes += sizeof(local);
            m_images[i] = bs.data();
        }
        return true;
    }

    bool LoadFromFile(LPCTSTR pszFileName)
    {
        MByteStreamEx stream;
        if (!stream.LoadFromFile(pszFileName))
            return false;

        return LoadFromStream(stream);
    }

    bool SaveToStream(MByteStreamEx& stream)
    {
        stream.clear();
        if (!IsIconDirOK())
            return false;

        DWORD offset = sizeof(ICONDIR);
        const DWORD SizeOfEntries = GetImageCount() * sizeof(EntryType);
        offset += SizeOfEntries;

        const DWORD nCount = GetImageCount();
        for (DWORD i = 0; i < nCount; ++i)
        {
            m_entries[i].dwImageOffset = offset;
            offset += m_entries[i].dwBytesInRes;
        }

        if (!stream.WriteRaw(m_dir) ||
            !stream.WriteData(&m_entries[0], SizeOfEntries))
        {
            return false;
        }

        for (DWORD i = 0; i < nCount; ++i)
        {
            LPBYTE pb = GetImagePtr(i) + sizeof(LOCALHEADER);
            DWORD size = GetImageSize(i) - sizeof(LOCALHEADER);
            if (!stream.WriteData(pb, size))
                return false;
        }

        return true;
    }

    bool SaveToFile(LPCTSTR pszFileName)
    {
        MByteStreamEx stream;
        if (!SaveToStream(stream))
            return false;
        return stream.SaveToFile(pszFileName);
    }

    DataType GetCursorGroup(int nBaseID) const
    {
        DataType group(SizeOfCursorGroup());

        memcpy(&group[0], &m_dir, sizeof(ICONDIR));

        int offset = sizeof(ICONDIR);
        for (int i = 0; i < GetImageCount(); i++)
        {
            BITMAPCOREHEADER    bmch;
            BITMAPINFOHEADER    bmih;
            LOCALHEADER         local;

            const BYTE *pb = GetImagePtr(i);
            memcpy(&local, pb, sizeof(local));
            memcpy(&bmch, pb + sizeof(local), sizeof(bmch));

            bool bCoreOnly = (bmch.bcSize == sizeof(bmch));
            if (!bCoreOnly)
            {
                memcpy(&bmih, pb + sizeof(local), sizeof(bmih));
            }

            ResourceEntryType  grpEntry;
            grpEntry.wWidth             = m_entries[i].bWidth;
            grpEntry.wHeight            = m_entries[i].bHeight;

            if (bCoreOnly)
            {
                if (grpEntry.wWidth == 0)
                    grpEntry.wWidth     = (WORD)bmch.bcWidth;
                if (grpEntry.wHeight == 0)
                    grpEntry.wHeight    = (WORD)bmch.bcHeight;

                grpEntry.wPlanes        = bmch.bcPlanes;
                grpEntry.wBitCount      = bmch.bcBitCount;
            }
            else
            {
                if (grpEntry.wWidth == 0)
                    grpEntry.wWidth     = (WORD)bmih.biWidth;
                if (grpEntry.wHeight == 0)
                    grpEntry.wHeight    = (WORD)bmih.biHeight;

                grpEntry.wPlanes        = bmih.biPlanes;
                grpEntry.wBitCount      = bmih.biBitCount;
            }

            grpEntry.wHeight           *= 2;
            grpEntry.dwBytesInRes       = m_entries[i].dwBytesInRes;
            grpEntry.nID                = WORD(nBaseID + i);

            memcpy(&group[offset], &grpEntry, sizeof(grpEntry));
            offset += sizeof(grpEntry);
        }

        return group;
    }

protected:
    ICONDIR                     m_dir;
    EntryListType               m_entries;
    DataListType                m_images;
}; // class CursorFile
