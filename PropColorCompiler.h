//========= Copyright Satjo Interactive 2025, All rights reserved. ============//
//
//      Оригинальный код написан: Tirmiks
//      Отдельные благодарности: ugo_zapad, Ambiabstract, MyCbEH
//
//=============================================================================//

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <windows.h>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

#pragma pack(push, 1)

struct Vector
{
    float x, y, z;
};

struct StudioHdr
{
    int id;
    int version;
    int checksum;
    char name[64];
    int length;
    Vector eyeposition;
    Vector illumposition;
    Vector hull_min;
    Vector hull_max;
    Vector view_bbmin;
    Vector view_bbmax;
    int flags;
    int numbones;
    int boneindex;
    int numbonecontrollers;
    int bonecontrollerindex;
    int numhitboxsets;
    int hitboxsetindex;
    int numlocalanim;
    int localanimindex;
    int numlocalseq;
    int localseqindex;
    int activitylistversion;
    int eventsindexed;
    int numtextures;
    int textureindex;
    int numcdtextures;
    int cdtextureindex;
    int numskinref;
    int numskinfamilies;
    int skinindex;
    int numbodyparts;
    int bodypartindex;
    int numlocalattachments;
    int localattachmentindex;
    int numlocalnodes;
    int localnodeindex;
    int localnodenameindex;
    int numflexdesc;
    int flexdescindex;
    int numflexcontrollers;
    int flexcontrollerindex;
    int numflexrules;
    int flexruleindex;
    int numikchains;
    int ikchainindex;
    int nummouths;
    int mouthindex;
    int numlocalposeparameters;
    int localposeparamindex;
    int surfacepropindex;
    int keyvalueindex;
    int keyvaluesize;
    int numlocalikautoplaylocks;
    int localikautoplaylockindex;
    float mass;
    int contents;
    int numincludemodels;
    int includemodelindex;
    int virtualModel;
    int szanimblocknameindex;
    int numanimblocks;
    int animblockindex;
    int animblockModel;
    int bonetablebynameindex;
    int pVertexBase;
    int pIndexBase;
    unsigned char constdirectionallightdot;
    unsigned char rootLOD;
    unsigned char numAllowedRootLODs;
    unsigned char unused1;
    int unused4;
    int numflexcontrollerui;
    int flexcontrolleruiindex;
    float flVertAnimFixedPointScale;
    int unused3;
    int studiohdr2index;
    int unused2;
};

struct StudioHdr2
{
    int numsrcbonetransform;
    int srcbonetransformindex;
    int illumpositionattachmentindex;
    float flMaxEyeDeflection;
    int linearboneindex;
    int sznameindex;
    int m_nBoneFlexDriverCount;
    int m_nBoneFlexDriverIndex;
    int unused[56];
};

struct Texture
{
    int sznameindex;
    int flags;
    int used;
    int unused1;
    int material;
    int clientmaterial;
    int unused[10];
};

struct PostCompilePropInfo {
    std::string originalModel;
    std::string coloredModel;
    std::string rendercolor;
    int skin;
    size_t startPos;
    size_t endPos;
};

#pragma pack(pop)

class MDLFile
{
private:
    std::vector<unsigned char> fileData;
    std::vector<unsigned char> newFileData;
    StudioHdr* header;
    StudioHdr2* header2;

    std::vector<std::string> textureNames;
    std::vector<std::string> textureDirs;
    std::vector<std::vector<short>> skinFamilies;
    std::string surfaceProp;
    std::string keyValues;

    int alreadyEditedOffset;

public:
    bool Load(const std::string& filename);
    bool Save(const std::string& filename);
    bool AddMaterial(const std::string& materialPath);
    bool AddMaterialWithSkin(const std::string& materialPath);
    bool AddMultipleMaterialsWithSkins(const std::vector<std::string>& materialPaths);

    std::vector<std::string> GetTextureNames() const { return textureNames; }
    std::vector<std::string> GetTextureDirs() const { return textureDirs; }

private:
    void ParseTextures();
    void ParseTextureDirs();
    void ParseSkinFamilies();
    void ParseSurfaceProp();
    void ParseKeyValues();
    std::string ReadString(int offset);
    int FindAlreadyEditedMarker();
    void RebuildMDLFile(const std::vector<std::string>& newTextureNames,
        const std::vector<std::vector<short>>& newSkinFamilies,
        const std::vector<std::string>& newTextureDirs);
};

class HammerCompiler
{
private:
    std::string vmfPath;
    std::string gameDir;

    std::vector<PostCompilePropInfo> postCompileProps;

    struct EntityInfo {
        std::string model;
        std::string rendercolor;
        int skin = 0;
        size_t startPos;
        size_t endPos;
    };

    struct ModelColorInfo
    {
        std::set<std::string> colors;
        std::vector<EntityInfo*> entities;
    };

    std::vector<std::string> createdFiles;
    std::vector<EntityInfo> entities;
    std::map<std::string, ModelColorInfo> modelData;

public:
    HammerCompiler(const std::string& vmfPath, const std::string& gameDir);
    bool ProcessVMF();
    bool ProcessModels();
    bool UpdateVMF();

    void AddCreatedFile(const std::string& filePath);
    bool SaveCreatedFilesList();
    bool DeleteCreatedFiles();
    bool ParseVMFForPostCompile();
    bool AddFilesToBSP(const std::string& bspPath, const std::string& gameDir);
    bool RestoreVMFAfterPostCompile();
    std::string RestoreEntityContent(const std::string& entityContent);

private:
    bool ParseVMF();
    bool CopyModelFiles(const std::string& originalModelPath);
    std::string CreateVMTContent(const std::string& originalContent, const std::string& newBaseTexture, const std::string& color);
    std::string ReadFileContent(const std::string& path);
    bool WriteFileContent(const std::string& path, const std::string& content);
};