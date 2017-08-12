/*
 * FileHelper.h
 *
 *  Created on: 2013年9月10日
 *      Author: Administrator
 */

#ifndef FILEHELPER_H_
#define FILEHELPER_H_
#include <string>
#include <vector>
#include <fstream>
#include <stdio.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <stdarg.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
#define ACCESS _access
#define MKDIR(a) _mkdir((a))
#else
#define ACCESS access
#define MKDIR(a) mkdir((a),0755)
#endif

class FileHelper
{
public:
    static bool save(const std::string filename, std::string& content)
    {
        FILE *file = fopen(filename.c_str(), "wb");

        if (file == NULL)
            return false;
        fwrite(content.c_str(),sizeof(char),content.size(),file);
        fclose(file);
        return true;
    }

    // used to open binary file
    static bool open(const std::string filename, std::string& content)
    {
        FILE *file = fopen(filename.c_str(), "rb");

        if (file == NULL)
            return false;

        fseek(file, 0, SEEK_END);
        int len = ftell(file);
        rewind(file);
        content.clear();
        char *buffer = new char[len];
        fread(buffer, sizeof(char), len, file);
        content.assign(buffer, len);
        delete []buffer;

        //int nRead;
        //content.clear();
        //char buffer[80];
        //while(!feof(file)){
        //  nRead = fread(buffer,sizeof(char),sizeof(buffer),file);
        //  if(nRead > 0){
        //      content.append(buffer);
        //  }
        //}
        fclose(file);
        return true;
    }

    // used to open text file
    static bool open(const std::string file_name, std::vector<std::string>& lines)
    {
        std::ifstream file(file_name.c_str(), std::ios::in);
        if (!file)
        {
            return false;
        }

        lines.clear();
        char buffer[BUFFER_SIZE];

        while (file.getline(buffer, BUFFER_SIZE, '\n'))
        {
            lines.push_back(buffer);
        }

        return true;
    }
    static bool CreateDir(const char *pszDir)
    {
        size_t i = 0;
        size_t iRet;
        size_t iLen = strlen(pszDir);
        char* buf=new char[iLen+1];
        strncpy(buf,pszDir,iLen+1);
        for (i = 1;i < iLen;i ++)    {
            if (pszDir[i] == '\\' || pszDir[i] == '/')  {
                buf[i] = '\0';
                //如果不存在,创建
                iRet = ACCESS(buf,0);
                if (iRet != 0)  {
                    iRet = MKDIR(buf);
                    if (iRet != 0)  {
                        delete[] buf;
                        return false;
                    }
                }
                //支持linux,将所有\换成/
                buf[i] = '/';
            }
        }
        delete[] buf;
        return true;
    }

private:
    enum { BUFFER_SIZE = 3000 };

};

#endif /* FILEHELPER_H_ */
