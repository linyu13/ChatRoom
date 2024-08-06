#pragma once
#include "../include/headFile.hpp"

class RedisAsyncContext
{
public:
    RedisAsyncContext();
    ~RedisAsyncContext();

    // 哈希表的相关操作
    int HashSet(const std::string& key, const std::string& field, const std::string& value);
    int HashDele(const std::string& key, const std::string& field);
    bool HashExists(const std::string& key, const std::string& field) const;
    std::string HashGet(const std::string& key, const std::string& field) const;
    std::unordered_map<std::string, std::string> HashGetAll(const std::string& key) const;
    int HashClear(const std::string& key);

    // 集合的相关操作
    int Insert(const std::string& key, const std::string& member);
    bool MemberExists(const std::string& key, const std::string& member) const;
    int DeleteValue(const std::string& key, const std::string& value);
    int DeleteAll(const std::string& key);

    // 有序集合的相关操作
    int ZAdd(const std::string& key, int score, const std::string& member);
    int ZAdd(const std::string& key, const std::string& score, const std::string& member);
    std::vector<std::string> ZRange(const std::string& key, int start, int stop) const;
    int ZRem(const std::string& key, const std::string& member);
    bool ZMemberExists(const std::string& key, const std::string& member) const;
    int ZClear(const std::string& key);

    // 列表的相关操作
    int LPush(const std::string& key, const std::string& value);
    int LLen(const std::string& key) const;
    std::vector<std::string> LRange(const std::string& key, int start, int stop) const;
    int LTrim(const std::string& key, int start, int stop);
    std::string LPop(const std::string& key);

private:
    redisReply* ExecuteCommand(const char* format, ...) const;

    std::unique_ptr<redisContext, decltype(&redisFree)> m_connection;
};

inline RedisAsyncContext::RedisAsyncContext()
    : m_connection(redisConnect("127.0.0.1", 6379), &redisFree)
{
    if (m_connection->err)
    {
        throw std::runtime_error("Failed to connect to Redis: " + std::string(m_connection->errstr));
    }
}

inline RedisAsyncContext::~RedisAsyncContext() = default;

inline redisReply* RedisAsyncContext::ExecuteCommand(const char* format, ...) const
{
    va_list args;
    va_start(args, format);
    redisReply* reply = (redisReply*)redisvCommand(m_connection.get(), format, args);
    va_end(args);

    if (!reply)
    {
        std::cerr << "Error: redisCommand returned NULL" << std::endl;
        throw std::runtime_error("Redis command failed.");
    }

    return reply;
}