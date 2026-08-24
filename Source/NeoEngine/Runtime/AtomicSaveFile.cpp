#include "AtomicSaveFile.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace NeoEngine {
namespace {
bool ValidSlot(std::string_view slot){return !slot.empty()&&slot.size()<=48U&&std::all_of(slot.begin(),slot.end(),[](unsigned char c){return std::isalnum(c)||c=='-'||c=='_';});}
}
bool AtomicSaveFile::Write(const std::filesystem::path& root,std::string_view slot,const std::vector<uint8_t>& bytes,AtomicSaveFileError& error){if(!ValidSlot(slot)){error=AtomicSaveFileError::InvalidSlot;return false;}if(bytes.size()>kMaxBytes){error=AtomicSaveFileError::PayloadLimit;return false;}std::error_code ec;std::filesystem::create_directories(root,ec);if(ec){error=AtomicSaveFileError::CreateDirectory;return false;}const std::filesystem::path finalPath=root/(std::string(slot)+".sav"),tempPath=root/(std::string(slot)+".tmp");{std::ofstream stream(tempPath,std::ios::binary|std::ios::trunc);if(!stream){error=AtomicSaveFileError::OpenWrite;return false;}if(!bytes.empty())stream.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));stream.flush();if(!stream){std::filesystem::remove(tempPath,ec);error=AtomicSaveFileError::WriteFailure;return false;}}std::filesystem::rename(tempPath,finalPath,ec);if(ec){std::filesystem::remove(tempPath,ec);error=AtomicSaveFileError::RenameFailure;return false;}error=AtomicSaveFileError::None;return true;}
bool AtomicSaveFile::Read(const std::filesystem::path& root,std::string_view slot,std::vector<uint8_t>& bytes,AtomicSaveFileError& error){if(!ValidSlot(slot)){error=AtomicSaveFileError::InvalidSlot;return false;}const std::filesystem::path path=root/(std::string(slot)+".sav");std::error_code ec;if(!std::filesystem::exists(path,ec)||ec){error=AtomicSaveFileError::Missing;return false;}const uintmax_t size=std::filesystem::file_size(path,ec);if(ec){error=AtomicSaveFileError::OpenRead;return false;}if(size>kMaxBytes){error=AtomicSaveFileError::PayloadLimit;return false;}std::ifstream stream(path,std::ios::binary);if(!stream){error=AtomicSaveFileError::OpenRead;return false;}std::vector<uint8_t> parsed(static_cast<size_t>(size));if(size>0U)stream.read(reinterpret_cast<char*>(parsed.data()),static_cast<std::streamsize>(size));if(!stream&&size>0U){error=AtomicSaveFileError::ReadFailure;return false;}bytes=std::move(parsed);error=AtomicSaveFileError::None;return true;}
} // namespace NeoEngine
