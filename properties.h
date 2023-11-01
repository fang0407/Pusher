#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <map>
#include <string>
#include <vector>

class Properties: public std::map<std::string, std::string>
{
public:
    bool HasProperty(const std::string &key) const
    {
        return find(key) != end();
    }
    void SetProperty(const char* key, bool intval)
    {
        SetProperty(std::string(key), std::to_string(intval));
    }
    void SetProperty(const char* key, int intval)
    {
        SetProperty(std::string(key), std::to_string(intval));
    }

    void SetProperty(const char* key, uint32_t val)
    {
        SetProperty(std::string(key), std::to_string(val));
    }

    void SetProperty(const char* key, uint64_t val)
    {
        SetProperty(std::string(key), std::to_string(val));
    }

    void SetProperty(const char* key, const char* val)
    {
        SetProperty(std::string(key), std::string(val));
    }

    void SetProperty(const std::string &key, const std::string &val)
    {
        insert(std::pair<std::string, std::string>(key, val));
    }

    void GetChildren(const std::string& path, Properties &children) const
    {
        //Create sarch string
        std::string parent(path);
        //Add the final .
        parent += ".";
        //For each property
        for (const_iterator it = begin(); it != end(); ++it) {
            const std::string &key = it->first;
            //Check if it is from parent
            if (key.compare(0, parent.length(), parent) == 0)
            //INsert it
            {
                children.SetProperty(key.substr(parent.length(), key.length() - parent.length()), it->second);
            }
        }
    }

    void GetChildren(const char* path, Properties &children) const
    {
        GetChildren(std::string(path), children);
    }

    Properties GetChildren(const std::string& path) const
    {
        Properties properties;
        //Get them
        GetChildren(path, properties);
        //Return
        return properties;
    }

    Properties GetChildren(const char* path) const
    {
        Properties properties;
        //Get them
        GetChildren(path, properties);
        //Return
        return properties;
    }

    void GetChildrenArray(const char* path, std::vector<Properties> &array) const
    {
        //Create sarch string
        std::string parent(path);
        //Add the final .
        parent += ".";
        //Get array length
        int length = GetProperty(parent + "length", 0);
        //For each element
        for (int i = 0; i < length; ++i) {
            char index[64];
            //Print string
            snprintf(index, sizeof(index), "%d", i);
            //And get children
            array.push_back(GetChildren(parent + index));
        }
    }

    const char* GetProperty(const char* key) const
    {
        return GetProperty(key, "");
    }

    std::string GetProperty(const char* key, const std::string defaultValue) const
    {
        //Find item
        const_iterator it = find(std::string(key));
        //If not found
        if (it == end())
        //return default
        {
            return defaultValue;
        }
        //Return value
        return it->second;
    }

    std::string GetProperty(const std::string &key, const std::string defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
        //return default
        {
            return defaultValue;
        }
        //Return value
        return it->second;
    }

    const char* GetProperty(const char* key, const char *defaultValue) const
    {
        //Find item
        const_iterator it = find(std::string(key));
        //If not found
        if (it == end())
        //return default
        {
            return defaultValue;
        }
        //Return value
        return it->second.c_str();
    }

    const char* GetProperty(const std::string &key, char *defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
        //return default
        {
            return defaultValue;
        }
        //Return value
        return it->second.c_str();
    }

    int GetProperty(const char* key, int defaultValue) const
    {
        return GetProperty(std::string(key), defaultValue);
    }

    int GetProperty(const std::string &key, int defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
        //return default
        {
            return defaultValue;
        }
        //Return value
        return atoi(it->second.c_str());
    }

    uint64_t GetProperty(const char* key, uint64_t defaultValue) const
    {
        return GetProperty(std::string(key), defaultValue);
    }

    uint64_t GetProperty(const std::string &key, uint64_t defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
        //return default
        {
            return defaultValue;
        }
        //Return value
        return atoll(it->second.c_str());
    }

    bool GetProperty(const char* key, bool defaultValue) const
    {
        return GetProperty(std::string(key), defaultValue);
    }

    bool GetProperty(const std::string &key, bool defaultValue) const
    {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
        //return default
        {
            return defaultValue;
        }
        //Get value
        char * val = (char *)it->second.c_str();
        //Check it
        if (strcasecmp(val, (char *)"yes") == 0) {
            return true;
        } else if (strcasecmp(val, (char *)"true") == 0) {
            return true;
        }
        //Return value
        return (atoi(val));
    }
};

#endif // PROPERTIES_H
