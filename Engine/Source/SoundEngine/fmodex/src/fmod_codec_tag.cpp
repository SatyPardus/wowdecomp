#include "fmod_settings.h"

#ifdef FMOD_SUPPORT_TAGS

#include "fmod.h"
#include "fmod_codec_tag.h"
#include "fmod_debug.h"
#include "fmod_file.h"
#include "fmod_metadata.h"
#include "fmod_string.h"

#include <stdio.h>
#include <stdlib.h>


namespace FMOD
{

FMOD_CODEC_DESCRIPTION_EX tagcodec;

#ifdef PLUGIN_EXPORTS

#ifdef __cplusplus
extern "C" {
#endif

    /*
        FMODGetCodecDescription is mandatory for every fmod plugin.  This is the symbol the register plugin function searches for.
        Must be declared with F_API to make it export as stdcall.
    */
    F_DECLSPEC F_DLLEXPORT FMOD_CODEC_DESCRIPTION_EX * F_API FMODGetCodecDescriptionEx()
    {
        return CodecTag::getDescriptionEx();
    }

#ifdef __cplusplus
}
#endif

#endif  /* PLUGIN_EXPORTS */


#if defined(FMOD_SUPPORT_DSHOW) || defined(FMOD_SUPPORT_ASF)

static const FMOD_GUID ASF_Header_Object                       = { 0x75B22630,0x668E,0x11CF,{0xA6,0xD9,0x00,0xAA,0x00,0x62,0xCE,0x6C}};
static const FMOD_GUID ASF_Content_Description_Object          = { 0x75B22633,0x668E,0x11CF,{0xA6,0xD9,0x00,0xAA,0x00,0x62,0xCE,0x6C}};
static const FMOD_GUID ASF_Extended_Content_Description_Object = { 0xD2D0A440,0xE307,0x11D2,{0x97,0xF0,0x00,0xA0,0xC9,0x5E,0xA8,0x50}};

#ifdef FMOD_SUPPORT_PRAGMAPACK
	#pragma pack(1)
#endif

typedef struct
{
	FMOD_GUID      guid FMOD_PACKED_INTERNAL;
    FMOD_UINT64    size FMOD_PACKED_INTERNAL;
} FMOD_PACKED ASF_CHUNK;

typedef struct
{
    int     number      FMOD_PACKED_INTERNAL;
    char    Reserved1   FMOD_PACKED_INTERNAL;
    char    Reserved2   FMOD_PACKED_INTERNAL;
} FMOD_PACKED ASF_HEADEROBJECT;

#ifdef FSOUND_SUPPORT_PRAGMAPACK
    #ifdef CODEWARRIOR
    #pragma pack(0)
    #else
    #pragma pack()
    #endif
#endif

#endif


#define ID3V2_FLAGS_UNSYNCHRONISATION 0x80
#define ID3V2_FLAGS_EXTHEADER         0x40
#define ID3V2_FLAGS_EXPERIMENTAL      0x20
#define ID3V2_FLAGS_FOOTER            0x10


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: 6a1ec5be manual
// OFFSET: 0x00914990
FMOD_CODEC_DESCRIPTION_EX *CodecTag::getDescriptionEx()
{
    FMOD_memset(&tagcodec, 0, sizeof(FMOD_CODEC_DESCRIPTION_EX));

    tagcodec.name        = "FMOD Tag Reader Codec";
    tagcodec.version     = 0x00010100;
    tagcodec.timeunits   = FMOD_TIMEUNIT_PCM;
    tagcodec.open        = &CodecTag::openCallback;
    tagcodec.close       = &CodecTag::closeCallback;
    tagcodec.read        = &CodecTag::readCallback;
    tagcodec.setposition = &CodecTag::setPositionCallback;

    tagcodec.mType       = FMOD_SOUND_TYPE_TAG;
    tagcodec.mSize       = sizeof(CodecTag);

    return &tagcodec;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: 43b2a480 auto
// OFFSET: 0x00914890
FMOD_RESULT CodecTag::openInternal(FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO *userexinfo)
{
    FMOD_RESULT  result = FMOD_OK;
    unsigned int startoffset, filepos;

    init(FMOD_SOUND_TYPE_TAG);

    FLOG((FMOD_DEBUG_LEVEL_LOG, __FILE__, __LINE__, "CodecTag::openInternal", "attempting to open ID3 or ASF tags..\n"));

    if (usermode & FMOD_IGNORETAGS)
    {
        return FMOD_ERR_FORMAT;
    }

	result = mFile->seek(0, SEEK_SET);
    if (result != FMOD_OK)
    {
        return result;
    }

    result = readTags();
    if (result == FMOD_OK)
    {
        result = mFile->tell(&filepos);
        if (result != FMOD_OK)
        {
            return result;
        }
        result = mFile->getStartOffset(&startoffset);
        if (result != FMOD_OK)
        {
            return result;
        }

        result = mFile->setStartOffset(startoffset + filepos);
        if (result != FMOD_OK)
        {
            return result;
        }
    }

    /*
        NOTE: If the tag reading failed for some reason then reset the file pointer and start trying to load the file
              from the beginning. This way we keep any tags we did successfully read and we might also be able to open 
              the file if it's something tolerant like an mp3. Other formats (i.e. Ogg) won't open but there's nothing 
              we can do about that.
    */

	result = mFile->seek(0, SEEK_SET);
    if (result != FMOD_OK)
    {
        return result;
    }

    /*
        Always return FMOD_ERR_FORMAT so the real file format codecs get called later
    */
    return FMOD_ERR_FORMAT;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
FMOD_RESULT CodecTag::closeInternal()
{
    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: ae9766b5 manual
// OFFSET: 0x00914550
FMOD_RESULT CodecTag::readTags()
{
    FMOD_RESULT  result = FMOD_OK;
    int          wavdatapos = 0;
	char         header[16];
    unsigned int filepos, itemsread;

    /*
        Find tags from the end of the file
    */
    wavdatapos = 0;
    for (;;)
    {
        /*
            Look for ID3v1
        */
        result = mFile->seek(wavdatapos - 128, SEEK_END);
        if (result != FMOD_OK)
        {
            break;
        }

        result = mFile->read(header, 1, 3, &itemsread);
        if (result != FMOD_OK)
        {
            return result;
        }
        if (itemsread != 3)
        {
            return FMOD_ERR_FILE_BAD;
        }

        if (!FMOD_strncmp(header, "TAG", 3))                        /* ID3v1 */
        {
            result = readID3v1();
            if (result != FMOD_OK)
            {
                return result;
            }

            wavdatapos -= 128;

            result = mFile->tell(&filepos);
            if (result != FMOD_OK)
            {
                return result;
            }
            if (filepos <= 128)
            {
                break;
            }
        }
        else
        {
            /*
                Look for ID3v2 footer
            */
            result = mFile->seek(wavdatapos - 10, SEEK_END);
            if (result != FMOD_OK)
            {
                if (result == FMOD_ERR_FILE_COULDNOTSEEK)
                {
                    break;  /* possibly netstream, try tags at start of file, we might be able to do it. */
                }
                return result;
            }

	        result = mFile->read(header, 1, 3, &itemsread);
            if (result != FMOD_OK)
            {
                return result;
            }
            if (itemsread != 3)
            {
                return FMOD_ERR_FILE_BAD;
            }

            if (!FMOD_strncmp(header, "3DI", 3))                    /* ID3v2 footer */
            {
                result = readID3v2FromFooter();
                if (result != FMOD_OK)
                {
                    return result;
                }

                result = mFile->tell(&filepos);
                if (result != FMOD_OK)
                {
                    return result;
                }
                wavdatapos = filepos;
            }
            else
            {
                break;
            }
        }
    }

    /*
        Find tags from the start of the file
    */
    wavdatapos = 0;
    result = mFile->seek(wavdatapos, SEEK_SET);
    if (result != FMOD_OK)
    {
        return result;
    }

    for (;;)
    {
	    result = mFile->read(header, 1, 16, &itemsread);
        if (result != FMOD_OK)
        {
            return result;
        }
        if (itemsread != 16)
        {
            return FMOD_ERR_FILE_BAD;
        }

        if (!FMOD_strncmp(header, "TAG", 3))                         /* ID3v1 */
        {
            result = mFile->seek(-13, SEEK_CUR);
            if (result != FMOD_OK)
            {
                return result;
            }

            result = readID3v1();
            if (result != FMOD_OK)
            {
                return result;
            }
        }
        else if (!FMOD_strncmp(header, "ID3", 3))                    /* ID3v2 */
        {
            result = mFile->seek(-13, SEEK_CUR);
            if (result != FMOD_OK)
            {
                return result;
            }

            result = readID3v2();
            if (result != FMOD_OK)
            {
                return result;
            }
        }
#if defined(FMOD_SUPPORT_DSHOW) || defined(FMOD_SUPPORT_ASF)
        else if (!memcmp(header, &ASF_Header_Object, 16))            /* ASF */
        {
            FMOD_GUID *guid = (FMOD_GUID *)header;

            result = readASF();
            if (result != FMOD_OK)
            {
                return result;
            }

            result = mFile->seek(wavdatapos, SEEK_SET);
            if (result != FMOD_OK)
            {
                return result;
            }

            break;
        }
#endif
        else
        {
            result = mFile->seek(wavdatapos, SEEK_SET);
            if (result != FMOD_OK)
            {
                return result;
            }

            break;                                              /* No tag found, leave search loop */
        }

        result = mFile->tell(&filepos);
        if (result != FMOD_OK)
        {
            return result;
        }
        wavdatapos = filepos;
    }

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: ef1058c8 auto
// OFFSET: 0x009134A0
FMOD_RESULT CodecTag::readID3v1()
{
    FMOD_RESULT  result;
    char         value[31], tmp[8];
    unsigned int itemsread;

    FMOD_memset(value, 0, 31);
	result = mFile->read(value, 1, 30, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 30)
    {
        return FMOD_ERR_FILE_BAD;
    }
    if (FMOD_strlen(value))
    {
        metaData(FMOD_TAGTYPE_ID3V1, "TITLE", value, FMOD_strlen(value) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    FMOD_memset(value, 0, 31);
	result = mFile->read(value, 1, 30, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 30)
    {
        return FMOD_ERR_FILE_BAD;
    }
    if (FMOD_strlen(value))
    {
        metaData(FMOD_TAGTYPE_ID3V1, "ARTIST", value, FMOD_strlen(value) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    FMOD_memset(value, 0, 31);
	result = mFile->read(value, 1, 30, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 30)
    {
        return FMOD_ERR_FILE_BAD;
    }
    if (FMOD_strlen(value))
    {
        metaData(FMOD_TAGTYPE_ID3V1, "ALBUM", value, FMOD_strlen(value) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    FMOD_memset(value, 0, 31);
	result = mFile->read(value, 1, 4, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 4)
    {
        return FMOD_ERR_FILE_BAD;
    }
    if (FMOD_strlen(value))
    {
        metaData(FMOD_TAGTYPE_ID3V1, "YEAR", value, FMOD_strlen(value) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    FMOD_memset(value, 0, 31);
	result = mFile->read(value, 1, 30, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 30)
    {
        return FMOD_ERR_FILE_BAD;
    }
    if (FMOD_strlen(value))
    {
        metaData(FMOD_TAGTYPE_ID3V1, "COMMENT", value, FMOD_strlen(value) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    if ((value[28] == 0) && (value[29] != 0))
    {
        /*
            Must be ID3v1.1
        */
        sprintf(tmp, "%d", (int)(value[29] & 0xff));
        metaData(FMOD_TAGTYPE_ID3V1, "TRACK", tmp, FMOD_strlen(tmp) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    FMOD_memset(value, 0, 31);
	result = mFile->read(value, 1, 1, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 1)
    {
        return FMOD_ERR_FILE_BAD;
    }
    sprintf(tmp, "%d", (int)(value[0] & 0xff));
    metaData(FMOD_TAGTYPE_ID3V1, "GENRE", tmp, FMOD_strlen(tmp) + 1, FMOD_TAGDATATYPE_STRING, false);

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: 064982d9 auto
// OFFSET: 0x00913800
FMOD_RESULT CodecTag::readID3v2()
{
    FMOD_RESULT     result;
    unsigned char   size[4], flags;
    unsigned short	version;
    unsigned int    taglen, tagdatalen = 0, tagoff, filepos, itemsread;
    int             wavdatapos;

    result = mFile->tell(&filepos);
    if (result != FMOD_OK)
    {
        return result;
    }
    wavdatapos = filepos;

    result = mFile->read(&version, 1, 2, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 2)
    {
        return FMOD_ERR_FILE_BAD;
    }

	result = mFile->read(&flags, 1, 1, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 1)
    {
        return FMOD_ERR_FILE_BAD;
    }

	result = mFile->read(size, 1, 4, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 4)
    {
        return FMOD_ERR_FILE_BAD;
    }

    taglen = ((unsigned int)size[0] << 21) + ((unsigned int)size[1] << 14) + ((unsigned int)size[2] << 7) + (unsigned int)size[3];

    if (flags & ID3V2_FLAGS_FOOTER)
    {
        taglen += 10;
    }

    wavdatapos += ((taglen + 10) - 3);      /* + 10 for header, - 3 for 'ID3' */
    tagoff = 10;

    do
    {
        unsigned char   tagdataid[5];
        unsigned short  tagdataflags;
        signed char     ascii;

        tagdataid[0] = tagdataid[1] = tagdataid[2] = tagdataid[3] = tagdataid[4] = 0;

        /*
            ID3v2.2.x and lower have a different frame header than ID3v2.3.x and above
        */
        if (version <= 2)
        {
            result = mFile->read(tagdataid, 3, 1, &itemsread);
            if (result != FMOD_OK)
            {
                return result;
            }
            if (itemsread != 1)
            {
                return FMOD_ERR_FILE_BAD;
            }

            result = mFile->read(size, 3, 1, &itemsread);
            if (result != FMOD_OK)
            {
                return result;
            }
            if (itemsread != 1)
            {
                return FMOD_ERR_FILE_BAD;
            }

            tagdatalen = ((unsigned int)size[0] << 16) | ((unsigned int)size[1] << 8) | (unsigned int)size[2];
        }
        else if (version >= 3)
        {
            result = mFile->read(tagdataid, 4, 1, &itemsread);
            if (result != FMOD_OK)
            {
                return result;
            }
            if (itemsread != 1)
            {
                return FMOD_ERR_FILE_BAD;
            }

            result = mFile->read(size, 4, 1, &itemsread);
            if (result != FMOD_OK)
            {
                return result;
            }
            if (itemsread != 1)
            {
                return FMOD_ERR_FILE_BAD;
            }

            result = mFile->read(&tagdataflags, 2, 1, &itemsread);
            if (result != FMOD_OK)
            {
                return result;
            }
            if (itemsread != 1)
            {
                return FMOD_ERR_FILE_BAD;
            }

            tagdatalen = ((unsigned int)size[0] << 24) + ((unsigned int)size[1] << 16) + ((unsigned int)size[2] << 8) + (unsigned int)size[3];
        }

        #define ISASCII(_x) (((_x) >= 32 && (_x) < 128) || ((_x) == 0))

        ascii = (ISASCII(tagdataid[0]) && ISASCII(tagdataid[1]) && ISASCII(tagdataid[2]) && ISASCII(tagdataid[3]));
     
        if (ascii && tagdatalen > 0 && tagdatalen < (1*1024*1024))   /* dont support erroneous or huge tag lengths */
        {
            FMOD_TAGDATATYPE datatype = FMOD_TAGDATATYPE_BINARY;
            char *tagdata;
            int taglen = tagdatalen;

            tagdata = (char *)FMOD_Memory_Alloc(tagdatalen);
            if (!tagdata)
            {
                result = mFile->seek(wavdatapos, SEEK_SET);
                return FMOD_ERR_MEMORY;
            }
            result = mFile->read(tagdata, 1, tagdatalen, &itemsread);
            if (result != FMOD_OK)
            {
                return result;
            }
            if (itemsread != tagdatalen)
            {
                FMOD_Memory_Free(tagdata);
                return result;
            }

            /*
                Parse some common tag types a little to make it easier on the user
            */
            if (tagdataid[0] == 'T')
            {
                switch (tagdata[0])
                {
                    case 0x00 :
                        datatype = FMOD_TAGDATATYPE_STRING;
                        break;
                    case 0x01 :
                        datatype = FMOD_TAGDATATYPE_STRING_UTF16;
                        break;
                    case 0x02 :
                        datatype = FMOD_TAGDATATYPE_STRING_UTF16BE;
                        break;
                    case 0x03 :
                        datatype = FMOD_TAGDATATYPE_STRING_UTF8;
                        break;
                    default :
                        break;
                }

                FMOD_memmove(tagdata, tagdata + 1, tagdatalen - 1);
                tagdata[tagdatalen - 1] = 0;
                taglen--;
            }
            else if (tagdataid[0] == 'W')
            {
                //AJS url tag - but it's enc, desc, text...
            }

            metaData(FMOD_TAGTYPE_ID3V2, (char *)tagdataid, tagdata, taglen, datatype, false);

            FMOD_Memory_Free(tagdata);
        }

        tagoff += (tagdatalen + 10);

    } while (tagoff < taglen);

    result = mFile->seek(wavdatapos, SEEK_SET);
    if (result != FMOD_OK)
    {
        return result;
    }

    return FMOD_OK;
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: 52de6df8 auto
// OFFSET: 0x00913BE0
FMOD_RESULT CodecTag::readID3v2FromFooter()
{
    FMOD_RESULT     result;
    char			size[4];
    unsigned short	version;
    unsigned char	flags;
    int				taglen, pos;
    unsigned int    itemsread, filepos;

    result = mFile->read(&version, 1, 2, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 2)
    {
        return FMOD_ERR_FILE_BAD;
    }

	result = mFile->read(&flags, 1, 1, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 1)
    {
        return FMOD_ERR_FILE_BAD;
    }

	result = mFile->read(size, 1, 4, &itemsread);
    if (result != FMOD_OK)
    {
        return result;
    }
    if (itemsread != 4)
    {
        return FMOD_ERR_FILE_BAD;
    }

    taglen = ((int )size[0] << 21) + ((int )size[1] << 14) + ((int )size[2] << 7) + size[3];

    if (flags & ID3V2_FLAGS_FOOTER)
    {
        taglen += 10;
    }

    result = mFile->seek(-taglen + 3, SEEK_CUR);
    if (result != FMOD_OK)
    {
        return result;
    }

    result = mFile->tell(&filepos);
    if (result != FMOD_OK)
    {
        return result;
    }
    pos = filepos - 3;

    result = readID3v2();
    if (result != FMOD_OK)
    {
        return result;
    }

    result = mFile->seek(pos, SEEK_SET);
    if (result != FMOD_OK)
    {
        return result;
    }

    return FMOD_OK;
}

#ifdef FMOD_SUPPORT_ASF
/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: f87c5c40 manual
// OFFSET: 0x00913CE0
FMOD_RESULT CodecTag::readASF()
{
    FMOD_RESULT  result;
    FMOD_UINT64  offset = 0;
    FMOD_UINT64  size   = -1;
    ASF_CHUNK    chunk;
    unsigned int itemsread, filepos;

	/*
        Read header
    */
    result = mFile->seek(0, SEEK_END);
    if (result != FMOD_OK)
    {
        return result;
    }
    result = mFile->tell(&filepos);
    if (result != FMOD_OK)
    {
        return result;
    }
    size = filepos;
    result = mFile->seek(0, SEEK_SET);
    if (result != FMOD_OK)
    {
        return result;
    }

	/*
        Decode chunks
    */
	do 
	{
        result = mFile->seek((int)offset, SEEK_SET);
        if (result != FMOD_OK)
        {
            return result;
        }

		result = mFile->read(&chunk, 1, sizeof(ASF_CHUNK), &itemsread);
        if ((result != FMOD_OK) || (itemsread != sizeof(ASF_CHUNK)))
        {
            return result;
        }

        #ifdef PLATFORM_ENDIAN_BIG
        chunk.size = FMOD_SWAPENDIAN_QWORD(chunk.size);
        #endif

		/*
            DATA CHUNK
        */
		if (!memcmp(&chunk.guid, &ASF_Header_Object, sizeof(FMOD_GUID)))
		{    
            ASF_CHUNK subchunk;
            ASF_HEADEROBJECT headerobject;
            int count;
            FMOD_UINT64 suboffset;
            
            result = mFile->read(&headerobject, 1, sizeof(headerobject), &itemsread);
            if ((result != FMOD_OK) || (itemsread != sizeof(headerobject)))
            {
                return result;
            }

            suboffset = sizeof(ASF_CHUNK) + sizeof(headerobject);

            for (count=0; count < headerobject.number; count++)
            {
                result = mFile->seek((int)(offset + suboffset), SEEK_SET);
                if (result != FMOD_OK)
                {
                    return result;
                }

        		result = mFile->read(&subchunk, 1, sizeof(ASF_CHUNK), &itemsread);
                if ((result != FMOD_OK) || (itemsread != sizeof(ASF_CHUNK)))
                {
                    return result;
                }

		        if (!memcmp(&subchunk.guid, &ASF_Content_Description_Object, sizeof(FMOD_GUID)))
		        {
                    static char tmp[4098];
                    static char tmp2[4098];
                    int         title_length = 0;
                    int         author_length = 0;
                    int         copyright_length = 0;
                    int         description_length = 0;
                    int         rating_length = 0;

                    result = mFile->read(&title_length, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1))
                    {
                        return result;
                    }
                    result = mFile->read(&author_length, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1))
                    {
                        return result;
                    }
                    result = mFile->read(&copyright_length, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1))
                    {
                        return result;
                    }
                    result = mFile->read(&description_length, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1))
                    {
                        return result;
                    }
                    result = mFile->read(&rating_length, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1))
                    {
                        return result;
                    }

                    if (title_length)
                    {
                        result = mFile->read(tmp, 1, title_length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != (unsigned int)title_length))
                        {
                            return result;
                        }
                        
                        wcstombs(tmp2, (const wchar_t *)tmp, 4096);

                        result = metaData(FMOD_TAGTYPE_ASF, "TITLE", tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                        if (result != FMOD_OK)
                        {
                            return result;
                        }
                    }

                    if (author_length)
                    {
                        result = mFile->read(tmp, 1, author_length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != (unsigned int)author_length))
                        {
                            return result;
                        }

                        wcstombs(tmp2, (const wchar_t *)tmp, 4096);

                        result = metaData(FMOD_TAGTYPE_ASF, "AUTHOR", tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                        if (result != FMOD_OK)
                        {
                            return result;
                        }
                    }

                    if (copyright_length)
                    {
                        result = mFile->read(tmp, 1, copyright_length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != (unsigned int)copyright_length))
                        {
                            return result;
                        }

                        wcstombs(tmp2, (const wchar_t *)tmp, 4096);

                        result = metaData(FMOD_TAGTYPE_ASF, "COPYRIGHT", tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                        if (result != FMOD_OK)
                        {
                            return result;
                        }
                    }

                    if (description_length)
                    {
                        result = mFile->read(tmp, 1, description_length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != (unsigned int)description_length))
                        {
                            return result;
                        }

                        wcstombs(tmp2, (const wchar_t *)tmp, 4096);

                        result = metaData(FMOD_TAGTYPE_ASF, "DESCRIPTION", tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                        if (result != FMOD_OK)
                        {
                            return result;
                        }
                    }

                    if (rating_length)
                    {
                        result = mFile->read(tmp, 1, rating_length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != (unsigned int)rating_length))
                        {
                            return result;
                        }

                        wcstombs(tmp2, (const wchar_t *)tmp, 4096);

                        result = metaData(FMOD_TAGTYPE_ASF, "RATING", tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                        if (result != FMOD_OK)
                        {
                            return result;
                        }
                    }
                }
                else if (!memcmp(&subchunk.guid, &ASF_Extended_Content_Description_Object, sizeof(FMOD_GUID)))
                {
                    unsigned short  descriptorcount;
                    int             count;
                    static char     tmp[4098];
                    static char     tmp2[4098];

                    result = mFile->read(&descriptorcount, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1))
                    {
                        return result;
                    }

                    for (count = 0; count < descriptorcount; count++)
                    {
                        unsigned short  Descriptor_Name_Length;
                        unsigned short  Descriptor_Value_Data_Type;
                        unsigned short  Descriptor_Value_Length;
                        char           *Descriptor_Value;

                        result = mFile->read(&Descriptor_Name_Length, 2, 1, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != 1))
                        {
                            return result;
                        }
                        result = mFile->read(&tmp2, 1, Descriptor_Name_Length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != Descriptor_Name_Length))
                        {
                            return result;
                        }
                        wcstombs(tmp, (const wchar_t *)tmp2, 4096);

                        result = mFile->read(&Descriptor_Value_Data_Type, 2, 1, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != 1))
                        {
                            return result;
                        }
                        result = mFile->read(&Descriptor_Value_Length, 2, 1, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != 1))
                        {
                            return result;
                        }
                        
                        Descriptor_Value = (char *)FMOD_Memory_Calloc(Descriptor_Value_Length);
                        if (!Descriptor_Value)
                        {
                            return FMOD_ERR_MEMORY;
                        }

                        result = mFile->read(Descriptor_Value, 1, Descriptor_Value_Length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != Descriptor_Value_Length))
                        {
                            return result;
                        }

                        switch (Descriptor_Value_Data_Type)
                        {
                            case 0x0000:    /* Unicode String */
                            {
                                wcstombs(tmp2, (const wchar_t *)Descriptor_Value, 4096);

                                result = metaData(FMOD_TAGTYPE_ASF, tmp, tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                                if (result != FMOD_OK)
                                {
                                    return result;
                                }
                                break;
                            }
                            case 0x0002:    /* Bool */
                            {
                                result = metaData(FMOD_TAGTYPE_ASF, tmp, Descriptor_Value, Descriptor_Value_Length, FMOD_TAGDATATYPE_BINARY, false);
                                if (result != FMOD_OK)
                                {
                                    return result;
                                }
                                break;
                            }
                            case 0x0003:    /* DWORD */
                            case 0x0004:    /* QWORD */
                            case 0x0005:    /* WORD */
                            {
                                result = metaData(FMOD_TAGTYPE_ASF, tmp, Descriptor_Value, Descriptor_Value_Length, FMOD_TAGDATATYPE_INT, false);
                                if (result != FMOD_OK)
                                {
                                    return result;
                                }
                                break;
                            }
                            default:
                            {
                                result = metaData(FMOD_TAGTYPE_ASF, tmp, Descriptor_Value, Descriptor_Value_Length, FMOD_TAGDATATYPE_STRING, false);
                                if (result != FMOD_OK)
                                {
                                    return result;
                                }
                                break;
                            }
                        }

                        FMOD_Memory_Free(Descriptor_Value);
                    }
                
                }

                suboffset += subchunk.size;
            }
		}

		offset += chunk.size;

	} while (offset < size && chunk.size);

    return FMOD_OK;
}
#endif

/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: 30a22e53 manual
// OFFSET: 0x00914950
FMOD_RESULT F_CALLBACK CodecTag::openCallback(FMOD_CODEC_STATE *codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO *userexinfo)
{
    CodecTag *tag = (CodecTag *)codec;

    return tag->openInternal(usermode, userexinfo);
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
FMOD_RESULT F_CALLBACK CodecTag::closeCallback(FMOD_CODEC_STATE *codec)
{
    CodecTag *tag = (CodecTag *)codec;

    return tag->closeInternal();
}


/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
FMOD_RESULT F_CALLBACK CodecTag::readCallback(FMOD_CODEC_STATE *codec, void *buffer, unsigned int sizebytes, unsigned int *bytesread)
{
    return FMOD_OK;
}

/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
FMOD_RESULT F_CALLBACK CodecTag::setPositionCallback(FMOD_CODEC_STATE *codec, int subsound, unsigned int position, FMOD_TIMEUNIT postype)
{
    return FMOD_OK;
}

/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
FMOD_RESULT F_CALLBACK CodecTag::soundcreateCallback(FMOD_CODEC_STATE *codec, int subsound, FMOD_SOUND *sound)
{
    return FMOD_OK;
}

}

#endif


