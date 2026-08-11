// ======================================================================
// StringTableTool.cpp
// Minimal .stf editor: import/export a tab-delimited text file <-> .stf
// ======================================================================

#include "FirstStringTableTool.h"

#include "sharedFoundation/SetupSharedFoundation.h"
#include "sharedThread/SetupSharedThread.h"
#include "sharedDebug/SetupSharedDebug.h"
#include "sharedFile/SetupSharedFile.h"

#include "fileInterface/StdioFile.h"
#include "LocalizedStringTableReaderWriter.h"
#include "UnicodeUtils.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

// ======================================================================

static void usage()
{
printf("StringTableTool - edit .stf localized string tables\n\n");
printf("Export to editable text:\n");
printf("  StringTableTool -e -i <input.stf> -o <output.txt>\n\n");
printf("Import from text (adds/overwrites keys, writes new .stf):\n");
printf("  StringTableTool -m -i <input.stf> -t <text.txt> -o <output.stf>\n\n");
printf("Text format: one entry per line, tab-separated:\n");
printf("  key<TAB>value\n");
}

// ----------------------------------------------------------------------

static int doExport(const char * stfPath, const char * txtPath)
{
StdioFileFactory factory;
LocalizedStringTableRW * table = LocalizedStringTableRW::loadRW(factory, stfPath);
if (!table)
{
printf("ERROR: could not load %s\n", stfPath);
return 1;
}

std::ofstream out(txtPath, std::ios::binary);
if (!out.is_open())
{
printf("ERROR: could not open %s for writing\n", txtPath);
delete table;
return 1;
}

LocalizedStringTableRW::NameMap_t & nameMap = table->getNameMap();
for (LocalizedStringTableRW::NameMap_t::const_iterator it = nameMap.begin(); it != nameMap.end(); ++it)
{
const std::string & key = it->first;
LocalizedString * ls = table->getLocalizedString(it->second);
if (!ls)
continue;
std::string narrow = Unicode::wideToNarrow(ls->getString());
out << key << "\t" << narrow << "\n";
}

out.close();
printf("SUCCESS: exported %s\n", txtPath);
delete table;
return 0;
}

// ----------------------------------------------------------------------

static int doImport(const char * stfPath, const char * txtPath, const char * outPath)
{
StdioFileFactory factory;
LocalizedStringTableRW * table = LocalizedStringTableRW::loadRW(factory, stfPath);
if (!table)
{
printf("Input .stf not found or unreadable, starting a new table: %s\n", stfPath);
table = new LocalizedStringTableRW(std::string(stfPath));
}

std::ifstream in(txtPath, std::ios::binary);
if (!in.is_open())
{
printf("ERROR: could not open %s\n", txtPath);
delete table;
return 1;
}

std::string line;
int count = 0;
int failed = 0;
while (std::getline(in, line))
{
if (line.empty())
continue;
std::string::size_type tab = line.find('\t');
if (tab == std::string::npos)
continue;

std::string key = line.substr(0, tab);
std::string val = line.substr(tab + 1);
if (!val.empty() && val[val.size() - 1] == '\r')
val.erase(val.size() - 1);

// remove existing entry with this key, if any, so re-adding doesn't collide
table->removeStringByName(key);

Unicode::String wideVal = Unicode::narrowToWide(val);
LocalizedString::id_type newId = table->getNextUniqueId();
LocalizedString * newStr = new LocalizedString(newId, wideVal);

std::string resultStr;
LocalizedString * added = table->addString(newStr, key, resultStr);
if (!added)
{
printf("FAILED to add key [%s]: %s\n", key.c_str(), resultStr.c_str());
delete newStr;
++failed;
continue;
}

++count;
}
in.close();

bool ok = table->writeRW(factory, outPath);
printf("%s: wrote %d entries (%d failed) to %s\n", ok ? "SUCCESS" : "FAILURE", count, failed, outPath);

delete table;
return (ok && failed == 0) ? 0 : 1;
}

// ----------------------------------------------------------------------

int main(int argc, char ** argv)
{
SetupSharedThread::install();
SetupSharedDebug::install(4096);

{
SetupSharedFoundation::Data data(SetupSharedFoundation::Data::D_console);
data.argc = argc;
data.argv = argv;
SetupSharedFoundation::install(data);
}
SetupSharedFile::install(false);

std::string mode, inFile, txtFile, outFile;
for (int i = 1; i < argc; ++i)
{
if (!strcmp(argv[i], "-e")) mode = "export";
else if (!strcmp(argv[i], "-m")) mode = "import";
else if (!strcmp(argv[i], "-i") && i + 1 < argc) inFile = argv[++i];
else if (!strcmp(argv[i], "-t") && i + 1 < argc) txtFile = argv[++i];
else if (!strcmp(argv[i], "-o") && i + 1 < argc) outFile = argv[++i];
}

int rc = 0;
if (mode == "export" && !inFile.empty() && !outFile.empty())
{
rc = doExport(inFile.c_str(), outFile.c_str());
}
else if (mode == "import" && !inFile.empty() && !txtFile.empty() && !outFile.empty())
{
rc = doImport(inFile.c_str(), txtFile.c_str(), outFile.c_str());
}
else
{
usage();
rc = 1;
}

SetupSharedFoundation::remove();
SetupSharedThread::remove();
return rc;
}

// ======================================================================
