/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about ADM files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_AdmH
#define MediaInfo_File_AdmH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <map>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class file_adm_private;

//***************************************************************************
// Class File_Adm
//***************************************************************************

class File_Adm : public File__Analyze
{
public :
    //In
    string MuxingMode;
    void chna_Add(int32u Index, const string& TrackUID);
    void* chna_Move();
    void chna_Move(File_Adm*);

    //Constructor/Destructor
    File_Adm();
    ~File_Adm();

private :
    //Streams management
    void Streams_Fill();

    //Buffer - File header
    bool FileHeader_Begin();

    //Temp
    file_adm_private* File_Adm_Private;
};

} //NameSpace

#endif

