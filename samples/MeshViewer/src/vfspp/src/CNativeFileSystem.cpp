//
//  CNativeFileSystem.cpp
//  vfspp
//
//  Created by Yevgeniy Logachev on 6/23/16.
//
//

#include "CNativeFileSystem.h"
#include <dirent.h>
#include <fstream>
#include "CNativeFile.h"
#include "CStringUtilsVFS.h"
#include <sys/stat.h>

using namespace vfspp;

// *****************************************************************************
// Constants
// *****************************************************************************

const uint64_t kChunkSize = 1024;
struct SDir : public DIR {};

// *****************************************************************************
// Public Methods
// *****************************************************************************

CNativeFileSystem::CNativeFileSystem(const std::string& basePath)
: m_basePath(basePath)
, m_IsInitialized(false)
{
    if (!CStringUtils::EndsWith(m_basePath, "/"))
    {
        m_basePath += "/";
    }
}

CNativeFileSystem::~CNativeFileSystem()
{

}

void CNativeFileSystem::Initialize()
{
    if (m_IsInitialized)
    {
        return;
    }
    
    SDir *dir = static_cast<SDir*>(opendir(basePath().c_str()));
    if (dir)
    {
        BuildFilelist(dir, basePath(), m_FileList);
        m_IsInitialized = true;
        
        closedir(dir);
    }
}

void CNativeFileSystem::Shutdown()
{
    m_basePath = "";
    m_FileList.clear();
    m_IsInitialized = false;
}


bool CNativeFileSystem::IsInitialized() const
{
    return m_IsInitialized;
}


const std::string& CNativeFileSystem::basePath() const
{
    return m_basePath;
}


const IFileSystem::TFileList& CNativeFileSystem::FileList() const
{
    return m_FileList;
}


bool CNativeFileSystem::isReadOnly() const
{
    if (!IsInitialized())
    {
        return true;
    }
    
    struct stat fileStat;
    if (stat(basePath().c_str(), &fileStat) < 0) {
        return false;
    }
    return (fileStat.st_mode & S_IWUSR);
}


IFilePtr CNativeFileSystem::openFile(const CFileInfo& filePath, int mode)
{
    CFileInfo fileInfo(basePath(), filePath.absolutePath(), false);
    IFilePtr file = FindFile(fileInfo);
    bool isExists = (file != nullptr);
    if (!isExists)
    {
        mode |= IFile::Truncate;
        file.reset(new CNativeFile(fileInfo));
    }
    file->Open(mode);
    
    if (!isExists && file->IsOpened())
    {
        m_FileList.insert(file);
    }
    
    return file;
}


void CNativeFileSystem::closeFile(IFilePtr file)
{
    if (file)
    {
        file->Close();
    }
}


bool CNativeFileSystem::createFile(const CFileInfo& filePath)
{
    bool result = false;
    if (!isReadOnly() && !isFileExists(filePath))
    {
        CFileInfo fileInfo(basePath(), filePath.absolutePath(), false);
        IFilePtr file = openFile(filePath, IFile::Out | IFile::Truncate);
        if (file)
        {
            result = true;
            file->Close();
        }
    }
    else
    {
        result = true;
    }
    
    return result;
}


bool CNativeFileSystem::removeFile(const CFileInfo& filePath)
{
    bool result = true;
    
    IFilePtr file = FindFile(filePath);
    if (!isReadOnly() && file)
    {
        CFileInfo fileInfo(basePath(), file->FileInfo().absolutePath(), false);
        if (remove(fileInfo.absolutePath().c_str()))
        {
            m_FileList.erase(file);
        }
    }
    
    return result;
}

bool CNativeFileSystem::copyFile(const CFileInfo& src, const CFileInfo& dest)
{
    bool result = false;
    if (!isReadOnly())
    {
        IFilePtr fromFile = FindFile(src);
        IFilePtr toFile = openFile(dest, IFile::Out);
        
        if (fromFile && toFile)
        {
            uint64_t size = kChunkSize;
            std::vector<uint8_t> buff((size_t)size);
            do
            {
                fromFile->Read(buff.data(), kChunkSize);
                toFile->Write(buff.data(), size);
            }
            while (size == kChunkSize);
            
            result = true;
        }
    }
    
    return result;
}


bool CNativeFileSystem::RenameFile(const CFileInfo& src, const CFileInfo& dest)
{
    if (!isReadOnly())
    {
        return false;
    }
    
    bool result = false;
    
    IFilePtr fromFile = FindFile(src);
    IFilePtr toFile = FindFile(dest);
    if (fromFile && !toFile)
    {
        CFileInfo toInfo(basePath(), dest.absolutePath(), false);
        
        if (rename(fromFile->FileInfo().absolutePath().c_str(), toInfo.absolutePath().c_str()))
        {
            m_FileList.erase(fromFile);
            toFile = openFile(dest, IFile::In);
            if (toFile)
            {
                result = true;
                toFile->Close();
            }
        }
    }
    
    return result;
}


bool CNativeFileSystem::isFileExists(const CFileInfo& filePath) const
{
    return (FindFile(basePath() + filePath.absolutePath()) != nullptr);
}


bool CNativeFileSystem::IsFile(const CFileInfo& filePath) const
{
    IFilePtr file = FindFile(filePath);
    if (file)
    {
        return !file->FileInfo().IsDir();
    }
    
    return false;
}


bool CNativeFileSystem::IsDir(const CFileInfo& dirPath) const
{
    IFilePtr file = FindFile(dirPath);
    if (file)
    {
        return file->FileInfo().IsDir();
    }
    
    return false;
}

// *****************************************************************************
// Protected Methods
// *****************************************************************************

// *****************************************************************************
// Private Methods
// *****************************************************************************

IFilePtr CNativeFileSystem::FindFile(const CFileInfo& fileInfo) const
{
    TFileList::const_iterator it = std::find_if(m_FileList.begin(), m_FileList.end(), [&](IFilePtr file)
    {
        return file->FileInfo() == fileInfo;
    });
    
    if (it != m_FileList.end())
    {
        return *it;
    }
    
    return nullptr;
}

void CNativeFileSystem::BuildFilelist(SDir* dir,
                                      std::string basePath,
                                      TFileList& outFileList)
{
    if (!CStringUtils::EndsWith(basePath, "/"))
    {
        basePath += "/";
    }
    
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
        std::string filename = ent->d_name;
        std::string filepath = basePath + filename;
        SDir *childDir = static_cast<SDir*>(opendir(filepath.c_str()));
        bool isDotOrDotDot = CStringUtils::EndsWith(filename, ".") && childDir;
        if (childDir && !isDotOrDotDot)
        {
            filename += "/";
        }
        
        CFileInfo fileInfo(basePath, filename, childDir != NULL);
        if (!FindFile(fileInfo))
        {
            IFilePtr file(new CNativeFile(fileInfo));
            outFileList.insert(file);
        }
        
        if (childDir)
        {
            if (!isDotOrDotDot)
            {
                BuildFilelist(childDir, (childDir ? filepath : basePath), outFileList);
            }
            closedir(childDir);
        }
    }
}
