//========= Copyright Satjo Interactive 2025, All rights reserved. ============//
//
//      Оригинальный код написан: Tirmiks
//      Отдельные благодарности: ugo_zapad, Ambiabstract, MyCbEH
//
//=============================================================================//

#include "PropColorCompiler.h"
#include "colored_cout.h"

namespace fs = std::filesystem;

bool MDLFile::Load(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cout << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    fileData.resize(size);
    file.read(reinterpret_cast<char*>(fileData.data()), size);
    file.close();

    header = reinterpret_cast<StudioHdr*>(fileData.data());

    int headerId = header->id;
    if (headerId != 'TSDI' && headerId != 'IDST')
    {
        std::cout << "Error: Invalid MDL file signature!" << std::endl;
        return false;
    }

    ParseTextures();
    ParseTextureDirs();
    ParseSkinFamilies();
    ParseSurfaceProp();
    ParseKeyValues();

    return true;
}

bool MDLFile::Save(const std::string& filename)
{
    if (newFileData.empty())
    {
        std::cout << "Error: No changes to save" << std::endl;
        return false;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cout << "Error: Cannot create file " << filename << std::endl;
        return false;
    }

    file.write(reinterpret_cast<char*>(newFileData.data()), newFileData.size());
    file.close();

    std::cout << "Successfully saved: " << filename << std::endl;
    return true;
}

std::string MDLFile::ReadString(int offset)
{
    if (offset < 0 || offset >= fileData.size())
        return "";

    const char* str = reinterpret_cast<const char*>(fileData.data() + offset);
    return std::string(str);
}

int MDLFile::FindAlreadyEditedMarker()
{
    return 0;
}

void MDLFile::ParseTextures()
{
    textureNames.clear();

    if (header->numtextures > 0 && header->textureindex > 0)
    {
        Texture* textures = reinterpret_cast<Texture*>(fileData.data() + header->textureindex);

        for (int i = 0; i < header->numtextures; i++)
        {
            std::string name = ReadString(header->textureindex + i * sizeof(Texture) + textures[i].sznameindex);
            textureNames.push_back(name);
        }
    }
}

void MDLFile::ParseTextureDirs()
{
    textureDirs.clear();

    if (header->numcdtextures > 0 && header->cdtextureindex > 0)
    {
        int* dirOffsets = reinterpret_cast<int*>(fileData.data() + header->cdtextureindex);

        for (int i = 0; i < header->numcdtextures; i++)
        {
            std::string dir = ReadString(dirOffsets[i]);
            textureDirs.push_back(dir);
        }
    }
}

void MDLFile::ParseSkinFamilies()
{
    skinFamilies.clear();

    if (header->numskinfamilies > 0 && header->skinindex > 0)
    {
        short* skins = reinterpret_cast<short*>(fileData.data() + header->skinindex);
        int index = 0;

        for (int i = 0; i < header->numskinfamilies; i++)
        {
            std::vector<short> family;
            for (int j = 0; j < header->numskinref; j++)
            {
                family.push_back(skins[index++]);
            }
            skinFamilies.push_back(family);
        }
    }
}

void MDLFile::ParseSurfaceProp()
{
    if (header->surfacepropindex > 0)
    {
        surfaceProp = ReadString(header->surfacepropindex);
    }
}

void MDLFile::ParseKeyValues()
{
    if (header->keyvalueindex > 0 && header->keyvaluesize > 0)
    {
        keyValues = std::string(reinterpret_cast<char*>(fileData.data() + header->keyvalueindex), header->keyvaluesize);
    }
}

bool MDLFile::AddMaterial(const std::string& materialPath)
{
    std::vector<std::string> newTextureNames = textureNames;
    newTextureNames.push_back(materialPath);

    std::vector<std::string> newTextureDirs = textureDirs;
    size_t lastSlash = materialPath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        std::string dir = materialPath.substr(0, lastSlash);
        if (std::find(newTextureDirs.begin(), newTextureDirs.end(), dir) == newTextureDirs.end())
        {
            newTextureDirs.push_back(dir);
        }
    }

    RebuildMDLFile(newTextureNames, skinFamilies, newTextureDirs);
    return true;
}

bool MDLFile::AddMaterialWithSkin(const std::string& materialPath)
{
    std::vector<std::string> newTextureNames = textureNames;
    newTextureNames.push_back(materialPath);
    short newMaterialIndex = static_cast<short>(newTextureNames.size() - 1);

    std::vector<std::string> newTextureDirs = textureDirs;
    size_t lastSlash = materialPath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        std::string dir = materialPath.substr(0, lastSlash);
        if (std::find(newTextureDirs.begin(), newTextureDirs.end(), dir) == newTextureDirs.end())
        {
            newTextureDirs.push_back(dir);
        }
    }

    std::vector<std::vector<short>> newSkinFamilies = skinFamilies;
    std::vector<short> newSkinFamily;
    for (int i = 0; i < header->numskinref; i++)
    {
        newSkinFamily.push_back(newMaterialIndex);
    }
    newSkinFamilies.push_back(newSkinFamily);

    RebuildMDLFile(newTextureNames, newSkinFamilies, newTextureDirs);
    return true;
}

bool MDLFile::AddMultipleMaterialsWithSkins(const std::vector<std::string>& materialPaths)
{
    std::vector<std::string> newTextureNames = textureNames;
    std::vector<std::string> newTextureDirs = textureDirs;

    for (const auto& materialPath : materialPaths)
    {
        newTextureNames.push_back(materialPath);

        size_t lastSlash = materialPath.find_last_of("/\\");
        if (lastSlash != std::string::npos)
        {
            std::string dir = materialPath.substr(0, lastSlash);
            if (std::find(newTextureDirs.begin(), newTextureDirs.end(), dir) == newTextureDirs.end())
            {
                newTextureDirs.push_back(dir);
            }
        }
    }

    std::vector<std::vector<short>> newSkinFamilies = skinFamilies;

    for (size_t i = 0; i < materialPaths.size(); i++)
    {
        short newMaterialIndex = static_cast<short>(textureNames.size() + i);
        std::vector<short> newSkinFamily;
        for (int j = 0; j < header->numskinref; j++)
        {
            newSkinFamily.push_back(newMaterialIndex);
        }
        newSkinFamilies.push_back(newSkinFamily);
    }

    RebuildMDLFile(newTextureNames, newSkinFamilies, newTextureDirs);
    return true;
}

void MDLFile::RebuildMDLFile(const std::vector<std::string>& newTextureNames,
    const std::vector<std::vector<short>>& newSkinFamilies,
    const std::vector<std::string>& newTextureDirs)
{
    int startOffset = static_cast<int>(fileData.size());
    int newSize = startOffset;

    newSize += static_cast<int>(newTextureNames.size() * sizeof(Texture));
    for (const auto& name : newTextureNames)
    {
        newSize += static_cast<int>(name.length() + 1);
    }

    newSize += static_cast<int>(newTextureDirs.size() * sizeof(int));
    for (const auto& dir : newTextureDirs)
    {
        newSize += static_cast<int>(dir.length() + 1);
    }

    int totalSkinIndices = 0;
    for (const auto& family : newSkinFamilies)
    {
        totalSkinIndices += static_cast<int>(family.size());
    }
    newSize += totalSkinIndices * static_cast<int>(sizeof(short));

    newSize += static_cast<int>(surfaceProp.length() + 1);
    newSize += static_cast<int>(keyValues.length() + 1);

    newFileData.resize(newSize);
    memcpy(newFileData.data(), fileData.data(), fileData.size());

    int currentOffset = startOffset;

    int textureStructOffset = currentOffset;
    int textureNameOffset = textureStructOffset + static_cast<int>(newTextureNames.size() * sizeof(Texture));

    int textureDirOffset = textureNameOffset;
    for (const auto& name : newTextureNames)
    {
        textureDirOffset += static_cast<int>(name.length() + 1);
    }

    int skinDataOffset = textureDirOffset + static_cast<int>(newTextureDirs.size() * sizeof(int));
    for (const auto& dir : newTextureDirs)
    {
        skinDataOffset += static_cast<int>(dir.length() + 1);
    }

    int surfacePropOffset = skinDataOffset + totalSkinIndices * static_cast<int>(sizeof(short));
    int keyValuesOffset = surfacePropOffset + static_cast<int>(surfaceProp.length() + 1);

    currentOffset = textureStructOffset;
    int nameCurrentOffset = 0;
    for (size_t i = 0; i < newTextureNames.size(); i++)
    {
        Texture tex;
        memset(&tex, 0, sizeof(Texture));
        tex.sznameindex = textureNameOffset + nameCurrentOffset - (textureStructOffset + static_cast<int>(i * sizeof(Texture)));
        tex.flags = 0;
        tex.used = 1;

        memcpy(newFileData.data() + currentOffset, &tex, sizeof(Texture));
        currentOffset += static_cast<int>(sizeof(Texture));
        nameCurrentOffset += static_cast<int>(newTextureNames[i].length() + 1);
    }

    currentOffset = textureNameOffset;
    for (const auto& name : newTextureNames)
    {
        memcpy(newFileData.data() + currentOffset, name.c_str(), name.length());
        currentOffset += static_cast<int>(name.length() + 1);
    }

    int dirNameOffset = textureDirOffset + static_cast<int>(newTextureDirs.size() * sizeof(int));
    currentOffset = textureDirOffset;
    int dirNameCurrentOffset = 0;
    for (size_t i = 0; i < newTextureDirs.size(); i++)
    {
        int dirOffset = dirNameOffset + dirNameCurrentOffset;
        memcpy(newFileData.data() + currentOffset, &dirOffset, sizeof(int));
        currentOffset += static_cast<int>(sizeof(int));
        dirNameCurrentOffset += static_cast<int>(newTextureDirs[i].length() + 1);
    }

    currentOffset = dirNameOffset;
    for (const auto& dir : newTextureDirs)
    {
        memcpy(newFileData.data() + currentOffset, dir.c_str(), dir.length());
        currentOffset += static_cast<int>(dir.length() + 1);
    }

    currentOffset = skinDataOffset;
    for (const auto& family : newSkinFamilies)
    {
        for (short texIndex : family)
        {
            memcpy(newFileData.data() + currentOffset, &texIndex, sizeof(short));
            currentOffset += static_cast<int>(sizeof(short));
        }
    }

    currentOffset = surfacePropOffset;
    memcpy(newFileData.data() + currentOffset, surfaceProp.c_str(), surfaceProp.length());
    currentOffset += static_cast<int>(surfaceProp.length() + 1);

    currentOffset = keyValuesOffset;
    memcpy(newFileData.data() + currentOffset, keyValues.c_str(), keyValues.length());
    currentOffset += static_cast<int>(keyValues.length() + 1);

    StudioHdr* newHeader = reinterpret_cast<StudioHdr*>(newFileData.data());
    newHeader->length = newSize;
    newHeader->numtextures = static_cast<int>(newTextureNames.size());
    newHeader->textureindex = textureStructOffset;
    newHeader->numcdtextures = static_cast<int>(newTextureDirs.size());
    newHeader->cdtextureindex = textureDirOffset;
    newHeader->skinindex = skinDataOffset;
    newHeader->numskinfamilies = static_cast<int>(newSkinFamilies.size());

    if (!newSkinFamilies.empty())
    {
        newHeader->numskinref = static_cast<int>(newSkinFamilies[0].size());
    }

    newHeader->surfacepropindex = surfacePropOffset;
    newHeader->keyvalueindex = keyValuesOffset;
    newHeader->keyvaluesize = static_cast<int>(keyValues.length());
}

HammerCompiler::HammerCompiler(const std::string& vmfPath, const std::string& gameDir)
    : vmfPath(vmfPath), gameDir(gameDir) {
}

bool HammerCompiler::ProcessVMF()
{
    std::cout << "Processing VMF file: " << vmfPath << std::endl;

    if (!ParseVMF())
    {
        std::cout << "Failed to parse VMF file" << std::endl;
        return false;
    }

    if (!ProcessModels())
    {
        std::cout << "Failed to process models" << std::endl;
        return false;
    }

    if (!UpdateVMF())
    {
        std::cout << "Failed to update VMF file" << std::endl;
        return false;
    }

    std::cout << "VMF processing completed successfully!" << std::endl;
    return true;
}

bool HammerCompiler::ParseVMF()
{
    std::string content = ReadFileContent(vmfPath);
    if (content.empty())
    {
        return false;
    }

    size_t pos = 0;
    while (pos < content.length())
    {
        size_t entityStart = content.find("entity", pos);
        if (entityStart == std::string::npos)
            break;

        size_t braceStart = content.find("{", entityStart);
        if (braceStart == std::string::npos)
            break;

        int braceCount = 1;
        size_t currentPos = braceStart + 1;

        while (braceCount > 0 && currentPos < content.length())
        {
            if (content[currentPos] == '{')
                braceCount++;
            else if (content[currentPos] == '}')
                braceCount--;
            currentPos++;
        }

        if (braceCount == 0)
        {
            std::string entityContent = content.substr(entityStart, currentPos - entityStart);

            EntityInfo entity;
            entity.startPos = entityStart;
            entity.endPos = currentPos;

            size_t classnamePos = entityContent.find("\"classname\"");
            if (classnamePos != std::string::npos)
            {
                size_t valueStart = entityContent.find("\"", classnamePos + 12);
                size_t valueEnd = entityContent.find("\"", valueStart + 1);
                if (valueStart != std::string::npos && valueEnd != std::string::npos)
                {
                    std::string classname = entityContent.substr(valueStart + 1, valueEnd - valueStart - 1);
                    if (classname == "prop_static")
                    {
                        size_t modelPos = entityContent.find("\"model\"");
                        if (modelPos != std::string::npos)
                        {
                            size_t valueStart = entityContent.find("\"", modelPos + 8);
                            size_t valueEnd = entityContent.find("\"", valueStart + 1);
                            if (valueStart != std::string::npos && valueEnd != std::string::npos)
                            {
                                entity.model = entityContent.substr(valueStart + 1, valueEnd - valueStart - 1);
                            }
                        }

                        size_t colorPos = entityContent.find("\"rendercolor\"");
                        if (colorPos != std::string::npos)
                        {
                            size_t valueStart = entityContent.find("\"", colorPos + 14);
                            size_t valueEnd = entityContent.find("\"", valueStart + 1);
                            if (valueStart != std::string::npos && valueEnd != std::string::npos)
                            {
                                entity.rendercolor = entityContent.substr(valueStart + 1, valueEnd - valueStart - 1);
                            }
                        }

                        size_t skinPos = entityContent.find("\"skin\"");
                        if (skinPos != std::string::npos) {
                            size_t valueStart = entityContent.find("\"", skinPos + 6);
                            size_t valueEnd = entityContent.find("\"", valueStart + 1);
                            if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                                entity.skin = std::stoi(entityContent.substr(valueStart + 1, valueEnd - valueStart - 1));
                            }
                        }

                        if (!entity.model.empty() && !entity.rendercolor.empty() && entity.rendercolor != "255 255 255")
                        {
                            entities.push_back(entity);
                            modelData[entity.model].colors.insert(entity.rendercolor);
                            modelData[entity.model].entities.push_back(&entities.back());
                        }
                    }
                }
            }

            pos = currentPos;
        }
        else
        {
            break;
        }
    }

    std::cout << "Found " << modelData.size() << " models with custom colors" << std::endl;
    return true;
}

bool HammerCompiler::ProcessModels()
{
    std::map<std::string, std::map<std::string, std::set<std::string>>> textureUsage;

    for (const auto& [modelPath, colorInfo] : modelData)
    {
        std::string fullModelPath = gameDir + "/" + modelPath;

        if (!CopyModelFiles(modelPath))
        {
            std::cout << "Failed to copy model files for: " << modelPath << std::endl;
            continue;
        }

        MDLFile mdl;
        if (mdl.Load(fullModelPath))
        {
            std::vector<std::string> textureNames = mdl.GetTextureNames();
            if (!textureNames.empty())
            {
                std::string baseTexture = textureNames[0];
                textureUsage[baseTexture][modelPath] = colorInfo.colors;
            }
        }
    }

    for (const auto& [texturePath, modelColors] : textureUsage)
    {
        std::cout << "Processing texture: " << texturePath << " used by " << modelColors.size() << " models" << std::endl;

        // Читаем оригинальный VMT файл
        std::string baseTexturePath = texturePath;
        size_t textureDotPos = baseTexturePath.find_last_of('.');
        if (textureDotPos != std::string::npos)
        {
            baseTexturePath = baseTexturePath.substr(0, textureDotPos);
        }

        std::string originalVmtPath = gameDir + "/materials/" + baseTexturePath + ".vmt";
        std::string originalVmtContent = ReadFileContent(originalVmtPath);

        if (originalVmtContent.empty())
        {
            std::cout << "Warning: Original VMT file not found or empty: " << originalVmtPath << std::endl;
            continue;
        }

        for (const auto& [modelPath, colors] : modelColors)
        {
            fs::path modelFilePath(modelPath);
            std::string modelName = modelFilePath.stem().string();
            std::string uniqueSuffix = "_" + modelName;

            std::cout << "Creating materials for model: " << modelPath << " with " << colors.size() << " colors" << std::endl;

            std::vector<std::string> materialPaths;
            std::vector<std::string> orderedColors(colors.begin(), colors.end());

            for (size_t i = 0; i < orderedColors.size(); i++)
            {
                std::string newBaseTexture = baseTexturePath + uniqueSuffix + "_color" + std::to_string(i + 1);
                std::string vmtContent = CreateVMTContent(originalVmtContent, newBaseTexture, orderedColors[i]);

                std::string vmtPath = gameDir + "/materials/" + newBaseTexture + ".vmt";
                std::cout << "Creating VMT: " << vmtPath << " with color " << orderedColors[i] << std::endl;

                if (!WriteFileContent(vmtPath, vmtContent))
                {
                    std::cout << "Failed to write VMT file: " << vmtPath << std::endl;
                    continue;
                }

                materialPaths.push_back(newBaseTexture);
                AddCreatedFile(vmtPath);
            }

            std::string fullModelPath = gameDir + "/" + modelPath;
            fs::path modelFilePathObj(fullModelPath);
            std::string coloredModelPath = (modelFilePathObj.parent_path() / (modelFilePathObj.stem().string() + "_colored.mdl")).string();

            MDLFile mdl;
            if (mdl.Load(coloredModelPath))
            {
                if (!mdl.AddMultipleMaterialsWithSkins(materialPaths))
                {
                    std::cout << "Failed to add materials to model: " << coloredModelPath << std::endl;
                }
                else
                {
                    mdl.Save(coloredModelPath);
                }
            }
        }
    }

    return true;
}

bool HammerCompiler::CopyModelFiles(const std::string& originalModelPath)
{
    std::string fullModelPath = gameDir + "/" + originalModelPath;
    std::cout << "Looking for model: " << fullModelPath << std::endl;

    if (!fs::exists(fullModelPath))
    {
        std::cout << "Model file not found: " << fullModelPath << std::endl;
        return false;
    }

    fs::path modelPath(fullModelPath);
    std::string baseName = modelPath.stem().string();
    std::string extension = modelPath.extension().string();
    fs::path parentDir = modelPath.parent_path();

    std::string coloredBaseName = baseName + "_colored";
    fs::path coloredModelPath = parentDir / (coloredBaseName + extension);

    std::cout << "Base name: " << baseName << std::endl;
    std::cout << "Colored base name: " << coloredBaseName << std::endl;
    std::cout << "Colored model path: " << coloredModelPath << std::endl;

    if (fs::exists(coloredModelPath))
    {
        std::cout << "Colored model already exists: " << coloredModelPath << std::endl;
        AddCreatedFile(coloredModelPath.string());
        return true;
    }

    try
    {
        fs::copy_file(fullModelPath, coloredModelPath, fs::copy_options::overwrite_existing);
        std::cout << "Copied MDL file to: " << coloredModelPath << std::endl;
        AddCreatedFile(coloredModelPath.string());
    }
    catch (const std::exception& e)
    {
        std::cout << "Failed to copy MDL file: " << e.what() << std::endl;
        return false;
    }

    std::vector<std::string> extensions = { ".dx90.vtx", ".vvd", ".phy" }; // добавить остальные, если потребуется (пропы из CS:GO имеют лишь это)
    for (const auto& ext : extensions)
    {
        fs::path originalFile = parentDir / (baseName + ext);
        fs::path coloredFile = parentDir / (coloredBaseName + ext);

        std::cout << "Looking for: " << originalFile << std::endl;

        if (fs::exists(originalFile))
        {
            try
            {
                fs::copy_file(originalFile, coloredFile, fs::copy_options::overwrite_existing);
                std::cout << "Copied file: " << originalFile << " to " << coloredFile << std::endl;
                AddCreatedFile(coloredFile.string());
            }
            catch (const std::exception& e)
            {
                std::cout << "Failed to copy file: " << originalFile << " - " << e.what() << std::endl;
            }
        }
        else
        {
            std::cout << "File not found (this may be normal): " << originalFile << std::endl;
        }
    }

    return true;
}

std::string HammerCompiler::CreateVMTContent(const std::string& originalContent, const std::string& newBaseTexture, const std::string& color)
{
    std::istringstream iss(originalContent);
    std::ostringstream oss;
    std::string line;

    bool hasColor2 = false;
    bool hasBlendTint = false;
    bool hasBaseTexture = false;

    // Извлекаем оригинальное имя текстуры из оригинального VMT
    std::string originalBaseTexture;
    std::istringstream issTemp(originalContent);
    std::string tempLine;
    while (std::getline(issTemp, tempLine)) {
        std::string trimmedLine = tempLine;
        trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t"));
        trimmedLine.erase(trimmedLine.find_last_not_of(" \t") + 1);

        if (trimmedLine.find("\"$basetexture\"") == 0) {
            size_t firstQuote = trimmedLine.find('"', 14);
            size_t secondQuote = trimmedLine.find('"', firstQuote + 1);
            if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
                originalBaseTexture = trimmedLine.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                break;
            }
        }
    }

    if (originalBaseTexture.empty()) {
        originalBaseTexture = newBaseTexture;
        size_t colorPos = originalBaseTexture.find("_color");
        if (colorPos != std::string::npos) {
            originalBaseTexture = originalBaseTexture.substr(0, colorPos);
        }
        size_t modelSuffixPos = originalBaseTexture.find_last_of('_');
        if (modelSuffixPos != std::string::npos) {
            std::string possibleSuffix = originalBaseTexture.substr(modelSuffixPos);
            if (possibleSuffix.find("hr_") == std::string::npos &&
                possibleSuffix.find("lr_") == std::string::npos) {
                originalBaseTexture = originalBaseTexture.substr(0, modelSuffixPos);
            }
        }
    }

    iss.clear();
    iss.str(originalContent);

    while (std::getline(iss, line))
    {
        std::string trimmedLine = line;
        trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t"));
        trimmedLine.erase(trimmedLine.find_last_not_of(" \t") + 1);

        if (trimmedLine.find("\"$basetexture\"") == 0) {
            size_t firstQuote = trimmedLine.find('"', 14);
            size_t secondQuote = trimmedLine.find('"', firstQuote + 1);
            if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
                line = "\t\"$basetexture\" \"" + originalBaseTexture + "\"";
            }
            hasBaseTexture = true;
        }

        else if (trimmedLine.find("\"$color2\"") == 0)
        {
            size_t firstBrace = trimmedLine.find('{');
            size_t secondBrace = trimmedLine.find('}');
            if (firstBrace != std::string::npos && secondBrace != std::string::npos)
            {
                line = "\t\"$color2\" \"{" + color + "}\"";
            }
            hasColor2 = true;
        }

        else if (trimmedLine.find("\"$blendtintbybasealpha\"") == 0)
        {
            size_t firstQuote = trimmedLine.find('"', 24);
            size_t secondQuote = trimmedLine.find('"', firstQuote + 1);
            if (firstQuote != std::string::npos && secondQuote != std::string::npos)
            {
                line = "\t\"$blendtintbybasealpha\" \"1\"";
            }
            hasBlendTint = true;
        }

        oss << line << "\n";
    }

    std::string result = oss.str();

    if (!hasBaseTexture) {
        size_t lastBrace = result.rfind('}');
        if (lastBrace != std::string::npos) {
            result.insert(lastBrace, "\n\t\"$basetexture\" \"" + originalBaseTexture + "\"");
        }
    }

    if (!hasColor2)
    {
        size_t lastBrace = result.rfind('}');
        if (lastBrace != std::string::npos)
        {
            result.insert(lastBrace, "\n\t\"$color2\" \"{" + color + "}\"");
        }
    }

    if (!hasBlendTint)
    {
        size_t lastBrace = result.rfind('}');
        if (lastBrace != std::string::npos)
        {
            result.insert(lastBrace, "\n\t\"$blendtintbybasealpha\" \"1\"");
        }
    }

    return result;
}

bool HammerCompiler::UpdateVMF() {
    std::cout << "Updating VMF file..." << std::endl;

    std::string content = ReadFileContent(vmfPath);
    if (content.empty())
    {
        return false;
    }

    std::cout << "Original VMF content size: " << content.length() << " bytes" << std::endl;

    std::string tempOutputPath = vmfPath + ".tmp_colored";

    std::vector<EntityInfo*> sortedEntities;
    for (auto& entity : entities)
    {
        sortedEntities.push_back(&entity);
    }

    std::sort(sortedEntities.begin(), sortedEntities.end(),
        [](const EntityInfo* a, const EntityInfo* b) { return a->startPos > b->startPos; });

    std::cout << "Processing " << sortedEntities.size() << " entities for update" << std::endl;

    bool hasChanges = false;

    for (auto* entity : sortedEntities)
    {
        if (modelData.find(entity->model) == modelData.end())
            continue;

        const auto& colorInfo = modelData[entity->model];
        std::string entityContent = content.substr(entity->startPos, entity->endPos - entity->startPos);

        std::cout << "Updating entity with model: " << entity->model << " and color: " << entity->rendercolor << std::endl;

        std::vector<std::string> orderedColors(colorInfo.colors.begin(), colorInfo.colors.end());

        int colorIndex = -1;
        for (size_t i = 0; i < orderedColors.size(); i++)
        {
            if (orderedColors[i] == entity->rendercolor)
            {
                colorIndex = static_cast<int>(i) + 1;
                break;
            }
        }

        if (colorIndex == -1)
        {
            std::cout << "Error: Could not find color index for: " << entity->rendercolor << std::endl;
            continue;
        }

        std::cout << "Color index: " << colorIndex << std::endl;

        size_t modelPos = entityContent.find("\"model\"");
        if (modelPos != std::string::npos)
        {
            size_t valueStart = entityContent.find("\"", modelPos + 8);
            size_t valueEnd = entityContent.find("\"", valueStart + 1);
            if (valueStart != std::string::npos && valueEnd != std::string::npos)
            {
                std::string newModelPath = entity->model;
                size_t dotPos = newModelPath.find_last_of('.');
                newModelPath = newModelPath.substr(0, dotPos) + "_colored.mdl";

                entityContent.replace(valueStart + 1, valueEnd - valueStart - 1, newModelPath);
                std::cout << "Updated model path to: " << newModelPath << std::endl;
            }
        }

        size_t skinPos = entityContent.find("\"skin\"");
        if (skinPos != std::string::npos)
        {
            size_t valueStart = entityContent.find("\"", skinPos + 7);
            size_t valueEnd = entityContent.find("\"", valueStart + 1);
            if (valueStart != std::string::npos && valueEnd != std::string::npos)
            {
                entityContent.replace(valueStart + 1, valueEnd - valueStart - 1, std::to_string(colorIndex));
                std::cout << "Updated skin to: " << colorIndex << std::endl;
            }
        }
        else
        {
            size_t insertPos = entityContent.find("\"model\"");
            if (insertPos != std::string::npos)
            {
                size_t lineEnd = entityContent.find("\n", insertPos);
                if (lineEnd != std::string::npos)
                {
                    entityContent.insert(lineEnd + 1, "\t\"skin\" \"" + std::to_string(colorIndex) + "\"\n");
                    std::cout << "Added skin parameter: " << colorIndex << std::endl;
                }
            }
        }

        size_t colorPos = entityContent.find("\"rendercolor\"");
        if (colorPos != std::string::npos)
        {
            size_t valueStart = entityContent.find("\"", colorPos + 14);
            size_t valueEnd = entityContent.find("\"", valueStart + 1);
            std::string originalColor;
            if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                originalColor = entityContent.substr(valueStart + 1, valueEnd - valueStart - 1);
            }

            size_t lineStart = entityContent.rfind("\n", colorPos);
            if (lineStart == std::string::npos) lineStart = 0;
            size_t lineEnd = entityContent.find("\n", colorPos);
            if (lineEnd != std::string::npos)
            {
                entityContent.erase(lineStart, lineEnd - lineStart + 1);
                std::cout << "Removed rendercolor parameter" << std::endl;
            }

            if (!originalColor.empty()) {
                size_t insertPos = entityContent.find("\"model\"");
                if (insertPos != std::string::npos)
                {
                    size_t lineEnd = entityContent.find("\n", insertPos);
                    if (lineEnd != std::string::npos)
                    {
                        entityContent.insert(lineEnd + 1, "\t\"$color\" \"" + originalColor + "\"\n");
                        std::cout << "Added $color marker for post-compile: " << originalColor << std::endl;
                    }
                }
            }
        }

        content.replace(entity->startPos, entity->endPos - entity->startPos, entityContent);
        hasChanges = true;
    }

    if (!hasChanges)
    {
        std::cout << "No changes to save in VMF" << std::endl;
        return true;
    }

    std::cout << "Writing updated VMF content..." << std::endl;

    if (!WriteFileContent(tempOutputPath, content))
    {
        std::cout << "Failed to write temporary VMF file" << std::endl;
        return false;
    }

    try
    {
        fs::remove(vmfPath);
        fs::rename(tempOutputPath, vmfPath);
        std::cout << "Successfully updated VMF file: " << vmfPath << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Failed to replace original VMF file: " << e.what() << std::endl;
        return false;
    }

    return true;
}

std::string HammerCompiler::ReadFileContent(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cout << "Failed to open file: " << path << std::endl;
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return content;
}

bool HammerCompiler::WriteFileContent(const std::string& path, const std::string& content)
{
    fs::path filePath(path);
    fs::create_directories(filePath.parent_path());

    std::ofstream file(path);
    if (!file.is_open())
    {
        std::cout << "Failed to create file: " << path << std::endl;
        return false;
    }

    file << content;
    file.close();

    std::cout << "Successfully wrote file: " << path << " (" << content.length() << " bytes)" << std::endl;
    return true;
}

void HammerCompiler::AddCreatedFile(const std::string& filePath) 
{
    if (!filePath.empty() && fs::exists(filePath)) {
        createdFiles.push_back(filePath);
    }
}

bool HammerCompiler::SaveCreatedFilesList()
{
    std::string listPath = this->vmfPath + ".created_files.txt";
    std::ofstream file(listPath);

    if (!file.is_open()) {
        std::cout << "Failed to create files list: " << listPath << std::endl;
        return false;
    }

    for (const auto& filePath : createdFiles) {
        file << filePath << "\n";
    }

    file.close();
    std::cout << "Saved created files list: " << listPath << " (" << createdFiles.size() << " files)" << std::endl;
    return true;
}

bool HammerCompiler::DeleteCreatedFiles() 
{
    std::string listPath = this->vmfPath + ".created_files.txt";

    if (!fs::exists(listPath)) {
        std::cout << "No created files list found: " << listPath << std::endl;
        return true;
    }

    std::ifstream file(listPath);
    if (!file.is_open()) {
        std::cout << "Failed to open created files list: " << listPath << std::endl;
        return false;
    }

    std::vector<std::string> filesToDelete;
    std::string filePath;

    while (std::getline(file, filePath)) {
        if (!filePath.empty() && fs::exists(filePath)) {
            filesToDelete.push_back(filePath);
        }
    }
    file.close();

    int deletedCount = 0;
    for (const auto& path : filesToDelete) {
        try {
            if (fs::exists(path)) {
                fs::remove(path);
                std::cout << "Deleted created file: " << path << std::endl;
                deletedCount++;
            }
        }
        catch (const std::exception& e) {
            std::cout << "Failed to delete " << path << ": " << e.what() << std::endl;
        }
    }

    try {
        if (fs::exists(listPath)) {
            fs::remove(listPath);
        }
    }
    catch (...) {}

    std::cout << "Deleted " << deletedCount << " created files" << std::endl;
    return true;
}

// BSPZIP:
// https://developer.valvesoftware.com/wiki/BSPZIP
//
bool HammerCompiler::AddFilesToBSP(const std::string& bspPath, const std::string& gameDir) {
    std::cout << "Adding files to BSP using BSPZIP: " << bspPath << std::endl;

    char modulePath[MAX_PATH];
    GetModuleFileNameA(NULL, modulePath, MAX_PATH);
    fs::path exePath(modulePath);
    fs::path exeDir = exePath.parent_path();
    fs::path bspzipPath = exeDir / "bspzip.exe";

    if (!fs::exists(bspzipPath)) {
        std::cout << "Error: bspzip.exe not found in: " << bspzipPath << std::endl;
        return false;
    }

    std::cout << "Found BSPZIP: " << bspzipPath << std::endl;

    fs::path fileListPath = fs::temp_directory_path() / "bsp_files_list.txt";
    std::ofstream fileList(fileListPath);

    if (!fileList.is_open()) {
        std::cout << "Failed to create file list: " << fileListPath << std::endl;
        return false;
    }

    std::set<std::string> addedFiles;
    int totalCount = 0;

    for (const auto& entity : entities) {
        if (entity.model.find("_colored.mdl") != std::string::npos) 
        {
            std::string modelFullPath = gameDir + "/" + entity.model;
            if (fs::exists(modelFullPath)) 
            {
                std::string relativePath = entity.model;

                if (addedFiles.find(relativePath) == addedFiles.end()) 
                {
                    fileList << relativePath << "\n";
                    fileList << modelFullPath << "\n";
                    addedFiles.insert(relativePath);
                    totalCount++;
                    std::cout << "Added to list: " << relativePath << " -> " << modelFullPath << std::endl;
                }
            }

            std::string basePath = entity.model.substr(0, entity.model.find_last_of('.'));
            std::vector<std::string> extensions = { ".vvd", ".dx90.vtx", ".phy" };

            for (const auto& ext : extensions) {
                std::string relatedFile = basePath + ext;
                std::string relatedFullPath = gameDir + "/" + relatedFile;

                if (fs::exists(relatedFullPath)) {
                    std::string relativePath = relatedFile;

                    if (addedFiles.find(relativePath) == addedFiles.end()) {
                        fileList << relativePath << "\n";
                        fileList << relatedFullPath << "\n";
                        addedFiles.insert(relativePath);
                        totalCount++;
                        std::cout << "Added to list: " << relativePath << " -> " << relatedFullPath << std::endl;
                    }
                }
            }

            std::string fullModelPath = gameDir + "/" + entity.model;
            MDLFile mdl;
            if (mdl.Load(fullModelPath)) {
                auto textureNames = mdl.GetTextureNames();
                for (const auto& texture : textureNames) {
                    if (texture.find("_color") != std::string::npos) {
                        std::string vmtPath = "materials/" + texture + ".vmt";
                        std::string vmtFullPath = gameDir + "/" + vmtPath;

                        if (fs::exists(vmtFullPath)) {
                            std::string relativePath = "materials/" + texture + ".vmt";

                            if (addedFiles.find(relativePath) == addedFiles.end()) {
                                fileList << relativePath << "\n";
                                fileList << vmtFullPath << "\n";
                                addedFiles.insert(relativePath);
                                totalCount++;
                                std::cout << "Added to list: " << relativePath << " -> " << vmtFullPath << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }

    fileList.close();

    if (totalCount == 0) {
        std::cout << "No files to add to BSP" << std::endl;
        fs::remove(fileListPath);
        return true;
    }

    std::cout << "Created file list with " << totalCount << " unique files: " << fileListPath << std::endl;

    std::ifstream checkFile(fileListPath);
    std::string line1, line2;
    int lineNum = 0;
    while (std::getline(checkFile, line1)) {
        if (!std::getline(checkFile, line2)) {
            std::cout << "Error: File list has odd number of lines at line " << lineNum + 1 << std::endl;
            fs::remove(fileListPath);
            return false;
        }
        lineNum += 2;
        if (line1.empty() || line2.empty()) {
            std::cout << "Error: Empty line in file list at line " << lineNum << std::endl;
            fs::remove(fileListPath);
            return false;
        }
    }
    checkFile.close();

    std::string tempBspPath = bspPath + ".tmp";

    std::string command = "cmd /c \"\"" + bspzipPath.string() + "\" -addlist \"" + bspPath + "\" \"" + fileListPath.string() + "\" \"" + tempBspPath + "\"\"";

    std::cout << "Executing BSPZIP command..." << std::endl;

    int result = std::system(command.c_str());

    fs::remove(fileListPath);

    if (result != 0) {
        std::cout << "BSPZIP execution failed with error code: " << result << std::endl;
        if (fs::exists(tempBspPath)) {
            fs::remove(tempBspPath);
        }
        return false;
    }

    if (!fs::exists(tempBspPath)) {
        std::cout << "Error: Temporary BSP file was not created: " << tempBspPath << std::endl;
        return false;
    }

    try {
        std::string backupBspPath = bspPath + ".backup";
        if (fs::exists(bspPath)) {
            fs::copy_file(bspPath, backupBspPath, fs::copy_options::overwrite_existing);
            std::cout << "Created backup: " << backupBspPath << std::endl;
        }

        fs::remove(bspPath);
        fs::rename(tempBspPath, bspPath);
        std::cout << "Successfully updated BSP with " << totalCount << " files" << std::endl;

        if (fs::exists(backupBspPath)) {
            fs::remove(backupBspPath);
        }

    }
    catch (const std::exception& e) {
        std::cout << "Failed to replace original BSP: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool HammerCompiler::ParseVMFForPostCompile() {
    std::string content = ReadFileContent(vmfPath);
    if (content.empty()) {
        return false;
    }

    entities.clear();
    modelData.clear();

    size_t pos = 0;
    while (pos < content.length()) {
        size_t entityStart = content.find("entity", pos);
        if (entityStart == std::string::npos)
            break;

        size_t braceStart = content.find("{", entityStart);
        if (braceStart == std::string::npos)
            break;

        int braceCount = 1;
        size_t currentPos = braceStart + 1;

        while (braceCount > 0 && currentPos < content.length()) {
            if (content[currentPos] == '{')
                braceCount++;
            else if (content[currentPos] == '}')
                braceCount--;
            currentPos++;
        }

        if (braceCount == 0) {
            std::string entityContent = content.substr(entityStart, currentPos - entityStart);

            EntityInfo entity;
            entity.startPos = entityStart;
            entity.endPos = currentPos;

            size_t classnamePos = entityContent.find("\"classname\"");
            if (classnamePos != std::string::npos) {
                size_t valueStart = entityContent.find("\"", classnamePos + 12);
                size_t valueEnd = entityContent.find("\"", valueStart + 1);
                if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                    std::string classname = entityContent.substr(valueStart + 1, valueEnd - valueStart - 1);
                    if (classname == "prop_static") {
                        size_t modelPos = entityContent.find("\"model\"");
                        if (modelPos != std::string::npos) {
                            size_t valueStart = entityContent.find("\"", modelPos + 8);
                            size_t valueEnd = entityContent.find("\"", valueStart + 1);
                            if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                                entity.model = entityContent.substr(valueStart + 1, valueEnd - valueStart - 1);
                            }
                        }

                        size_t colorMarkerPos = entityContent.find("\"$color\"");
                        if (colorMarkerPos != std::string::npos) {
                            size_t valueStart = entityContent.find("\"", colorMarkerPos + 8);
                            size_t valueEnd = entityContent.find("\"", valueStart + 1);
                            if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                                entity.rendercolor = entityContent.substr(valueStart + 1, valueEnd - valueStart - 1);
                            }
                        }

                        size_t skinPos = entityContent.find("\"skin\"");
                        if (skinPos != std::string::npos) {
                            size_t valueStart = entityContent.find("\"", skinPos + 6);
                            size_t valueEnd = entityContent.find("\"", valueStart + 1);
                            if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                                entity.skin = std::stoi(entityContent.substr(valueStart + 1, valueEnd - valueStart - 1));
                            }
                        }

                        if (!entity.model.empty() && entity.model.find("_colored.mdl") != std::string::npos) {
                            entities.push_back(entity);
                            if (!entity.rendercolor.empty()) {
                                modelData[entity.model].colors.insert(entity.rendercolor);
                            }
                            modelData[entity.model].entities.push_back(&entities.back());
                        }
                    }
                }
            }

            pos = currentPos;
        }
        else {
            break;
        }
    }

    std::cout << "Found " << modelData.size() << " colored models for post-compile restoration" << std::endl;
    return true;
}

bool HammerCompiler::RestoreVMFAfterPostCompile() {
    std::cout << "Restoring original VMF after post-compile..." << std::endl;

    std::string content = ReadFileContent(vmfPath);
    if (content.empty()) {
        return false;
    }

    bool hasChanges = false;
    int restoredCount = 0;

    size_t pos = 0;
    while (pos < content.length()) {
        size_t entityStart = content.find("entity", pos);
        if (entityStart == std::string::npos) break;

        size_t braceStart = content.find("{", entityStart);
        if (braceStart == std::string::npos) break;

        int braceCount = 1;
        size_t currentPos = braceStart + 1;

        while (braceCount > 0 && currentPos < content.length()) {
            if (content[currentPos] == '{') braceCount++;
            else if (content[currentPos] == '}') braceCount--;
            currentPos++;
        }

        if (braceCount == 0) {
            std::string entityContent = content.substr(entityStart, currentPos - entityStart);

            if (entityContent.find("_colored.mdl") != std::string::npos) {
                std::string restoredEntity = RestoreEntityContent(entityContent);
                if (restoredEntity != entityContent) {
                    content.replace(entityStart, currentPos - entityStart, restoredEntity);
                    hasChanges = true;
                    restoredCount++;
                    std::cout << "Restored entity with colored model" << std::endl;
                }
            }

            pos = currentPos;
        }
        else {
            break;
        }
    }

    size_t coloredPos = 0;
    int globalReplaceCount = 0;
    while ((coloredPos = content.find("_colored.mdl", coloredPos)) != std::string::npos) {
        size_t quoteBefore = content.rfind('"', coloredPos);
        size_t quoteAfter = content.find('"', coloredPos);

        if (quoteBefore != std::string::npos && quoteAfter != std::string::npos) {
            content.replace(coloredPos, 12, ".mdl");
            globalReplaceCount++;
            coloredPos += 4;
        }
        else {
            coloredPos += 12;
        }
    }

    if (globalReplaceCount > 0) {
        hasChanges = true;
        std::cout << "Globally replaced " << globalReplaceCount << " remaining _colored.mdl references" << std::endl;
    }

    size_t skinPos = 0;
    int skinRemoveCount = 0;
    while ((skinPos = content.find("\"skin\"", skinPos)) != std::string::npos) {
        size_t lineStart = content.rfind('\n', skinPos);
        if (lineStart == std::string::npos) lineStart = 0;
        size_t lineEnd = content.find('\n', skinPos);
        if (lineEnd == std::string::npos) lineEnd = content.length();

        std::string line = content.substr(lineStart, lineEnd - lineStart);
        if (line.find("\"skin\"") != std::string::npos && line.find("\"") != std::string::npos) {
            content.erase(lineStart, lineEnd - lineStart);
            skinRemoveCount++;
        }
        else {
            skinPos = lineEnd;
        }
    }

    if (skinRemoveCount > 0) {
        hasChanges = true;
        std::cout << "Removed " << skinRemoveCount << " remaining skin parameters" << std::endl;
    }

    size_t globalColorPos = 0;
    int globalColorReplaceCount = 0;
    while ((globalColorPos = content.find("\"$color\"", globalColorPos)) != std::string::npos) {
        content.replace(globalColorPos, 8, "\"rendercolor\"");
        globalColorReplaceCount++;
        globalColorPos += 12;
    }

    if (globalColorReplaceCount > 0) {
        hasChanges = true;
        std::cout << "Globally replaced " << globalColorReplaceCount << " remaining $color markers" << std::endl;
    }

    if (hasChanges) {
        if (!WriteFileContent(vmfPath, content)) {
            return false;
        }
        std::cout << "Successfully restored VMF: " << restoredCount << " entities restored, "
            << globalReplaceCount << " global replacements, "
            << skinRemoveCount << " skins removed, "
            << globalColorReplaceCount << " $color markers replaced" << std::endl;
    }
    else {
        std::cout << "No changes to restore in VMF" << std::endl;
    }

    return hasChanges;
}

std::string HammerCompiler::RestoreEntityContent(const std::string& entityContent) {
    std::string result = entityContent;

    size_t modelPos = result.find("\"model\"");
    if (modelPos != std::string::npos) {
        size_t valueStart = result.find("\"", modelPos + 8);
        size_t valueEnd = result.find("\"", valueStart + 1);
        if (valueStart != std::string::npos && valueEnd != std::string::npos) {
            std::string modelPath = result.substr(valueStart + 1, valueEnd - valueStart - 1);
            size_t coloredPos = modelPath.find("_colored.mdl");
            if (coloredPos != std::string::npos) {
                std::string originalModel = modelPath.substr(0, coloredPos) + ".mdl";
                result.replace(valueStart + 1, valueEnd - valueStart - 1, originalModel);
            }
        }
    }

    size_t skinPos = result.find("\"skin\"");
    while (skinPos != std::string::npos) {
        size_t lineStart = result.rfind("\n", skinPos);
        if (lineStart == std::string::npos) lineStart = 0;
        size_t lineEnd = result.find("\n", skinPos);
        if (lineEnd != std::string::npos) {
            result.erase(lineStart, lineEnd - lineStart + 1);
        }
        else {
            result.erase(lineStart);
        }
        skinPos = result.find("\"skin\"", skinPos);
    }

    size_t markerPos = result.find("\"$color\"");
    if (markerPos != std::string::npos) {
        result.replace(markerPos, 8, "\"rendercolor\"");
    }

    return result;
}

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    std::cout << clr::cyan << "Satjo Interactive - PropColorCompiler.exe (Nov 28 2025)\n";
    std::cout << clr::cyan << "This program is created by Tirmiks and is provided AS IS.\n";

    if (argc == 4 && std::string(argv[1]) == "-postcompile") 
    {
        std::string vmfFile = argv[2];
        std::string gameDir = argv[3];

        std::cout << clr::white << "\nPost-compilation started..." << std::endl;
        std::cout << "VMF File: " << vmfFile << std::endl;
        std::cout << "Game Directory: " << gameDir << std::endl;

        if (!fs::exists(vmfFile))
        {
            std::cout << "Error: VMF file not found: " << vmfFile << std::endl;
            return 1;
        }

        if (!fs::exists(gameDir))
        {
            std::cout << "Error: Game directory not found: " << gameDir << std::endl;
            return 1;
        }

        HammerCompiler compiler(vmfFile, gameDir);

        std::string bspPath = vmfFile;
        size_t dotPos = bspPath.find_last_of('.');
        if (dotPos != std::string::npos) {
            bspPath = bspPath.substr(0, dotPos) + ".bsp";
        }

        if (!fs::exists(bspPath)) {
            std::cout << "BSP file not found: " << bspPath << std::endl;
            return 1;
        }

        std::cout << "BSP File: " << bspPath << std::endl;

        if (!compiler.ParseVMFForPostCompile()) {
            std::cout << "Failed to parse VMF for post-compile" << std::endl;
            return 1;
        }

        if (!compiler.AddFilesToBSP(bspPath, gameDir)) {
            std::cout << "Failed to add files to BSP" << std::endl;
            return 1;
        }

        if (!compiler.RestoreVMFAfterPostCompile()) {
            std::cout << "Failed to restore VMF" << std::endl;
            return 1;
        }

        if (!compiler.DeleteCreatedFiles()) {
            std::cout << "Warning: Failed to delete some created files" << std::endl;
        }

        std::cout << clr::green << "Post-compilation completed successfully!" << std::endl;
        return 0;
    }
    else if (argc == 3) {
        std::string vmfFile = argv[1];
        std::string gameDir = argv[2];

        std::cout << clr::white << "VMF File: " << vmfFile << std::endl;
        std::cout << "Game Directory: " << gameDir << std::endl;

        if (!fs::exists(vmfFile))
        {
            std::cout << "Error: VMF file not found: " << vmfFile << std::endl;
            return 1;
        }

        if (!fs::exists(gameDir))
        {
            std::cout << "Error: Game directory not found: " << gameDir << std::endl;
            return 1;
        }

        std::cout << "Starting compilation process..." << std::endl;

        HammerCompiler compiler(vmfFile, gameDir);
        if (!compiler.ProcessVMF())
        {
            std::cout << "Failed to process VMF file" << std::endl;
            return 1;
        }

        if (!compiler.SaveCreatedFilesList()) {
            std::cout << "Warning: Failed to save created files list" << std::endl;
        }

        std::cout << clr::green << "\nCompilation completed successfully!" << std::endl;
        std::cout << "Now compile your map in Hammer, then run this program with -postcompile parameter!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << clr::white << "\nProp Static Color Compiler for Hammer++" << std::endl;
        std::cout << "Usage: " << argv[0] << " <vmf_file> <game_dir>" << std::endl;
        std::cout << "Post-compile: " << argv[0] << " -postcompile <vmf_file> <game_dir>" << std::endl;
        std::cout << "Example: " << argv[0] << " C:\\mapsrc\\map.vmf C:\\game\\mission_borealis" << std::endl;
        std::cout << "Post-compile example: " << argv[0] << " -postcompile C:\\mapsrc\\map.vmf C:\\game\\mission_borealis" << std::endl;
        std::cout << "\nSet these parameters in Hammer++ for first compilation: $path\\$file.$ext $gamedir" << std::endl;
        std::cout << "\nSet these parameters in Hammer++ for post-compilation: -postcompile $path\\$file.$ext $gamedir" << std::endl;
        return 1;
    }
}