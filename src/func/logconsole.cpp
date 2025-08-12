#include "logconsole.hpp"
#include "log/loglevel.hpp"
#include "log/logevent.hpp"
#include "log/logformatter.hpp"
#include <iostream>
namespace zhao
{
    void StdoutConsole::log(LogEvent::ptr event)
    {
        std::cout << m_formatter->format(event) << std::endl;
    }
    FileConsole::FileConsole(const std::string &filename) : m_filename(filename)
    {
        // 检查文件流是否已经打开
        if (!m_filestream.is_open())
        {
            //查看文件是否存在,存在就删除掉
            if(access(m_filename.c_str(), F_OK) == 0)
                unlink(m_filename.c_str());
            // 打开文件，如果文件不存在则创建，支持追加写入
            m_filestream.open(m_filename, std::ios::app | std::ios::out);

            // 检查文件是否成功打开
            if (!m_filestream.is_open())
            {
                std::cerr << "Failed to open log file: " << m_filename << std::endl;
            }
        }
    }
    void FileConsole::reloacation(const std::string &filename)
    {
        if(m_filestream.is_open())
        {
            m_filestream.close();
        }
        m_filestream.open(filename, std::ios::app | std::ios::out);
        m_filename = filename;
    }
    void FileConsole::log(LogEvent::ptr event)
    {
        m_filestream << m_formatter->format(event) ;
        m_filestream.flush();  // 强制立即写入文件
    }
}