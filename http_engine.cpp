#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <memory.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <set>
#include <string>

#include "log.h"

#include "http_engine.h"

#define __error_line (-14000 - __LINE__)

struct MethodName
{
    const char* name;
    int support;
};

static MethodName method_name[] = {{"GET", 1},
                                   {"POST", 0},
                                   {"HEAD", 0},
                                   {"PUT", 0},
                                   {"DELETE", 0},
                                   {"TRACE", 0},
                                   {"CONNECT", 0},
                                   {"OPTIONS", 0},
                                   {NULL, 0}};

static bool IsStringEndsWith(const std::string& sentence, const std::string& suffix)
{
    if (sentence.size() < suffix.size())
    {
        return false;
    }
    return (0 == sentence.compare(sentence.size() - suffix.size(), suffix.size(), suffix));
}

static int ReadTypeFileRecursive(const char* dir_path,
                                 const char* suffix,
                                 std::set<std::string>& list)
{
    DIR* handle = opendir(dir_path);
    if (!handle)
    {
        LogErr("opendir %s err %s", dir_path, strerror(errno));
        return __error_line;
    }

    char buffer[256]      = {0};
    struct dirent* cursor = NULL;
    while ((cursor = readdir(handle)) != NULL)
    {
        if (cursor->d_name[0] == '.')
        {
            continue;
        }

        snprintf(buffer, sizeof(buffer), "%s/%s", dir_path, cursor->d_name);

        struct stat fs;
        if (stat(buffer, &fs) < 0)
        {
            LogErr("stat %s err %s", buffer, strerror(errno));
            closedir(handle);
            return __error_line;
        }

        if (S_ISREG(fs.st_mode) && IsStringEndsWith(cursor->d_name, suffix))
        {
            list.insert(buffer);
        }
        else if (S_ISDIR(fs.st_mode))
        {
            int ret = ReadTypeFileRecursive(buffer, suffix, list);
            if (ret < 0)
            {
                closedir(handle);
                return ret;
            }
        }
        else
        {
            continue;
        }
    }

    closedir(handle);
    return 0;
}

static int ReadFile(const char* path, std::string& text)
{
    FILE* fp = fopen(path, "r");
    if (!fp)
    {
        LogErr("open %s err %s", path, strerror(errno));
        return __error_line;
    }

    char buffer[256] = {0};
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        text.append(buffer);
    }

    fclose(fp);
    LogInfo("read %s txt:\n%s", path, text.c_str());
    return 0;
}

static void SplitByCRLF(char* text, std::vector<char*>& lines)
{
    if (text == NULL)
    {
        return;
    }

    static const char* delim = "\r\n";
    char* save               = NULL;
    char* token              = NULL;

    token = strtok_r(text, delim, &save);
    if (token)
    {
        lines.push_back(token);
    }

    while (token)
    {
        token = strtok_r(NULL, delim, &save);
        if (token)
        {
            lines.push_back(token);
        }
    }
    return 0;
}

static int SplitBySpace(char* sentence, std::vector<const char*>& words)
{
    if (sentence == NULL)
    {
        return;
    }

    static const char* delim = " ";
    char* save               = NULL;
    char* token              = NULL;

    token = strtok_r(sentence, delim, &save);
    if (token)
    {
        words.push_back(token);
    }

    while (token)
    {
        token = strtok_r(NULL, delim, &save);
        if (token)
        {
            words.push_back(token);
        }
    }
    return 0;
}

int RequestParser::Parse()
{
    return 0;
}

int HttpEngine::Load(const char* path)
{
    std::set<std::string> html_path;
    int ret = ReadTypeFileRecursive(path, ".html", html_path);
    if (ret < 0)
    {
        return ret;
    }

    for (const auto& html : html_path)
    {
        std::string text;
        int ret = ReadFile(html.c_str(), text);
        if (ret < 0)
        {
            return ret;
        }

        html_text_[html] = text;
    }

    return 0;
}

int HttpEngine::HandleRequest(const char* request, char* response, size_t& length)
{
    return 0;
}
