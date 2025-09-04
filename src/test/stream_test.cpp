#include "stream/bytestream.hpp"
#include "log/logger.hpp"
#define MODULE_NAME "bytestream"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))



/**
 * @brief ByteStream测试
 */
static void test01() {
    dbg << "开始ByteStream测试...";
    
    zhao::ByteStream bs;
    
    // 测试基本类型读写
    bs.writeInt8(-8);
    bs.writeUint8(8);
    bs.writeInt16(-1616);
    bs.writeUint16(1616);
    bs.writeInt32(-323232);
    bs.writeUint32(323232);
    bs.writeInt64(-6464646464);
    bs.writeUint64(6464646464);
    bs.writeFloat(3.14f);
    bs.writeDouble(3.141592653589793);
    
    // 重置读取位置
    bs.lseek(0);
    
    // 验证读取的值
    dbg << "读取int8: " << (int)bs.readInt8();  // -8
    dbg << "读取uint8: " << (int)bs.readUint8(); // 8
    dbg << "读取int16: " << bs.readInt16();     // -1616
    dbg << "读取uint16: " << bs.readUint16();   // 1616
    dbg << "读取int32: " << bs.readInt32();     // -323232
    dbg << "读取uint32: " << bs.readUint32();   // 323232
    dbg << "读取int64: " << bs.readInt64();     // -6464646464
    dbg << "读取uint64: " << bs.readUint64();   // 6464646464
    dbg << "读取float: " << bs.readFloat();     // 3.14
    dbg << "读取double: " << bs.readDouble();   // 3.141592653589793
    
    // 测试字符串操作
    const char* testStr = "Hello ByteStream!";
    size_t len = strlen(testStr);
    bs.clear();  // 清空之前的数据
    
    bs.writeStringWithSize(testStr, len);
    bs.lseek(0);
    std::string readStr = bs.readStringWithSize();
    dbg << "读取字符串: " << readStr;
    
    // 测试数组操作
    int32_t writeArr[] = {1, -2, 3, -4, 5};
    int32_t readArr[5] = {0};
    bs.clear();
    
    bs.writeArray(writeArr, 5);
    bs.lseek(0);
    bs.readArray(readArr, 5);
    
    dbg << "读取数组:";
    for(int i = 0; i < 5; i++) {
        dbg << readArr[i] << " ";
    }
    
    dbg << "ByteStream测试完成";
}
void stream_test(void)
{
    test01();
}