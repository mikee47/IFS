#include "include/IFS/File.h"

namespace IFS
{
constexpr OpenFlags File::ReadOnly;
constexpr OpenFlags File::WriteOnly;
constexpr OpenFlags File::ReadWrite;
constexpr OpenFlags File::Create;
constexpr OpenFlags File::Append;
constexpr OpenFlags File::Truncate;
constexpr OpenFlags File::CreateNewAlways;

} // namespace IFS
