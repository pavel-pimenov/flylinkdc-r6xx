/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_DtsH
#define MediaInfo_DtsH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#ifdef ES
   #undef ES //Solaris defines this somewhere
#endif
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Dts
//***************************************************************************

class File_Dts : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;

    //Constructor/Destructor
    File_Dts();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();
    void Read_Buffer_Unsynched();

    //Buffer - Demux
    #if MEDIAINFO_DEMUX
    bool Demux_UnpacketizeContainer_Test();
    #endif //MEDIAINFO_DEMUX

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Core();
    void Core_XCh(int64u Size);
    void Core_XXCh(int64u Size);
    void Core_X96k(int64u Size);
    void HD();
    void HD_XCh(int64u Size);
    void HD_XXCh(int64u Size);
    void HD_X96k(int64u Size);
    void HD_XLL(int64u Size);
    void HD_XBR(int64u Size);
    void HD_XSA(int64u Size);

    //Buffer
    bool FrameSynchPoint_Test();
    const int8u* Save_Buffer;
    size_t Save_Buffer_Offset;
    size_t Save_Buffer_Size;

    //Temp
    std::vector<ZenLib::int32u> Asset_Sizes;
    Ztring Profile;
    int32u Original_Size;
    int32u HD_size;
    int16u Primary_Frame_Byte_Size;
    int16u Number_Of_PCM_Sample_Blocks;
    int16u HD_SpeakerActivityMask;
    int8u  channel_arrangement;
    int8u  channel_arrangement_XCh;
    int8u  sample_frequency;
    int8u  sample_frequency_X96k;
    int8u  bit_rate;
    int8u  lfe_effects;
    int8u  bits_per_sample;
    int8u  ExtensionAudioDescriptor;
    int8u  HD_BitResolution;
    int8u  HD_BitResolution_Real;
    int8u  HD_MaximumSampleRate;
    int8u  HD_MaximumSampleRate_Real;
    int8u  HD_TotalNumberChannels;
    int8u  HD_ExSSFrameDurationCode;
    bool   ExtendedCoding;
    bool   Word;
    bool   BigEndian;
    bool   ES;
    bool   Core_Exists;
    bool   One2OneMapChannels2Speakers;
    enum   presence
    {
        presence_Core_Core,
        presence_Core_XXCh,
        presence_Core_X96,
        presence_Core_XCh,
        presence_Extended_Core,
        presence_Extended_XBR,
        presence_Extended_XXCh,
        presence_Extended_X96,
        presence_Extended_LBR,
        presence_Extended_XLL,
        presence_Extended_XLL_X,
        presence_Extended_XLL_IMAX,
        presence_Max
    };
    std::bitset<presence_Max> Presence;
    enum   data
    {
        Profiles,
        Channels,
        ChannelPositions,
        ChannelPositions2,
        ChannelLayout,
        BitDepth,
        SamplingRate,
        BitRate,
        BitRate_Mode,
        Compression_Mode,
        data_Max,
    };
    ZtringList Data[data_Max];
    int8u  Core_Core_AMODE;
    int8u  Core_Core_LFF;
    int8u  Core_XCh_AMODE;
    int8u  Core_XXCh_nuNumChSetsInXXCh;
    size_t Extension_XLL_X_No;
    size_t Extension_XLL_X_Yes;
    size_t Extension_XLL_IMAXX_No;
    size_t Extension_XLL_IMAXX_Yes;

    //Helpers
    float64 BitRate_Get(bool WithHD=false);
    void    Streams_Fill_Extension();
    void    Streams_Fill_Core_ES();
    void    Streams_Fill_Core();
};

} //NameSpace

#endif
