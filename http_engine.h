#pragma once

#include <map>
#include <string>

enum HTTP_METHOD_NAME
{
    METHOD_GET,
    METHOD_POST,
    METHOD_HEAD,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_TRACE,
    METHOD_CONNECT,
    METHOD_OPTIONS,
    METHOD_NULL
};

class RequestParser
{
public:
    RequestParser(char* request) : request_(request)
    {
        Parse();
    }

    int Method();

private:
    int Parse();

private:
    char* request_;
    bool valid_;
    int method_;
    const char* path_;
};

class HttpEngine
{
public:
    static HttpEngine* Instance()
    {
        static HttpEngine inst;
        return &inst;
    }
    ~HttpEngine()
    {
    }

    int Load(const char* path);

    int HandleRequest(char* request, char* response, size_t& length);

private:
    HttpEngine()
    {
    }

private:
    const char* dir_path_;
    std::map<std::string, std::string> html_text_;
};
