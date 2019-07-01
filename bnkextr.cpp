/*
http://www.geocities.jp/aoyoume/aotuv/index.html
http://rarewares.org/ogg-oggenc.php#oggenc-aotuv
http://www.eveonline.com/ingameboard.asp?a=topic&threadID=1018956
http://forum.xentax.com/viewtopic.php?f=17&t=3477
http://wiki.xentax.com/index.php?title=Wwise_SoundBank_(*.bnk)

.BNK Format specifications

char {4} - header (BKHD) // BanK HeaDer
uint32 {4} - size of BKHD
uint32 {4} - unknown (version?)
uint32 {4} - unknown
uint32 {4} - unknown
uint32 {4} - unknown
byte {x} - zero padding (if any)

char {4} - header (DIDX) // Data InDeX
uint32 {4} - size of DIDX
following by records 12 bytes each:
	uint32 {4} - unknown
	uint32 {4} - relative file offset from start of DATA, 16 bytes aligned
	uint32 {4} - file size

char {4} - header (DATA)
uint32 {4} - size of DATA

char {4} - header (HIRC) // ???
uint32 {4} - size of HIRC

char {4} - header (STID) // Sound Type ID
uint32 {4} - size of STID
uint32 {4} - Always 1?
uint32 {4} - Always 1?
uint32 {4} - unknown
byte {1} - TID Length (TL)
char {TL} - TID string (usually same as filename, but without extension)

Init.bnk
STMG
HIRC
FXPR
ENVS
*/

#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <sys/stat.h>

#ifdef __GNUC__
#elif _MSC_VER
#include <windows.h>
#endif

struct Index;
struct Section;

typedef unsigned int u32;

#pragma pack(push, 1)
struct Index
{
	int wemId;
	int offset;
	unsigned int size;
};

#pragma pack(push, 1)
struct Section
{
	char sign[4];
	unsigned int size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct HircObject
{
    char mObjectType;
    u32 mSize;
};
#pragma pack(pop)
#pragma pack(pop)

namespace Hirc
{
enum ObjectType
	{
		sound_sfx = 2,
		event_action = 3,
		event_object = 4
	};
}



int swap32(const int dw)
{
#ifdef __GNUC__
	return __builtin_bswap32(dw);
#elif _MSC_VER
	return _byteswap_ulong(dw);
#endif
}

void MakeDirectory(std::string dirname)
{
#ifdef __GNUC__
    mkdir(dirname.c_str());
#elif _MSC_VER
    CreateDirectory(dirname.c_str(), 0);
#endif
}

template<typename T>
void bnk_read(std::fstream &file, T &a)
{
    file.read(reinterpret_cast<char*>(&a), sizeof(a));
}


int main(int argc, char* argv[])
{
	std::cout << "Wwise *.BNK File Extractor" << std::endl;
	std::cout << "(c) CTPAX-X Team 2009-2010 - http://www.CTPAX-X.org" << std::endl;
	std::cout << "(c) RAWR 2015 - http://www.rawr4firefall.com" << std::endl;
	std::cout << std::endl;

	// Has no argument(s)
	if((argc < 2) || (argc > 3))
	{
		std::cout << "Usage: bnkextr filename.bnk [/swap]" << std::endl;
		std::cout << "/swap - swap byte order (use it for unpacking 'Army of Two')" << std::endl;
		return 0;
	}

    std::string bankfile(argv[1]);
    bankfile = bankfile.substr(0, bankfile.find('.'));
	std::fstream bnkfile;
	bnkfile.open(argv[1], std::ios::binary | std::ios::in);

	// Could not open BNK file
	if(!bnkfile.is_open())
	{
		std::cout << "Can't open input file: " << argv[1] << std::endl;
		return 0;
	}

	unsigned int data_pos = 0;
	std::vector<Index> files;
	Section content_section;
	Index content_index;

	while(bnkfile.read(reinterpret_cast<char*>(&content_section), sizeof(content_section)))
	{
		unsigned int section_pos = bnkfile.tellg();

		// Was the /swap command used?
		if(argc > 3)
			content_section.size = swap32(content_section.size);

		if(std::strncmp(content_section.sign, "BKHD", 4) == 0)
		{
			u32 bankVersion;
			bnk_read(bnkfile, bankVersion);

			u32 bankID;
			bnk_read(bnkfile, bankID);

			std::ofstream outfile2;
			outfile2.open(bankfile + "-info.txt", std::ios::out);

			outfile2 << "Bank ID: " << bankID << ", version: " << bankVersion << std::endl;
			
			outfile2.close();
		}
		else if(std::strncmp(content_section.sign, "DIDX", 4) == 0)
		{
			// Read files
			for(unsigned int i = 0; i < content_section.size; i += sizeof(content_index))
			{
				bnkfile.read(reinterpret_cast<char*>(&content_index), sizeof(content_index));
				files.push_back(content_index);
			}
		}
		else if(std::strncmp(content_section.sign, "STID", 4) == 0)
		{
			// To be implemented
			std::cout << bnkfile.tellg() << std::endl;
		}
		else if(std::strncmp(content_section.sign, "DATA", 4) == 0)
		{
			// Get DATA offset
			data_pos = bnkfile.tellg();
		}
        else if (std::strncmp(content_section.sign, "HIRC", 4) == 0)
        {
            u32 objectCount;
            bnk_read(bnkfile, objectCount);

            std::ofstream outfile;
			outfile.open(bankfile + "-objects.txt", std::ios::out);

            for (unsigned int i = 0; i < objectCount; ++i)
            {
                HircObject object;
                bnk_read(bnkfile, object);

                const u32 objectPos = bnkfile.tellg();

				u32 objectID;
				bnk_read(bnkfile, objectID);

				switch (object.mObjectType) {
					case (Hirc::sound_sfx):
						outfile << "SFX ID: " << objectID << std::endl;

						unsigned char unusedBytes[5];
						bnk_read(bnkfile, unusedBytes);

						// uint8_t ytpe;
						// bnk_read(bnkfile, ytpe);
						// outfile << "\t" << "real type: " << ytpe << std::endl;

						u32 fileID;
						bnk_read(bnkfile, fileID);
						outfile << "\t" << "wem id: " << fileID << std::endl;

						u32 sourceID;
						bnk_read(bnkfile, sourceID);
						outfile << "\t" << "length: " << sourceID << std::endl;

						u32 fileType;
						bnk_read(bnkfile, fileType);
						outfile << "\t" << "type: " << fileType << std::endl;

						break;
					case (Hirc::event_action):
						outfile << "Event Action ID: " << objectID << std::endl;

						uint8_t eventActionScope;
						bnk_read(bnkfile, eventActionScope);

						uint8_t eventActionType;
						bnk_read(bnkfile, eventActionType);

						u32 eventActionObjReference;
						bnk_read(bnkfile, eventActionObjReference);
						outfile << "\t" << "Object Ref ID: " << eventActionObjReference << std::endl;

						// bnkfile.seekg(bnkfile.tellg() + 1);

						// uint8_t paramsCount;
						// bnk_read(bnkfile, paramsCount);

						// bnkfile.seekg(bnkfile.tellg() + paramsCount);
						// bnkfile.seekg(bnkfile.tellg() + paramsCount);

						// bnkfile.seekg(bnkfile.tellg() + 1);

						// u32 groupID;
						// bnk_read(bnkfile, groupID);
						// outfile << "\t" << "state/switch group id: " << groupID << std::endl;

						// u32 mainID;
						// bnk_read(bnkfile, mainID);
						// outfile << "\t" << "state/switch: " << mainID << std::endl;

						break;
					case (Hirc::event_object):
						outfile << "Event Object ID: " << objectID << std::endl;

						u32 eventCount;
						bnk_read(bnkfile, eventCount);

						for (u32 j = 0; j < eventCount; ++j)
						{
							u32 eventActionId;
							bnk_read(bnkfile, eventActionId);
							outfile << "\t" << "Event Action Ref ID: " << eventActionId << std::endl;
						}

						break;
					// case (0x05):

						// std::string inputStr;
						// bnk_read(bnkfile, inputStr);
						// bnkfile.read(reinterpret_cast<char*>(&inputStr), 8);

						// std::cout << inputStr << std::endl;

						// break;
					default:
						outfile << "unknown object: " << objectID << std::endl;
						outfile << "\t" << "type: " << object.mObjectType << std::endl;

						break;
				}

                bnkfile.seekg(objectPos + object.mSize);
            }

            outfile.close();
        }

		// Seek to the end of the section
		bnkfile.seekg(section_pos + content_section.size);
	}

	// Reset EOF
	bnkfile.clear();

	// Extract files
    MakeDirectory(bankfile);

	if((data_pos > 0) && (files.size() > 0))
	{
		for(std::size_t i = 0; i < files.size(); ++i)
		{
			std::string filename = bankfile + "/" + std::to_string(files[i].wemId) + ".wem";

			std::fstream wemfile;
			wemfile.open(filename, std::ios::out | std::ios::binary);

			// Was the /swap command used?
			if(argc > 3)
			{
				files[i].size = swap32(files[i].size);
				files[i].offset = swap32(files[i].offset);
			}

			if(wemfile.is_open())
			{
				std::vector<char> data(files[i].size, 0);

				bnkfile.seekg(data_pos + files[i].offset);
				bnkfile.read(static_cast<char*>(data.data()), files[i].size);
				wemfile.write(static_cast<char*>(data.data()), files[i].size);
			}
		}
	}
}
