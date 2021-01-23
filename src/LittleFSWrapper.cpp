#include <LittleFSWrapper.hpp>

void LittleFSWrapper::init()
{

}

uint8_t LittleFSWrapper::readByteIfExists(String filename, uint8_t defaultValue)
{
  
    return 0;
}
bool LittleFSWrapper::writeByte(String filename, uint8_t byte)
{

    return true;
}

uint32_t LittleFSWrapper::readIntIfExists(String filename, uint32_t defaultValue)
{
    uint32_t ret = defaultValue;
  return ret;
}

bool LittleFSWrapper::writeInt(String filename, uint32_t value)
{
  
    return 0 > -1;
}

String LittleFSWrapper::readStringIfExists(String filename, String defaultValue)
{
    auto ret = defaultValue;
        return ret;
   
}
bool LittleFSWrapper::writeString(String filename, String value)
{
   
    return 0 > -1;
}


LittleFSWrapper fileSystem;