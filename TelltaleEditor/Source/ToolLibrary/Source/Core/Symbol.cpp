#include <Core/Symbol.hpp>
#include <Resource/DataStream.hpp>
#include <Core/Context.hpp>

#include <sstream>
#include <iomanip>
#include <cstdint>

// Standard CRC32 and CRC64 routines and tables.

U64 CRC64(const U8 *Buffer, U32 BufferLength, U64 InitialCRC64)
{
    for (U32 i = 0; i < BufferLength; i++)
    {
        InitialCRC64 = CRC64_Table[((U32)(InitialCRC64 >> 56) ^ Buffer[i]) & 0xFF] ^ (InitialCRC64 << 8);
    }
    return InitialCRC64;
}

U64 CRC64LowerCase(const U8 *Buffer, U32 BufferLength, U64 InitialCRC64)
{
    for (U32 i = 0; i < BufferLength; i++)
    {
        // In lower case, bit 6 is set (0x20).
        U8 letter = Buffer[i];
        if (letter >= 0x41 && letter <= 0x5A) //[A,Z]
            letter |= 0x20;

        InitialCRC64 = CRC64_Table[((U32)(InitialCRC64 >> 56) ^ letter) & 0xFF] ^ (InitialCRC64 << 8);
    }
    return InitialCRC64;
}

U32 CRC32(const U8 *Buffer, U32 BufferLength, U32 InitialCRC32)
{
    InitialCRC32 = ~InitialCRC32; // Is inverted
    for (U32 i = 0; i < BufferLength; i++)
    {
        InitialCRC32 = CRC32_Table[(U32)(InitialCRC32 ^ Buffer[i]) & 0xFF] ^ (InitialCRC32 >> 8);
    }
    return ~InitialCRC32; // Inverted again
}

SymbolTable RuntimeSymbols{};
SymbolTable GameSymbols{};

SymbolTable* SymbolTable::_ActiveTables = nullptr;

Symbol SymbolFromHexString(const String& str)
{
    if(str.length() != 16)
        return Symbol();
    
    // Parse the string as a hexadecimal number
    U64 result{};
    std::istringstream iss(str);
    iss >> std::hex >> result;
    
    if(iss.fail())
        return Symbol();
    
    return result;
}

// Converts to a 16 byte hex string
String SymbolToHexString(Symbol sym)
{
    std::ostringstream oss;
    oss << std::setw(16) << std::setfill('0') << std::hex << std::uppercase << sym;
    return oss.str();
}

// SYMBOL TABLE

void SymbolTable::SerialiseIn(DataStreamRef& stream)
{
    TTE_ASSERT(IsCallingFromMain(), "Can only be called on main thread");
    if(stream->GetSize() == 0)
        return; // empty file
    
    // read total file into string stream.
    std::istringstream ss{};
    U8* temp = TTE_ALLOC(stream->GetSize() + 1, MEMORY_TAG_TEMPORARY);
    temp[stream->GetSize()] = 0; // null terminator for string
    stream->Read(temp, stream->GetSize());
    ss.str((CString)temp);
    TTE_FREE(temp);
    
    // keep reading lines
    for (String line; std::getline(ss, line);)
    {
        if(line.length())
            Register(line);
    }
}

void SymbolTable::SerialiseOut(DataStreamRef& stream)
{
    TTE_ASSERT(IsCallingFromMain(), "Can only be called on main thread");
    std::lock_guard<std::mutex> _L{_Lock};
    std::ostringstream ss{};
    for(auto& str: _Table)
    {
        ss << str << "\n";
    }
    String str = ss.str();
    stream->Write((const U8*)str.c_str(), (U64)str.length());
}

SymbolTable::SymbolTable()
{
    _Next = _ActiveTables;
    _ActiveTables = this;
}

String SymbolTable::_Find(Symbol sym)
{
    std::lock_guard<std::mutex> _L{_Lock};
    auto it = _SortedHashed.find(sym); // BINARY SEARCH
    if(it == _SortedHashed.end())
    {
        return ""; // not found
    }
    else
    {
        String& str = _Table[it->second];
        return StringStartsWith(str, "CRC32:") ? String(str.c_str() + 6) : str;
    }
}

void SymbolTable::Clear()
{
    std::lock_guard<std::mutex> _L{_Lock};
    _Table.clear();
    _SortedHashed.clear();
}

void SymbolTable::Register(const String& str)
{
    if(str.length() == 0)
        return;
    std::lock_guard<std::mutex> _L{_Lock};
    
    Symbol sym;
    if(StringStartsWith(str, "CRC32:"))
    {
        CString s = str.c_str() + 6;
        sym = Symbol((U64)CRC32((const U8*)s, (U32)str.length() - 6));
    }
    else
        sym = Symbol(str);
    
    auto it = _SortedHashed.find(sym); // BINARY SEARCH
    if(it == _SortedHashed.end())
    {
        // Append
        U32 index = (U32)_Table.size();
        _SortedHashed[sym] = index;
        _Table.push_back(str);
    }
    else
    {
        // Ensure no hash collision!
        if(!CompareCaseInsensitive(str, _Table[it->second]))
        {
            TTE_ASSERT(false, "Detected hash collision! '%s' and '%s'"
                       " have the same case insensitive hash!", str.c_str(), _Table[it->second].c_str());
        }
    }
}

const U32 CRC32_Table[256] = {
    0,          0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D};

const U64 CRC64_Table[256] = {
    0x0000000000000000L, 0x42F0E1EBA9EA3693L, 0x85E1C3D753D46D26L, 0xC711223CFA3E5BB5L, 0x493366450E42ECDFL, 0x0BC387AEA7A8DA4CL, 0xCCD2A5925D9681F9L,
    0x8E224479F47CB76AL, 0x9266CC8A1C85D9BEL, 0xD0962D61B56FEF2DL, 0x17870F5D4F51B498L, 0x5577EEB6E6BB820BL, 0xDB55AACF12C73561L, 0x99A54B24BB2D03F2L,
    0x5EB4691841135847L, 0x1C4488F3E8F96ED4L, 0x663D78FF90E185EFL, 0x24CD9914390BB37CL, 0xE3DCBB28C335E8C9L, 0xA12C5AC36ADFDE5AL, 0x2F0E1EBA9EA36930L,
    0x6DFEFF5137495FA3L, 0xAAEFDD6DCD770416L, 0xE81F3C86649D3285L, 0xF45BB4758C645C51L, 0xB6AB559E258E6AC2L, 0x71BA77A2DFB03177L, 0x334A9649765A07E4L,
    0xBD68D2308226B08EL, 0xFF9833DB2BCC861DL, 0x388911E7D1F2DDA8L, 0x7A79F00C7818EB3BL, 0xCC7AF1FF21C30BDEL, 0x8E8A101488293D4DL, 0x499B3228721766F8L,
    0x0B6BD3C3DBFD506BL, 0x854997BA2F81E701L, 0xC7B97651866BD192L, 0x00A8546D7C558A27L, 0x4258B586D5BFBCB4L, 0x5E1C3D753D46D260L, 0x1CECDC9E94ACE4F3L,
    0xDBFDFEA26E92BF46L, 0x990D1F49C77889D5L, 0x172F5B3033043EBFL, 0x55DFBADB9AEE082CL, 0x92CE98E760D05399L, 0xD03E790CC93A650AL, 0xAA478900B1228E31L,
    0xE8B768EB18C8B8A2L, 0x2FA64AD7E2F6E317L, 0x6D56AB3C4B1CD584L, 0xE374EF45BF6062EEL, 0xA1840EAE168A547DL, 0x66952C92ECB40FC8L, 0x2465CD79455E395BL,
    0x3821458AADA7578FL, 0x7AD1A461044D611CL, 0xBDC0865DFE733AA9L, 0xFF3067B657990C3AL, 0x711223CFA3E5BB50L, 0x33E2C2240A0F8DC3L, 0xF4F3E018F031D676L,
    0xB60301F359DBE0E5L, 0xDA050215EA6C212FL, 0x98F5E3FE438617BCL, 0x5FE4C1C2B9B84C09L, 0x1D14202910527A9AL, 0x93366450E42ECDF0L, 0xD1C685BB4DC4FB63L,
    0x16D7A787B7FAA0D6L, 0x5427466C1E109645L, 0x4863CE9FF6E9F891L, 0x0A932F745F03CE02L, 0xCD820D48A53D95B7L, 0x8F72ECA30CD7A324L, 0x0150A8DAF8AB144EL,
    0x43A04931514122DDL, 0x84B16B0DAB7F7968L, 0xC6418AE602954FFBL, 0xBC387AEA7A8DA4C0L, 0xFEC89B01D3679253L, 0x39D9B93D2959C9E6L, 0x7B2958D680B3FF75L,
    0xF50B1CAF74CF481FL, 0xB7FBFD44DD257E8CL, 0x70EADF78271B2539L, 0x321A3E938EF113AAL, 0x2E5EB66066087D7EL, 0x6CAE578BCFE24BEDL, 0xABBF75B735DC1058L,
    0xE94F945C9C3626CBL, 0x676DD025684A91A1L, 0x259D31CEC1A0A732L, 0xE28C13F23B9EFC87L, 0xA07CF2199274CA14L, 0x167FF3EACBAF2AF1L, 0x548F120162451C62L,
    0x939E303D987B47D7L, 0xD16ED1D631917144L, 0x5F4C95AFC5EDC62EL, 0x1DBC74446C07F0BDL, 0xDAAD56789639AB08L, 0x985DB7933FD39D9BL, 0x84193F60D72AF34FL,
    0xC6E9DE8B7EC0C5DCL, 0x01F8FCB784FE9E69L, 0x43081D5C2D14A8FAL, 0xCD2A5925D9681F90L, 0x8FDAB8CE70822903L, 0x48CB9AF28ABC72B6L, 0x0A3B7B1923564425L,
    0x70428B155B4EAF1EL, 0x32B26AFEF2A4998DL, 0xF5A348C2089AC238L, 0xB753A929A170F4ABL, 0x3971ED50550C43C1L, 0x7B810CBBFCE67552L, 0xBC902E8706D82EE7L,
    0xFE60CF6CAF321874L, 0xE224479F47CB76A0L, 0xA0D4A674EE214033L, 0x67C58448141F1B86L, 0x253565A3BDF52D15L, 0xAB1721DA49899A7FL, 0xE9E7C031E063ACECL,
    0x2EF6E20D1A5DF759L, 0x6C0603E6B3B7C1CAL, 0xF6FAE5C07D3274CDL, 0xB40A042BD4D8425EL, 0x731B26172EE619EBL, 0x31EBC7FC870C2F78L, 0xBFC9838573709812L,
    0xFD39626EDA9AAE81L, 0x3A28405220A4F534L, 0x78D8A1B9894EC3A7L, 0x649C294A61B7AD73L, 0x266CC8A1C85D9BE0L, 0xE17DEA9D3263C055L, 0xA38D0B769B89F6C6L,
    0x2DAF4F0F6FF541ACL, 0x6F5FAEE4C61F773FL, 0xA84E8CD83C212C8AL, 0xEABE6D3395CB1A19L, 0x90C79D3FEDD3F122L, 0xD2377CD44439C7B1L, 0x15265EE8BE079C04L,
    0x57D6BF0317EDAA97L, 0xD9F4FB7AE3911DFDL, 0x9B041A914A7B2B6EL, 0x5C1538ADB04570DBL, 0x1EE5D94619AF4648L, 0x02A151B5F156289CL, 0x4051B05E58BC1E0FL,
    0x87409262A28245BAL, 0xC5B073890B687329L, 0x4B9237F0FF14C443L, 0x0962D61B56FEF2D0L, 0xCE73F427ACC0A965L, 0x8C8315CC052A9FF6L, 0x3A80143F5CF17F13L,
    0x7870F5D4F51B4980L, 0xBF61D7E80F251235L, 0xFD913603A6CF24A6L, 0x73B3727A52B393CCL, 0x31439391FB59A55FL, 0xF652B1AD0167FEEAL, 0xB4A25046A88DC879L,
    0xA8E6D8B54074A6ADL, 0xEA16395EE99E903EL, 0x2D071B6213A0CB8BL, 0x6FF7FA89BA4AFD18L, 0xE1D5BEF04E364A72L, 0xA3255F1BE7DC7CE1L, 0x64347D271DE22754L,
    0x26C49CCCB40811C7L, 0x5CBD6CC0CC10FAFCL, 0x1E4D8D2B65FACC6FL, 0xD95CAF179FC497DAL, 0x9BAC4EFC362EA149L, 0x158E0A85C2521623L, 0x577EEB6E6BB820B0L,
    0x906FC95291867B05L, 0xD29F28B9386C4D96L, 0xCEDBA04AD0952342L, 0x8C2B41A1797F15D1L, 0x4B3A639D83414E64L, 0x09CA82762AAB78F7L, 0x87E8C60FDED7CF9DL,
    0xC51827E4773DF90EL, 0x020905D88D03A2BBL, 0x40F9E43324E99428L, 0x2CFFE7D5975E55E2L, 0x6E0F063E3EB46371L, 0xA91E2402C48A38C4L, 0xEBEEC5E96D600E57L,
    0x65CC8190991CB93DL, 0x273C607B30F68FAEL, 0xE02D4247CAC8D41BL, 0xA2DDA3AC6322E288L, 0xBE992B5F8BDB8C5CL, 0xFC69CAB42231BACFL, 0x3B78E888D80FE17AL,
    0x7988096371E5D7E9L, 0xF7AA4D1A85996083L, 0xB55AACF12C735610L, 0x724B8ECDD64D0DA5L, 0x30BB6F267FA73B36L, 0x4AC29F2A07BFD00DL, 0x08327EC1AE55E69EL,
    0xCF235CFD546BBD2BL, 0x8DD3BD16FD818BB8L, 0x03F1F96F09FD3CD2L, 0x41011884A0170A41L, 0x86103AB85A2951F4L, 0xC4E0DB53F3C36767L, 0xD8A453A01B3A09B3L,
    0x9A54B24BB2D03F20L, 0x5D45907748EE6495L, 0x1FB5719CE1045206L, 0x919735E51578E56CL, 0xD367D40EBC92D3FFL, 0x1476F63246AC884AL, 0x568617D9EF46BED9L,
    0xE085162AB69D5E3CL, 0xA275F7C11F7768AFL, 0x6564D5FDE549331AL, 0x279434164CA30589L, 0xA9B6706FB8DFB2E3L, 0xEB46918411358470L, 0x2C57B3B8EB0BDFC5L,
    0x6EA7525342E1E956L, 0x72E3DAA0AA188782L, 0x30133B4B03F2B111L, 0xF7021977F9CCEAA4L, 0xB5F2F89C5026DC37L, 0x3BD0BCE5A45A6B5DL, 0x79205D0E0DB05DCEL,
    0xBE317F32F78E067BL, 0xFCC19ED95E6430E8L, 0x86B86ED5267CDBD3L, 0xC4488F3E8F96ED40L, 0x0359AD0275A8B6F5L, 0x41A94CE9DC428066L, 0xCF8B0890283E370CL,
    0x8D7BE97B81D4019FL, 0x4A6ACB477BEA5A2AL, 0x089A2AACD2006CB9L, 0x14DEA25F3AF9026DL, 0x562E43B4931334FEL, 0x913F6188692D6F4BL, 0xD3CF8063C0C759D8L,
    0x5DEDC41A34BBEEB2L, 0x1F1D25F19D51D821L, 0xD80C07CD676F8394L, 0x9AFCE626CE85B507L};
