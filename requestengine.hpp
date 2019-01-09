#ifndef REQENGINE_H
#define REQENGINE_H

#include <fstream>
#include <string>
#include <vector>
#include "utils/base64.h"
#include "auth_strategy/authstrategy.hpp"
#include <boost/filesystem.hpp>

namespace FS = boost::filesystem;
//
// Request engine provides interface for executing operations on the server
//
class RequestEngine
{
private:
  using string = std::string;

  string data_root; // path to directory with users catalogues
  string auth_root; // path to directory with auth files
  AuthStrategy *auth;

public:
  RequestEngine(string &data_root, string &auth_root)
  {
    this->data_root = data_root;
    this->auth_root = auth_root;
  }
  RequestEngine(const char *data_root, const char *auth_root) : data_root(data_root), auth_root(auth_root)
  {
  }

  public:
    RequestEngine(string& data_root, string& auth_root, AuthStrategy *auth)
    {
        this->data_root = data_root;
        this->auth_root = auth_root;
        this->auth = auth;
    }
    RequestEngine(const char* data_root, const char* auth_root, AuthStrategy *auth): data_root(data_root), auth_root(auth_root), auth(auth)
    {}

    int createUser(const string &username, const string &password, const string &publicLimit, const string &privateLimit, string &publicUsed, string &privateUsed)
    {
      try
      {
        std::ofstream usersFile;
        usersFile.open(auth_root + "users.auth", std::ios::app);
        usersFile << username + ":" + password + ":" + publicLimit + ":" + privateLimit + ":" + publicUsed + ":" + privateUsed + "\n";
        usersFile.close();
        return 0;
      }
      catch (...)
      {
        return -1;
      }
    }

    int deleteUser(const string &username)
    {
      try
      {
        std::ifstream usersFile;
        std::ofstream newUsersFile;
        usersFile.open(auth_root + "users.auth");
        newUsersFile.open(auth_root + "usersTemp.auth");

        string line;
        bool userDeleted = false;

        while (std::getline(usersFile, line))
        {
          string usrName = splitWithDelimiter(line, ':')[0];
          if (usrName != username)
            newUsersFile << line << std::endl;
          else
            userDeleted = true;
        }

        if (!userDeleted)
          return -1;

        const string oldFile = auth_root + "users.auth";
        const string tempFile = auth_root + "usersTemp.auth";

        remove(oldFile.c_str());
        rename(tempFile.c_str(), oldFile.c_str());

        usersFile.close();
        newUsersFile.close();
        return 0;
      }
      catch (...)
      {
        return -1;
      }
    }

    int alterUser(const string &username, const string &password, const string &pubLimit, const string &privLimit)
    {
      try
      {
        // find user
        string userLine = auth->getUserLine(username);

        if (userLine == "")
          return -1;

        //std::cout << userLine << std::endl;
        std::vector<string> user = splitWithDelimiter(userLine, ':');

        // save used spaces
        string pubUsed = user[4];
        string privUsed = user[5];

        // delete old user
        if (deleteUser(username) != 0)
          return -1;

        // create new
        if (createUser(username, password, pubLimit, privLimit, pubUsed, privUsed) != 0)
          return -1;

        return 0;
      }
      catch (...)
      {
        return -1;
      }
    }

    User* getUser(string &username)
    {
      try
      {
        string userLine = auth->getUserLine(username);

        if (userLine == "")
          return nullptr;

        std::vector<string> user = splitWithDelimiter(userLine, ':');
        string password = user[1];
        string publicLimit = user[2];
        string privateLimit = user[3];
        string publicUsed = user[4];
        string privateUsed = user[5];

        return new User(username, password, std::stoi(publicLimit), std::stoi(privateLimit), std::stof(publicUsed.c_str()), std::stof(privateUsed.c_str()));
      }
      catch (...)
      {
        return nullptr;
      }
    }

    int sendFile(const string &path, string &fileChunk)
    {
      const int chunkSize = 30;  // TODO: zmienic to na jakis const
      const int offset = 0;

      // read chunk of file considering offset
      std::ifstream file(path, std::ios::binary);
      file.seekg(offset);
      char *buffer = new char[chunkSize];
      file.read(buffer, chunkSize);

      // Encode buffer
      unsigned char *bufferToEncode = reinterpret_cast<unsigned char*>(buffer);
      fileChunk = base64_encode(bufferToEncode, chunkSize);

      // 7. clean up
      file.close();
      delete[] buffer;

      return 0;
    }

  int createFile(const string &path, const string &name, string &err_msg)
  {
    string p = data_root+path;
    try
    {
      if (!FS::exists(p))
      {
        err_msg = path + " does not exist";
        return -1;
      }
      std::ofstream f(p+"/"+name);
      if(f.is_open())
        f.close();
    }
    catch (const FS::filesystem_error &ex)
    {
      err_msg = ex.what();
      return -1;
    }
    return 0;
  }

  int createDirectory(const string &path, const string &name, string &err_msg)
  {
    try
    {
      FS::create_directory(data_root + path + "/" + name);
    }
    catch (const FS::filesystem_error &ex)
    {
      err_msg = ex.what();
      return -1;
    }
    return 0;
  }

  //
  // Go through given path put all file names in FILES and all directories names in DIRS
  //
  int listDirectory(const string &path, std::vector<string> &files, std::vector<string> &dirs, string &err_msg)
  {
    string p = data_root+path;
    try
    {
      if (FS::exists(p))
      {
        if (FS::is_directory(p))
        {
          for (FS::directory_entry &i : FS::directory_iterator(p))
          {
            if (FS::is_directory(i.path()))
              dirs.push_back(i.path().filename().string());
            else
              files.push_back(i.path().filename().string());
          }
        }
        return 0;
      }
      else
      {
        err_msg = path + " does not exist.";
      }
    }
    catch (const FS::filesystem_error &ex)
    {
      err_msg = ex.what();
      return -1;
    }
  }

  int deleteFile(const string &path, string& err_msg)
  {
    try
    {
      int files_removed = FS::remove_all(data_root + path);
      return files_removed;
    }
    catch (const FS::filesystem_error &ex)
    {
      err_msg = ex.what();
      return -1;
    }
  }
};

#endif // REQENGINE_H
