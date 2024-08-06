#include "../include/headFile.hpp"
#include "../redis/redis.hpp"

// 哈希表的相关操作
inline int RedisAsyncContext::HashSet(const std::string& key, const std::string& field, const std::string& value)
{
    auto reply = ExecuteCommand("hmset %s %s %s", key.c_str(), field.c_str(), value.c_str());
    int type = reply->type;
    freeReplyObject(reply);
    return type;
}

inline int RedisAsyncContext::HashDele(const std::string& key, const std::string& field)
{
    auto reply = ExecuteCommand("hdel %s %s", key.c_str(), field.c_str());
    int num = reply->integer;
    freeReplyObject(reply);
    return num;
}

inline bool RedisAsyncContext::HashExists(const std::string& key, const std::string& field) const
{
    auto reply = ExecuteCommand("hexists %s %s", key.c_str(), field.c_str());
    bool exists = (reply->integer == 1);
    freeReplyObject(reply);
    return exists;
}

inline std::string RedisAsyncContext::HashGet(const std::string& key, const std::string& field) const
{
    auto reply = ExecuteCommand("hget %s %s", key.c_str(), field.c_str());
    std::string value = reply->str ? reply->str : "";
    freeReplyObject(reply);
    return value;
}

inline std::unordered_map<std::string, std::string> RedisAsyncContext::HashGetAll(const std::string& key) const
{
    std::unordered_map<std::string, std::string> result;
    auto reply = ExecuteCommand("HGETALL %s", key.c_str());

    if (reply->type == REDIS_REPLY_ARRAY)
    {
        for (size_t i = 0; i < reply->elements; i += 2)
        {
            result[reply->element[i]->str] = reply->element[i + 1]->str;
        }
    }

    freeReplyObject(reply);
    return result;
}

inline int RedisAsyncContext::HashClear(const std::string& key)
{
    auto reply = ExecuteCommand("DEL %s", key.c_str());
    int type = reply->type;
    freeReplyObject(reply);
    return type;
}

// 集合的相关操作
inline int RedisAsyncContext::Insert(const std::string& key, const std::string& member)
{
    auto reply = ExecuteCommand("sadd %s %s", key.c_str(), member.c_str());
    int type = reply->type;
    freeReplyObject(reply);
    return type;
}

inline bool RedisAsyncContext::MemberExists(const std::string& key, const std::string& member) const
{
    auto reply = ExecuteCommand("sismember %s %s", key.c_str(), member.c_str());
    bool exists = (reply->integer == 1);
    freeReplyObject(reply);
    return exists;
}

inline int RedisAsyncContext::DeleteValue(const std::string& key, const std::string& value)
{
    auto reply = ExecuteCommand("srem %s %s", key.c_str(), value.c_str());
    int num = reply->integer;
    freeReplyObject(reply);
    return num;
}

inline int RedisAsyncContext::DeleteAll(const std::string& key)
{
    auto reply = ExecuteCommand("DEL %s", key.c_str());
    int type = reply->type;
    freeReplyObject(reply);
    return type;
}

// 有序集合的相关操作
inline int RedisAsyncContext::ZAdd(const std::string& key, int score, const std::string& member)
{
    auto reply = ExecuteCommand("zadd %s %d %s", key.c_str(), score, member.c_str());
    int type = reply->type;
    freeReplyObject(reply);
    return type;
}

inline int RedisAsyncContext::ZAdd(const std::string& key, const std::string& score, const std::string& member)
{
    auto reply = ExecuteCommand("zadd %s %s %s", key.c_str(), score.c_str(), member.c_str());
    int type = reply->type;
    freeReplyObject(reply);
    return type;
}

inline std::vector<std::string> RedisAsyncContext::ZRange(const std::string& key, int start, int stop) const
{
    std::vector<std::string> members;
    auto reply = ExecuteCommand("zrange %s %d %d", key.c_str(), start, stop);

    if (reply->type == REDIS_REPLY_ARRAY)
    {
        for (size_t i = 0; i < reply->elements; ++i)
        {
            members.push_back(reply->element[i]->str);
        }
    }

    freeReplyObject(reply);
    return members;
}

inline int RedisAsyncContext::ZRem(const std::string& key, const std::string& member)
{
    auto reply = ExecuteCommand("zrem %s %s", key.c_str(), member.c_str());
    int type = reply->type;
    freeReplyObject(reply);
    return type;
}

inline bool RedisAsyncContext::ZMemberExists(const std::string& key, const std::string& member) const
{
    auto reply = ExecuteCommand("zrank %s %s", key.c_str(), member.c_str());
    bool exists = (reply->type != REDIS_REPLY_NIL);
    freeReplyObject(reply);
    return exists;
}

inline int RedisAsyncContext::ZClear(const std::string& key)
{
    auto reply = ExecuteCommand("del %s", key.c_str());
    int status = reply->type;
    freeReplyObject(reply);
    return status;
}

// 列表的相关操作
inline int RedisAsyncContext::LPush(const std::string& key, const std::string& value)
{
    auto reply = ExecuteCommand("lpush %s %s", key.c_str(), value.c_str());

    if (reply->type != REDIS_REPLY_INTEGER)
    {
        std::cerr << "Error: Expected integer reply" << std::endl;
        freeReplyObject(reply);
        return -1; // 或其他合适的错误值
    }

    int num = reply->integer;
    freeReplyObject(reply);
    return num;
}

inline int RedisAsyncContext::LLen(const std::string& key) const
{
    auto reply = ExecuteCommand("llen %s", key.c_str());
    int num = reply->integer;
    freeReplyObject(reply);
    return num;
}

inline std::vector<std::string> RedisAsyncContext::LRange(const std::string& key, int start, int stop) const
{
    std::vector<std::string> values;
    auto reply = ExecuteCommand("lrange %s %d %d", key.c_str(), start, stop);

    if (reply->type == REDIS_REPLY_ARRAY)
    {
        for (size_t i = 0; i < reply->elements; ++i)
        {
            values.push_back(reply->element[i]->str);
        }
    }

    freeReplyObject(reply);
    return values;
}

inline int RedisAsyncContext::LTrim(const std::string& key, int start, int stop)
{
    auto reply = ExecuteCommand("ltrim %s %d %d", key.c_str(), start, stop);
    int status = reply->type;
    freeReplyObject(reply);
    return status;
}

inline std::string RedisAsyncContext::LPop(const std::string& key)
{
    auto reply = ExecuteCommand("lpop %s", key.c_str());
    std::string value = (reply->type == REDIS_REPLY_STRING) ? reply->str : "";
    freeReplyObject(reply);
    return value;
}